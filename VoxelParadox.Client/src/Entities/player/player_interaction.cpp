#include "player.hpp"

#include "audio/game_audio_controller.hpp"

// Player interaction:
// - raycast target selection
// - block breaking/placing
// - item drops
// - portal action hotkeys

void Player::handleBlockInteraction(WorldStack& worldStack, float dt) {
    FractalWorld* world = worldStack.currentWorld();
    const BlockType targetType =
        (world && hasTarget) ? world->getBlock(targetBlock) : BlockType::AIR;
    const bool portalActionTarget =
        hasTarget &&
        !usesCustomBlockModel(targetType) &&
        (targetType == BlockType::PORTAL || isSolid(targetType));

    if (Input::keyPressed(GLFW_KEY_V) && portalActionTarget) {
        if (tryPrepareNestedWorld(worldStack, targetBlock)) {
            beginNestedPreviewFadeIn(targetBlock, targetNormal);
        }
    }

    if (Input::mouseDown(GLFW_MOUSE_BUTTON_LEFT) && hasTarget) {
        updateBlockBreaking(worldStack, dt);
    } else {
        resetBlockBreaking();
    }

    if (Input::mousePressed(GLFW_MOUSE_BUTTON_RIGHT) && hasTarget) {
        resetBlockBreaking();
        placeBlockAtTarget(worldStack);
    }

    if (Input::keyPressed(GLFW_KEY_F) && portalActionTarget) {
        resetBlockBreaking();
        beginNestedEntryTransition(worldStack);
    }

    if (Input::keyPressed(GLFW_KEY_R)) {
        resetBlockBreaking();
        beginAscendTransition(worldStack);
    }
}

void Player::updateBlockBreaking(WorldStack& worldStack, float dt) {
    FractalWorld* world = worldStack.currentWorld();
    if (!world || !hasTarget) {
        resetBlockBreaking();
        return;
    }

    const BlockType targetType = world->getBlock(targetBlock);
    if (targetType == BlockType::AIR) {
        resetBlockBreaking();
        return;
    }

    const bool changedTarget =
        !isBreakingBlock || breakingBlock != targetBlock || breakingBlockType != targetType;
    if (changedTarget) {
        isBreakingBlock = true;
        breakingBlock = targetBlock;
        breakingBlockType = targetType;
        breakingTimer = 0.0f;
        breakingProgress = 0.0f;
        breakingHitCooldown = kBreakHitRepeatInterval;
        if (audioController) {
            audioController->onBlockHit(targetType, targetBlock, true);
        }
    }

    const float breakTime = getBreakTimeSeconds(targetType);
    if (breakTime <= 0.0f) {
        breakTargetBlock(worldStack);
        resetBlockBreaking();
        return;
    }

    breakingTimer = glm::min(breakingTimer + dt, breakTime);
    breakingProgress = glm::clamp(breakingTimer / breakTime, 0.0f, 1.0f);
    breakingHitCooldown -= dt;
    if (audioController && breakingHitCooldown <= 0.0f &&
        breakingTimer < breakTime) {
        audioController->onBlockHit(targetType, targetBlock, false);
        breakingHitCooldown = kBreakHitRepeatInterval;
    }

    if (breakingTimer >= breakTime) {
        breakTargetBlock(worldStack);
        resetBlockBreaking();
    }
}

void Player::breakTargetBlock(WorldStack& worldStack) {
    FractalWorld* world = worldStack.currentWorld();
    if (!world) {
        return;
    }

    const BlockType brokenType = world->getBlock(targetBlock);
    if (brokenType == BlockType::AIR) {
        resetBlockBreaking();
        return;
    }
    const InventoryItem harvestTool = hotbar.getSelectedItem();

    if (brokenType == BlockType::PORTAL) {
        if (worldStack.deleteUniverseAtPortal(targetBlock)) {
            if (audioController) {
                audioController->onBlockBroken(brokenType, targetBlock);
            }
            if (shouldDropBlockItemForTool(harvestTool, brokenType)) {
                world->spawnDroppedItem(targetBlock, makeInventoryBlock(brokenType),
                                        camera.getForward() * kDroppedItemThrowSpeed);
            }
            clearTargetSelection();
            clearNestedPreview();
            return;
        }
    }

    world->setBlock(targetBlock, BlockType::AIR);
    if (audioController) {
        audioController->onBlockBroken(brokenType, targetBlock);
    }
    if (shouldDropBlockItemForTool(harvestTool, brokenType)) {
        world->spawnDroppedItem(targetBlock, makeInventoryBlock(brokenType),
                                camera.getForward() * kDroppedItemThrowSpeed);
    }

    const glm::ivec3 supportedBlockPos = targetBlock + glm::ivec3(0, 1, 0);
    if (world->getBlock(supportedBlockPos) == BlockType::MEMBRANE_WIRE) {
        world->setBlock(supportedBlockPos, BlockType::AIR);
        if (audioController) {
            audioController->onBlockBroken(BlockType::MEMBRANE_WIRE, supportedBlockPos);
        }
        world->spawnDroppedItem(supportedBlockPos, makeInventoryBlock(BlockType::MEMBRANE_WIRE),
                                camera.getForward() * (kDroppedItemThrowSpeed * 0.35f));
    }

    resetBlockBreaking();
}

