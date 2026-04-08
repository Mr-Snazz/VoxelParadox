#include "player.hpp"

#include <algorithm>
#include <array>

#include "audio/game_audio_controller.hpp"

// Player movement:
// - first-person camera look
// - grounded body locomotion
// - stance changes
// - block collisions
// - hotbar selection

namespace {

constexpr float kCollisionEpsilon = 0.0005f;
constexpr float kGroundProbeDistance = 0.08f;
constexpr int kPenetrationResolveIterations = 8;

bool overlapsOnAxis(float minA, float maxA, float minB, float maxB) {
    return maxA > minB + kCollisionEpsilon && minA < maxB - kCollisionEpsilon;
}

glm::vec3 makeBodyMinCorner(const glm::vec3& feetPosition, float radius) {
    return feetPosition + glm::vec3(-radius, 0.0f, -radius);
}

glm::vec3 makeBodyMaxCorner(const glm::vec3& feetPosition, float radius, float bodyHeight) {
    return feetPosition + glm::vec3(radius, bodyHeight, radius);
}

float computeJumpVelocity(float gravityAcceleration, float jumpHeight) {
    return std::sqrt(glm::max(0.0f, 2.0f * gravityAcceleration * jumpHeight));
}

glm::vec3 moveTowards(const glm::vec3& current, const glm::vec3& target, float maxDelta) {
    const glm::vec3 delta = target - current;
    const float distance = glm::length(delta);
    if (distance <= 0.0001f || distance <= maxDelta) {
        return target;
    }
    return current + (delta / distance) * maxDelta;
}

}  // namespace

void Player::updateCameraLook() {
    float mdx = 0.0f;
    float mdy = 0.0f;
    Input::getMouseDelta(mdx, mdy);
    camera.rotate(mdx, mdy);
}

void Player::updateZoom(float dt, bool allowMovementInput) {
    const bool zoomHeld = allowMovementInput && Input::keyDown(GLFW_KEY_C);
    const float targetFov = zoomHeld ? zoomedFov : normalFov;
    const float blend = std::min(1.0f, dt * zoomSmoothSpeed);

    camera.baseFov += (targetFov - camera.baseFov) * blend;
}

float Player::getCurrentBodyHeight() const {
    return crouching ? crouchingHeight : standingHeight;
}

float Player::getTargetEyeHeight() const {
    return crouching ? crouchingEyeHeight : standingEyeHeight;
}

glm::vec3 Player::getFeetPosition() const {
    return camera.position - glm::vec3(0.0f, currentEyeHeight, 0.0f);
}

void Player::setFeetPosition(const glm::vec3& feetPosition) {
    camera.position = feetPosition + glm::vec3(0.0f, currentEyeHeight, 0.0f);
}

bool Player::wantsToCrouch(bool allowMovementInput) const {
    return allowMovementInput &&
           (Input::keyDown(GLFW_KEY_LEFT_SHIFT) || Input::keyDown(GLFW_KEY_RIGHT_SHIFT));
}

bool Player::canOccupyBodyAt(FractalWorld* world, const glm::vec3& feetPosition,
                             float bodyHeight) const {
    if (!world) {
        return true;
    }

    const glm::vec3 minCorner = makeBodyMinCorner(feetPosition, playerRadius);
    const glm::vec3 maxCorner =
        makeBodyMaxCorner(feetPosition, playerRadius, bodyHeight) -
        glm::vec3(kCollisionEpsilon);

    const glm::ivec3 minBlock = glm::ivec3(glm::floor(minCorner));
    const glm::ivec3 maxBlock = glm::ivec3(glm::floor(maxCorner));

    for (int bx = minBlock.x; bx <= maxBlock.x; ++bx) {
        for (int by = minBlock.y; by <= maxBlock.y; ++by) {
            for (int bz = minBlock.z; bz <= maxBlock.z; ++bz) {
                if (isSolid(world->getBlock(glm::ivec3(bx, by, bz)))) {
                    return false;
                }
            }
        }
    }

    return true;
}

