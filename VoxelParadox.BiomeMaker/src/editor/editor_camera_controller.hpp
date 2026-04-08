#pragma once

#include <algorithm>
#include <cmath>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

#include "engine/camera.hpp"
#include "engine/input.hpp"

namespace BiomeMaker {

class EditorCameraController {
public:
  void update(Camera& camera, float dt, bool viewportHovered) {
    ensureOrientationState(camera);

    if (viewportHovered) {
      const float scroll = Input::getScroll();
      if (std::abs(scroll) > 0.001f) {
        moveSpeed_ = std::clamp(moveSpeed_ + scroll * 2.0f, 2.0f, 80.0f);
      }
    }

    // Once RMB capture starts inside the viewport, keep it alive until release.
    const bool captureActive =
        Input::mouseDown(GLFW_MOUSE_BUTTON_RIGHT) &&
        (capturing_ || viewportHovered);
    if (captureActive != capturing_) {
      capturing_ = captureActive;
      Input::setCursorVisible(!capturing_);
    }

    if (!capturing_) {
      return;
    }

    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
    Input::getMouseDelta(mouseDeltaX, mouseDeltaY);
    yawRadians_ -= mouseDeltaX * camera.sensitivity;
    pitchRadians_ = std::clamp(pitchRadians_ - mouseDeltaY * camera.sensitivity,
                               -kMaxPitchRadians, kMaxPitchRadians);
    applyOrientation(camera);

    glm::vec3 movement(0.0f);
    if (Input::keyDown(GLFW_KEY_W)) {
      movement += camera.getForward();
    }
    if (Input::keyDown(GLFW_KEY_S)) {
      movement -= camera.getForward();
    }
    if (Input::keyDown(GLFW_KEY_D)) {
      movement += camera.getRight();
    }
    if (Input::keyDown(GLFW_KEY_A)) {
      movement -= camera.getRight();
    }
    if (Input::keyDown(GLFW_KEY_E)) {
      movement += glm::vec3(0.0f, 1.0f, 0.0f);
    }
    if (Input::keyDown(GLFW_KEY_Q)) {
      movement -= glm::vec3(0.0f, 1.0f, 0.0f);
    }

    if (glm::dot(movement, movement) > 1e-5f) {
      movement = glm::normalize(movement);
      float speed = moveSpeed_;
      if (Input::keyDown(GLFW_KEY_LEFT_SHIFT) || Input::keyDown(GLFW_KEY_RIGHT_SHIFT)) {
        speed *= boostMultiplier_;
      }
      camera.position += movement * speed * dt;
    }
  }

  void focusOnAnchor(Camera& camera, const glm::ivec3& anchorChunk) {
    const glm::vec3 anchorCenter =
        glm::vec3(anchorChunk * 16) + glm::vec3(8.0f, 8.0f, 8.0f);
    camera.position = anchorCenter + glm::vec3(36.0f, 28.0f, 36.0f);
    camera.lookAt(anchorCenter);
    syncFromCamera(camera);
  }

  float moveSpeed() const { return moveSpeed_; }
  bool isCapturing() const { return capturing_; }

private:
  static constexpr float kMaxPitchRadians =
      glm::half_pi<float>() - 0.02f;

  float moveSpeed_ = 18.0f;
  float boostMultiplier_ = 4.0f;
  bool capturing_ = false;
  bool orientationInitialized_ = false;
  float yawRadians_ = 0.0f;
  float pitchRadians_ = 0.0f;

  void ensureOrientationState(const Camera& camera) {
    if (!orientationInitialized_) {
      syncFromCamera(camera);
    }
  }

  void syncFromCamera(const Camera& camera) {
    const glm::vec3 forward = glm::normalize(camera.getForward());
    pitchRadians_ =
        std::asin(std::clamp(forward.y, -1.0f, 1.0f));
    yawRadians_ = std::atan2(forward.x, -forward.z);
    orientationInitialized_ = true;
  }

  void applyOrientation(Camera& camera) const {
    const glm::quat yaw =
        glm::angleAxis(yawRadians_, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 yawRight = yaw * glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::quat pitch = glm::angleAxis(pitchRadians_, yawRight);
    camera.orientation = glm::normalize(pitch * yaw);
  }
};

} // namespace BiomeMaker