void Player::placeBlockAtTarget(WorldStack& worldStack) {
    FractalWorld* world = worldStack.currentWorld();
    if (!world) {
        return;
    }

    const InventoryItem& selectedItem = hotbar.getSelectedItem();
    if (!isPlaceableInventoryItem(selectedItem) ||
        hotbar.getSelectedCount() <= 0) {
        return;
    }

    const glm::ivec3 placePos = targetBlock + targetNormal;
    const BlockType existingType = world->getBlock(placePos);
    if (!isReplaceableBlock(existingType)) {
        return;
    }

    if (selectedItem.blockType == BlockType::MEMBRANE_WIRE) {
        if (targetNormal != glm::ivec3(0, 1, 0)) {
            return;
        }

        const BlockType supportType = world->getBlock(placePos + glm::ivec3(0, -1, 0));
        if (!canSupportTopPlacedBlock(supportType, selectedItem.blockType)) {
            return;
        }
    }

    if (existingType == BlockType::MEMBRANE_WIRE &&
        selectedItem.blockType != BlockType::MEMBRANE_WIRE) {
        world->spawnDroppedItem(placePos, makeInventoryBlock(existingType),
                                camera.getForward() * (kDroppedItemThrowSpeed * 0.35f));
        if (audioController) {
            audioController->onBlockBroken(existingType, placePos);
        }
    }

    world->setBlock(placePos, selectedItem.blockType);
    hotbar.consumeSelected(1);
    if (audioController) {
        audioController->onBlockPlaced(selectedItem.blockType, placePos);
    }
}

void Player::dropSelectedItem(WorldStack& worldStack) {
    FractalWorld* world = worldStack.currentWorld();
    if (!world || !hotbar.hasSelectedItem() || hotbar.getSelectedCount() <= 0) {
        return;
    }

    const InventoryItem droppedItem = hotbar.getSelectedItem();
    const glm::vec3 throwDirection = glm::normalize(camera.getForward());
    const glm::vec3 spawnPosition =
        camera.position + throwDirection * kDroppedItemSpawnDistance;
    const glm::vec3 initialVelocity = throwDirection * kDroppedItemThrowSpeed;

    if (!hotbar.consumeSelected(1)) {
        return;
    }

    world->spawnDroppedItemAtPosition(spawnPosition, droppedItem, initialVelocity,
                                      kDroppedItemPickupDelay);
}

void Player::spawnEnemyAtTarget(WorldStack& worldStack, EnemyType type) {
    FractalWorld* world = worldStack.currentWorld();
    if (!world || !hasTarget || targetNormal != glm::ivec3(0, 1, 0)) {
        return;
    }

    const glm::vec3 spawnPosition =
        glm::vec3(targetBlock) + glm::vec3(0.5f, 1.0f, 0.5f);
    const glm::vec3 forward = glm::normalize(camera.getForward());
    const float yawDegrees =
        glm::degrees(std::atan2(-forward.x, -forward.z));
    world->spawnEnemy(type, spawnPosition, yawDegrees);
}

void Player::doRaycast(FractalWorld* world) {
    // DDA stepping finds the first solid block and the face that was hit.
    clearTargetOnly();

    const glm::vec3 origin = camera.position;
    const glm::vec3 dir = camera.getForward();

    glm::ivec3 current = glm::ivec3(glm::floor(origin));
    glm::ivec3 step(0);
    glm::vec3 tMax(0.0f);
    glm::vec3 tDelta(0.0f);

    for (int axis = 0; axis < 3; axis++) {
        if (dir[axis] > 0.0f) {
            step[axis] = 1;
            tMax[axis] = (std::floor(origin[axis]) + 1.0f - origin[axis]) / dir[axis];
            tDelta[axis] = 1.0f / dir[axis];
        } else if (dir[axis] < 0.0f) {
            step[axis] = -1;
            tMax[axis] = (origin[axis] - std::floor(origin[axis])) / (-dir[axis]);
            tDelta[axis] = 1.0f / (-dir[axis]);
        } else {
            step[axis] = 0;
            tMax[axis] = 1e30f;
            tDelta[axis] = 1e30f;
        }
    }

    const int maxSteps = static_cast<int>(breakRange / 0.5f);
    for (int i = 0; i < maxSteps; i++) {
        int axis = 0;
        if (tMax.x < tMax.y) {
            axis = tMax.x < tMax.z ? 0 : 2;
        } else {
            axis = tMax.y < tMax.z ? 1 : 2;
        }

        current[axis] += step[axis];
        const float t = tMax[axis];
        tMax[axis] += tDelta[axis];

        if (t > breakRange) {
            break;
        }

        if (!canTargetBlock(world->getBlock(current))) {
            continue;
        }

        hasTarget = true;
        targetBlock = current;
        targetNormal = glm::ivec3(0);
        targetNormal[axis] = -step[axis];
        return;
    }
}