bool Player::hasSupportBelow(FractalWorld* world, const glm::vec3& feetPosition,
                             float inset, bool requireAllSamples) const {
    if (!world) {
        return false;
    }

    const float sampleRadius = glm::max(0.0f, playerRadius - inset);
    const std::array<glm::vec2, 5> samples = {
        glm::vec2(0.0f, 0.0f),
        glm::vec2(-sampleRadius, -sampleRadius),
        glm::vec2(-sampleRadius, sampleRadius),
        glm::vec2(sampleRadius, -sampleRadius),
        glm::vec2(sampleRadius, sampleRadius),
    };

    const int supportY = static_cast<int>(std::floor(feetPosition.y - kGroundProbeDistance));
    bool anySupported = false;

    for (const glm::vec2& sampleOffset : samples) {
        const glm::ivec3 supportBlock(
            static_cast<int>(std::floor(feetPosition.x + sampleOffset.x)),
            supportY,
            static_cast<int>(std::floor(feetPosition.z + sampleOffset.y)));
        const bool supported = isSolid(world->getBlock(supportBlock));
        if (requireAllSamples && !supported) {
            return false;
        }
        anySupported = anySupported || supported;
    }

    return requireAllSamples ? true : anySupported;
}

void Player::updateStance(FractalWorld* world, bool allowMovementInput, float dt) {
    const glm::vec3 feetPosition = getFeetPosition();
    if (wantsToCrouch(allowMovementInput)) {
        crouching = true;
    } else if (crouching && (!world || canOccupyBodyAt(world, feetPosition, standingHeight))) {
        crouching = false;
    }

    const float blend = glm::clamp(crouchTransitionSpeed * dt, 0.0f, 1.0f);
    currentEyeHeight = glm::mix(currentEyeHeight, getTargetEyeHeight(), blend);
    setFeetPosition(feetPosition);
}

void Player::simulateMovement(float dt, FractalWorld* world, bool allowMovementInput) {
    // Split physics into substeps to keep the body stable against the voxel grid.
    float remaining = dt;
    while (remaining > 0.0f) {
        const float step = std::min(remaining, kPhysicsMaxStep);

        if (world) {
            glm::vec3 feetPosition = getFeetPosition();
            resolveBodyPenetration(world, feetPosition, getCurrentBodyHeight());
            setFeetPosition(feetPosition);
        }

        updateStance(world, allowMovementInput, step);
        grounded =
            world && hasSupportBelow(world, getFeetPosition(), 0.0f, false) && velocity.y <= 0.05f;

        handleMovement(step, allowMovementInput);

        if (world) {
            resolveCollisions(world, step);
        } else {
            camera.position += velocity * step;
            grounded = false;
        }

        remaining -= step;
    }
}

void Player::clearTargetSelection() {
    clearTargetOnly();
    resetBlockBreaking();
}

void Player::clearTargetOnly() {
    hasTarget = false;
    targetBlock = glm::ivec3(0);
    targetNormal = glm::ivec3(0);
}

void Player::resetBlockBreaking() {
    isBreakingBlock = false;
    breakingBlock = glm::ivec3(0);
    breakingBlockType = BlockType::AIR;
    breakingTimer = 0.0f;
    breakingProgress = 0.0f;
    breakingHitCooldown = 0.0f;
}

void Player::notifyInventoryStateChanged() {
    if (audioController) {
        audioController->onInventoryStateChanged(isInventoryOpen());
    }
}

