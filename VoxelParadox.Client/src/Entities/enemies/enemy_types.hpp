#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

// Enemy ids used by gameplay, rendering, and spawn helpers.
enum class EnemyType {
    Guy = 1,
};

// Modules add reusable behavior to a base enemy instance.
enum class EnemyModuleType {
    DamagePlayerOnTouch = 1,
    LookAtPlayerCamera = 2,
    ChargePlayerOnLookTrigger = 3,
};

// Simple box collider used for enemy-vs-player touch checks.
struct EnemyBoxCollider {
    glm::vec3 centerOffset{0.0f};
    glm::vec3 halfExtents{0.5f};
};

// Touching the enemy removes life from the player once per contact.
struct EnemyDamagePlayerOnTouchModule {
    int damagePoints = 3;
    bool touchingPlayerLastFrame = false;
};

// Rotates one named model part so it faces the player's camera.
struct EnemyLookAtPlayerCameraModule {
    std::string partName = "head";
    float currentYawDegrees = 0.0f;
    float currentPitchDegrees = 0.0f;
    float maxPitchDegrees = 60.0f;
    float localYawOffsetDegrees = 0.0f;
    float localPitchOffsetDegrees = 0.0f;
};

// A named trigger box can live on any authored mesh part.
// If the player looks directly at it, the enemy charges the player and removes itself on hit.
struct EnemyChargePlayerOnLookTriggerModule {
    std::string partName = "head";
    glm::vec3 triggerCenterOffset{0.0f};
    glm::vec3 triggerHalfExtents{0.15f};
    float triggerSizeMultiplier = 1.0f;
    float activationMaxDistance = 20.0f;
    float chargeSpeed = 18.0f;
    float targetYOffset = 0.0f;
    int damagePoints = 5;
    bool activated = false;
};

// One enemy can carry multiple modules so future enemies can reuse behavior.
struct EnemyModule {
    EnemyModuleType type = EnemyModuleType::DamagePlayerOnTouch;
    EnemyDamagePlayerOnTouchModule damagePlayerOnTouch{};
    EnemyLookAtPlayerCameraModule lookAtPlayerCamera{};
    EnemyChargePlayerOnLookTriggerModule chargePlayerOnLookTrigger{};
};

// Runtime enemy state stored in the world.
struct WorldEnemy {
    EnemyType type = EnemyType::Guy;
    glm::vec3 position{0.0f};
    float yawDegrees = 0.0f;
    EnemyBoxCollider collider{};
    std::vector<EnemyModule> modules{};
    bool pendingDestroy = false;
};
