// Arquivo: Engine/src/engine/camera.h
// Papel: declara um componente do núcleo compartilhado do engine.
// Fluxo: apoia bootstrap, timing, input, câmera ou render base do projeto.
// Dependências principais: GLFW, GLM e utilitários do engine.
#pragma once

// Arquivo: Engine/src/engine/camera.h
// Papel: declara a cÃ¢mera livre usada pelo runtime, pelo preview de portal e pelo BiomeMaker.
// Fluxo: recebe deltas de mouse, mantÃ©m a orientaÃ§Ã£o em quaternion e expÃµe matrizes de view,
// projeÃ§Ã£o e projeÃ§Ã£o para tela para HUDs e overlays.
// DependÃªncias principais: GLM para vetores, matrizes e quaternions.

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera {
public:
    glm::vec3 position{0, 0, 0};
    glm::quat orientation{1, 0, 0, 0}; // identity quaternion
    float baseFov = 80.0f;
    float nearPlane = 0.05f, farPlane = 300.0f;
    float sensitivity = 0.002f;

    // Disorientation
    float rollDriftPhase = 0.0f;
    float rollDriftSpeed = 0.12f;   // radians/sec base speed
    float rollDriftAmount = 0.08f;  // max roll per cycle
    float fovPulsePhase = 0.0f;
    float fovPulseAmount = 3.0f;    // degrees of FOV pulse
    float visualRollRadians = 0.0f; // transient render-only roll used by feedback effects
    glm::vec3 visualLocalOffset{0.0f}; // render-only local camera offset for bob and sway

    void rotate(float dx, float dy);
    void applyDisorientation(float dt, int depth);
    float getCurrentFov() const;
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspect) const;

    // Unity-style helper: project a 3D point into 2D screen space (pixels, top-left origin).
    // Returns false if the point is behind the camera or screen size is invalid.
    bool WorldToScreenPoint(const glm::vec3& worldPos, int screenWidth, int screenHeight,
                            glm::vec2& outScreen, float* outNdcDepth = nullptr) const;

    glm::vec3 getForward() const { return orientation * glm::vec3(0, 0, -1); }
    glm::vec3 getUp()      const { return orientation * glm::vec3(0, 1, 0); }
    glm::vec3 getRight()   const { return orientation * glm::vec3(1, 0, 0); }

    void lookAt(const glm::vec3& target, const glm::vec3& worldUp = glm::vec3(0, 1, 0));
};