void Player::handleMovement(float dt, bool allowMovementInput) {
    glm::vec3 forward = camera.getForward();
    forward.y = 0.0f;
    if (glm::dot(forward, forward) <= 0.0001f) {
        forward = glm::vec3(0.0f, 0.0f, -1.0f);
    } else {
        forward = glm::normalize(forward);
    }

    glm::vec3 right = glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f));
    if (glm::dot(right, right) <= 0.0001f) {
        right = glm::vec3(1.0f, 0.0f, 0.0f);
    } else {
        right = glm::normalize(right);
    }

    glm::vec3 moveInput(0.0f);
    if (allowMovementInput) {
        if (Input::keyDown(GLFW_KEY_W)) moveInput += forward;
        if (Input::keyDown(GLFW_KEY_S)) moveInput -= forward;
        if (Input::keyDown(GLFW_KEY_D)) moveInput += right;
        if (Input::keyDown(GLFW_KEY_A)) moveInput -= right;
    }

    if (glm::dot(moveInput, moveInput) > 0.0001f) {
        moveInput = glm::normalize(moveInput);
    }

    const bool running =
        allowMovementInput &&
        (Input::keyDown(GLFW_KEY_LEFT_CONTROL) || Input::keyDown(GLFW_KEY_RIGHT_CONTROL)) &&
        !crouching;
    const float targetSpeed = crouching ? crouchSpeed : (running ? runSpeed : walkSpeed);
    const glm::vec3 desiredHorizontalVelocity = moveInput * targetSpeed;

    glm::vec3 horizontalVelocity(velocity.x, 0.0f, velocity.z);
    const bool hasMoveInput = glm::dot(moveInput, moveInput) > 0.0001f;
    const float moveRate =
        grounded ? (hasMoveInput ? groundAcceleration : groundDeceleration)
                 : (hasMoveInput ? airAcceleration : airDeceleration);
    horizontalVelocity =
        moveTowards(horizontalVelocity, desiredHorizontalVelocity, moveRate * dt);

    if (grounded && !hasMoveInput) {
        const float frictionFactor = glm::max(0.0f, 1.0f - groundFriction * dt);
        horizontalVelocity *= frictionFactor;
    }

    velocity.x = horizontalVelocity.x;
    velocity.z = horizontalVelocity.z;

    const bool jumpPressed = allowMovementInput && Input::keyPressed(GLFW_KEY_SPACE);
    const bool jumpHeld = allowMovementInput && Input::keyDown(GLFW_KEY_SPACE);
    const bool shouldJumpNow = jumpPressed || (grounded && jumpHeld);

    if (shouldJumpNow) {
        velocity.y = computeJumpVelocity(gravityAcceleration, jumpHeight);
        grounded = false;
    } else if (grounded) {
        velocity.y = -gravityAcceleration * dt;
    } else {
        velocity.y -= gravityAcceleration * dt;
    }
}

void Player::handleHotbarSelectionInput() {
    static constexpr int hotbarKeys[PlayerHotbar::SLOT_COUNT] = {
        GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5,
        GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9,
    };

    for (int index = 0; index < PlayerHotbar::SLOT_COUNT; index++) {
        if (Input::keyPressed(hotbarKeys[index])) {
            const int previousIndex = hotbar.getSelectedIndex();
            hotbar.selectSlot(index);
            if (audioController && hotbar.getSelectedIndex() != previousIndex) {
                audioController->onHotbarSelectionChanged();
            }
            break;
        }
    }

    const float scroll = Input::getScroll();
    if (std::abs(scroll) <= 0.001f) {
        return;
    }

    int steps = static_cast<int>(std::round(scroll));
    if (steps == 0) {
        steps = scroll > 0.0f ? 1 : -1;
    }

    int selected = hotbar.getSelectedIndex();
    const int direction = steps > 0 ? -1 : 1;
    for (int i = 0; i < std::abs(steps); i++) {
        selected =
            (selected + direction + PlayerHotbar::SLOT_COUNT) % PlayerHotbar::SLOT_COUNT;
    }
    const int previousIndex = hotbar.getSelectedIndex();
    hotbar.selectSlot(selected);
    if (audioController && hotbar.getSelectedIndex() != previousIndex) {
        audioController->onHotbarSelectionChanged();
    }
}

