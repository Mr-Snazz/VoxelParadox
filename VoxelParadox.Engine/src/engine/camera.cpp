// Arquivo: Engine/src/engine/camera.cpp
// Papel: implementa um componente do nÃºcleo compartilhado do engine.
// Fluxo: apoia bootstrap, timing, input, cÃ¢mera ou render base do projeto.
// DependÃªncias principais: GLFW, GLM e utilitÃ¡rios do engine.
#include "camera.hpp"

// Arquivo: Engine/src/engine/camera.cpp
// Papel: implementa a rotaÃƒÂ§ÃƒÂ£o livre da cÃƒÂ¢mera e utilitÃƒÂ¡rios de projeÃƒÂ§ÃƒÂ£o 3D.
// Fluxo: converte yaw/pitch a partir do forward atual, reconstrÃƒÂ³i a orientaÃƒÂ§ÃƒÂ£o com `lookAt`
// e gera view/projection para runtime e editor.
// DependÃƒÂªncias principais: GLM e trigonometria padrÃƒÂ£o.

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace {

glm::quat buildVisualRoll(const Camera& camera) {
    return glm::angleAxis(camera.visualRollRadians, glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::quat renderOrientation(const Camera& camera) {
    return glm::normalize(camera.orientation * buildVisualRoll(camera));
}

glm::vec3 renderPosition(const Camera& camera) {
    return camera.position + (camera.orientation * camera.visualLocalOffset);
}

}  // namespace

void Camera::rotate(float dx, float dy) {
    // A rotaÃƒÂ§ÃƒÂ£o recalcula yaw/pitch a partir do forward atual para evitar drift cumulativo estranho.
    const glm::vec3 forward = glm::normalize(getForward());
    float yaw = std::atan2(forward.x, -forward.z);
    float pitch = std::asin(glm::clamp(forward.y, -1.0f, 1.0f));

    yaw += dx * sensitivity;
    pitch = glm::clamp(pitch - dy * sensitivity,
                       glm::radians(-89.0f), glm::radians(89.0f));

    const float cosPitch = std::cos(pitch);
    const glm::vec3 targetForward(
        std::sin(yaw) * cosPitch,
        std::sin(pitch),
        -std::cos(yaw) * cosPitch);
    lookAt(position + targetForward, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::applyDisorientation(float dt, int depth) {
    (void)depth;

    // Roll drift - increases with depth
    // rollDriftPhase += dt * rollDriftSpeed * (1.0f + depth * 0.3f);
    // float rollAmount = rollDriftAmount * (1.0f + depth * 0.2f);
    // float rollDelta = std::sin(rollDriftPhase) * rollAmount * dt;
    // glm::quat qRoll = glm::angleAxis(rollDelta, getForward());
    // orientation = glm::normalize(qRoll * orientation);

    // FOV pulse
    fovPulsePhase += dt * 1.5f;
}

float Camera::getCurrentFov() const {
    const float pulse = std::sin(fovPulsePhase) * fovPulseAmount;
    return baseFov + pulse;
}

glm::mat4 Camera::getViewMatrix() const {
    const glm::mat4 rot =
        glm::mat4_cast(glm::conjugate(renderOrientation(*this)));
    const glm::mat4 trans = glm::translate(glm::mat4(1.0f), -renderPosition(*this));
    return rot * trans;
}

glm::mat4 Camera::getProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(getCurrentFov()), aspect, nearPlane, farPlane);
}

bool Camera::WorldToScreenPoint(const glm::vec3& worldPos, int screenWidth, int screenHeight,
                                glm::vec2& outScreen, float* outNdcDepth) const {
    // O HUD usa esse helper para ancorar elementos 2D em pontos do mundo sem depender do renderer.
    if (screenWidth <= 0 || screenHeight <= 0) {
        return false;
    }

    const float aspect = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
    const glm::mat4 vp = getProjectionMatrix(aspect) * getViewMatrix();
    const glm::vec4 clip = vp * glm::vec4(worldPos, 1.0f);
    if (clip.w <= 0.0001f) {
        return false;
    }

    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    outScreen.x = (ndc.x * 0.5f + 0.5f) * static_cast<float>(screenWidth);
    outScreen.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(screenHeight);
    if (outNdcDepth) {
        *outNdcDepth = ndc.z;
    }
    return true;
}

void Camera::lookAt(const glm::vec3& target, const glm::vec3& worldUp) {
    // O fallback de eixos evita degeneraÃƒÂ§ÃƒÂ£o quando forward e up ficam quase paralelos.
    glm::vec3 forward = target - position;
    const float forwardLen2 = glm::dot(forward, forward);
    if (forwardLen2 < 1e-6f) {
        return;
    }

    forward = glm::normalize(forward);

    glm::vec3 right = glm::cross(forward, worldUp);
    float rightLen2 = glm::dot(right, right);
    if (rightLen2 < 1e-6f) {
        right = glm::cross(forward, glm::vec3(0.0f, 0.0f, 1.0f));
        rightLen2 = glm::dot(right, right);
        if (rightLen2 < 1e-6f) {
            right = glm::cross(forward, glm::vec3(1.0f, 0.0f, 0.0f));
        }
    }

    right = glm::normalize(right);
    const glm::vec3 up = glm::normalize(glm::cross(right, forward));

    // Rotation from local axes (X,Y,Z) to world (right, up, back).
    const glm::mat3 rot(right, up, -forward);
    orientation = glm::normalize(glm::quat_cast(rot));
}