void Player::resolveBodyPenetration(FractalWorld* world, glm::vec3& feetPosition,
                                    float bodyHeight) {
    if (!world) {
        return;
    }

    for (int iteration = 0; iteration < kPenetrationResolveIterations; ++iteration) {
        const glm::vec3 minCorner = makeBodyMinCorner(feetPosition, playerRadius);
        const glm::vec3 maxCorner =
            makeBodyMaxCorner(feetPosition, playerRadius, bodyHeight) -
            glm::vec3(kCollisionEpsilon);
        const glm::ivec3 minBlock = glm::ivec3(glm::floor(minCorner));
        const glm::ivec3 maxBlock = glm::ivec3(glm::floor(maxCorner));

        bool resolvedOverlap = false;
        for (int bx = minBlock.x; bx <= maxBlock.x && !resolvedOverlap; ++bx) {
            for (int by = minBlock.y; by <= maxBlock.y && !resolvedOverlap; ++by) {
                for (int bz = minBlock.z; bz <= maxBlock.z; ++bz) {
                    const glm::ivec3 blockPos(bx, by, bz);
                    if (!isSolid(world->getBlock(blockPos))) {
                        continue;
                    }

                    const glm::vec3 blockMin = glm::vec3(blockPos);
                    const glm::vec3 blockMax = blockMin + glm::vec3(1.0f);
                    if (!overlapsOnAxis(minCorner.x, maxCorner.x, blockMin.x, blockMax.x) ||
                        !overlapsOnAxis(minCorner.y, maxCorner.y, blockMin.y, blockMax.y) ||
                        !overlapsOnAxis(minCorner.z, maxCorner.z, blockMin.z, blockMax.z)) {
                        continue;
                    }

                    const float pushPositiveX = blockMax.x - minCorner.x;
                    const float pushNegativeX = maxCorner.x - blockMin.x;
                    const float pushPositiveY = blockMax.y - minCorner.y;
                    const float pushNegativeY = maxCorner.y - blockMin.y;
                    const float pushPositiveZ = blockMax.z - minCorner.z;
                    const float pushNegativeZ = maxCorner.z - blockMin.z;

                    glm::vec3 pushVector(pushPositiveX + kCollisionEpsilon, 0.0f, 0.0f);
                    float smallestPush = pushPositiveX;

                    if (pushNegativeX < smallestPush) {
                        smallestPush = pushNegativeX;
                        pushVector = glm::vec3(-(pushNegativeX + kCollisionEpsilon), 0.0f, 0.0f);
                    }
                    if (pushPositiveY < smallestPush) {
                        smallestPush = pushPositiveY;
                        pushVector = glm::vec3(0.0f, pushPositiveY + kCollisionEpsilon, 0.0f);
                    }
                    if (pushNegativeY < smallestPush) {
                        smallestPush = pushNegativeY;
                        pushVector = glm::vec3(0.0f, -(pushNegativeY + kCollisionEpsilon), 0.0f);
                    }
                    if (pushPositiveZ < smallestPush) {
                        smallestPush = pushPositiveZ;
                        pushVector = glm::vec3(0.0f, 0.0f, pushPositiveZ + kCollisionEpsilon);
                    }
                    if (pushNegativeZ < smallestPush) {
                        pushVector = glm::vec3(0.0f, 0.0f, -(pushNegativeZ + kCollisionEpsilon));
                    }

                    feetPosition += pushVector;
                    if (pushVector.y > 0.0f && velocity.y < 0.0f) {
                        grounded = true;
                        velocity.y = 0.0f;
                    }
                    if (pushVector.y < 0.0f && velocity.y > 0.0f) {
                        velocity.y = 0.0f;
                    }
                    resolvedOverlap = true;
                    break;
                }
            }
        }

        if (!resolvedOverlap) {
            return;
        }
    }
}

void Player::resolveCollisions(FractalWorld* world, float dt) {
    if (!world) {
        camera.position += velocity * dt;
        grounded = false;
        return;
    }

    glm::vec3 feetPosition = getFeetPosition();
    const float bodyHeight = getCurrentBodyHeight();
    const bool startedGrounded = grounded;

    auto sweepAxis = [&](int axis, float delta) {
        bool collided = false;
        if (std::abs(delta) <= kCollisionEpsilon) {
            return std::pair<float, bool>(feetPosition[axis], false);
        }

        glm::vec3 candidateFeet = feetPosition;
        candidateFeet[axis] += delta;

        const glm::vec3 minCorner = makeBodyMinCorner(candidateFeet, playerRadius);
        const glm::vec3 maxCorner =
            makeBodyMaxCorner(candidateFeet, playerRadius, bodyHeight) -
            glm::vec3(kCollisionEpsilon);
        const glm::ivec3 minBlock = glm::ivec3(glm::floor(minCorner));
        const glm::ivec3 maxBlock = glm::ivec3(glm::floor(maxCorner));

        float resolvedAxis = candidateFeet[axis];
        for (int bx = minBlock.x; bx <= maxBlock.x; ++bx) {
            for (int by = minBlock.y; by <= maxBlock.y; ++by) {
                for (int bz = minBlock.z; bz <= maxBlock.z; ++bz) {
                    const glm::ivec3 blockPos(bx, by, bz);
                    if (!isSolid(world->getBlock(blockPos))) {
                        continue;
                    }

                    const glm::vec3 blockMin = glm::vec3(blockPos);
                    const glm::vec3 blockMax = blockMin + glm::vec3(1.0f);

                    if (axis == 0) {
                        if (!overlapsOnAxis(minCorner.y, maxCorner.y, blockMin.y, blockMax.y) ||
                            !overlapsOnAxis(minCorner.z, maxCorner.z, blockMin.z, blockMax.z)) {
                            continue;
                        }
                        if (delta > 0.0f) {
                            resolvedAxis =
                                std::min(resolvedAxis, blockMin.x - playerRadius - kCollisionEpsilon);
                        } else {
                            resolvedAxis =
                                std::max(resolvedAxis, blockMax.x + playerRadius + kCollisionEpsilon);
                        }
                    } else if (axis == 1) {
                        if (!overlapsOnAxis(minCorner.x, maxCorner.x, blockMin.x, blockMax.x) ||
                            !overlapsOnAxis(minCorner.z, maxCorner.z, blockMin.z, blockMax.z)) {
                            continue;
                        }
                        if (delta > 0.0f) {
                            resolvedAxis =
                                std::min(resolvedAxis, blockMin.y - bodyHeight - kCollisionEpsilon);
                        } else {
                            resolvedAxis =
                                std::max(resolvedAxis, blockMax.y + kCollisionEpsilon);
                        }
                    } else {
                        if (!overlapsOnAxis(minCorner.x, maxCorner.x, blockMin.x, blockMax.x) ||
                            !overlapsOnAxis(minCorner.y, maxCorner.y, blockMin.y, blockMax.y)) {
                            continue;
                        }
                        if (delta > 0.0f) {
                            resolvedAxis =
                                std::min(resolvedAxis, blockMin.z - playerRadius - kCollisionEpsilon);
                        } else {
                            resolvedAxis =
                                std::max(resolvedAxis, blockMax.z + playerRadius + kCollisionEpsilon);
                        }
                    }
                }
            }
        }

        collided = std::abs(resolvedAxis - candidateFeet[axis]) > kCollisionEpsilon;
        return std::pair<float, bool>(resolvedAxis, collided);
    };

    const auto [resolvedX, collidedX] = sweepAxis(0, velocity.x * dt);
    glm::vec3 candidateFeet = feetPosition;
    candidateFeet.x = resolvedX;
    const bool blockSneakX =
        startedGrounded && crouching && !hasSupportBelow(world, candidateFeet, 0.0f, false);
    if (collidedX || blockSneakX) {
        velocity.x = 0.0f;
    } else {
        feetPosition.x = resolvedX;
    }

    const auto [resolvedZ, collidedZ] = sweepAxis(2, velocity.z * dt);
    candidateFeet = feetPosition;
    candidateFeet.z = resolvedZ;
    const bool blockSneakZ =
        startedGrounded && crouching && !hasSupportBelow(world, candidateFeet, 0.0f, false);
    if (collidedZ || blockSneakZ) {
        velocity.z = 0.0f;
    } else {
        feetPosition.z = resolvedZ;
    }

    grounded = false;
    const auto [resolvedY, collidedY] = sweepAxis(1, velocity.y * dt);
    if (collidedY) {
        if (velocity.y < 0.0f) {
            grounded = true;
        }
        velocity.y = 0.0f;
    }
    feetPosition.y = resolvedY;

    resolveBodyPenetration(world, feetPosition, bodyHeight);
    if (!grounded && velocity.y <= 0.05f) {
        grounded = hasSupportBelow(world, feetPosition, 0.0f, false);
    }
    if (grounded && velocity.y < 0.0f) {
        velocity.y = 0.0f;
    }

    setFeetPosition(feetPosition);
}
