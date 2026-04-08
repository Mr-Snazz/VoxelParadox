#include "enemy_definition.hpp"

#include <algorithm>

#include "client_assets.hpp"
#include "enemy_runtime_helpers.hpp"

namespace {

struct CachedEnemyDefinition {
    bool initialized = false;
    bool valid = false;
    EnemyDefinition definition{};
    std::string error{};
};

glm::vec3 safeHalfExtents(const glm::vec3& boundsMin, const glm::vec3& boundsMax) {
    const glm::vec3 size = glm::max(boundsMax - boundsMin, glm::vec3(0.05f));
    return size * 0.5f;
}

bool buildGuyDefinition(EnemyDefinition& outDefinition, std::string& outError) {
    outDefinition = EnemyDefinition{};
    outDefinition.type = EnemyType::Guy;

    if (!loadFbxEntityModel(ClientAssets::kGuyModelAsset, EnemyType::Guy,
                            outDefinition.modelAsset, outError)) {
        return false;
    }

    outDefinition.collider.centerOffset =
        (outDefinition.modelAsset.boundsMin + outDefinition.modelAsset.boundsMax) * 0.5f;
    outDefinition.collider.halfExtents =
        safeHalfExtents(outDefinition.modelAsset.boundsMin,
                        outDefinition.modelAsset.boundsMax);

    EnemyModule touchDamageModule;
    touchDamageModule.type = EnemyModuleType::DamagePlayerOnTouch;
    touchDamageModule.damagePlayerOnTouch.damagePoints = 3;
    outDefinition.defaultModules.push_back(touchDamageModule);

    EnemyModule lookAtPlayerModule;
    lookAtPlayerModule.type = EnemyModuleType::LookAtPlayerCamera;
    lookAtPlayerModule.lookAtPlayerCamera.partName = "head";
    lookAtPlayerModule.lookAtPlayerCamera.maxPitchDegrees = 55.0f;
    lookAtPlayerModule.lookAtPlayerCamera.localYawOffsetDegrees = 180.0f;
    lookAtPlayerModule.lookAtPlayerCamera.localPitchOffsetDegrees = -10.2f;
    outDefinition.defaultModules.push_back(lookAtPlayerModule);

    const EntityModelPartAsset* headPart =
        EnemyRuntime::findModelPart(outDefinition.modelAsset, "head");
    if (!headPart) {
        outError = "Guy FBX is missing the 'head' part required by the head trigger module.";
        return false;
    }

    EnemyModule chargeOnLookTriggerModule;
    chargeOnLookTriggerModule.type = EnemyModuleType::ChargePlayerOnLookTrigger;
    chargeOnLookTriggerModule.chargePlayerOnLookTrigger.partName = "head";
    chargeOnLookTriggerModule.chargePlayerOnLookTrigger.triggerCenterOffset =
        (headPart->boundsMin + headPart->boundsMax) * 0.5f;
    chargeOnLookTriggerModule.chargePlayerOnLookTrigger.triggerHalfExtents =
        safeHalfExtents(headPart->boundsMin, headPart->boundsMax);
    chargeOnLookTriggerModule.chargePlayerOnLookTrigger.triggerSizeMultiplier = 2.0f;
    chargeOnLookTriggerModule.chargePlayerOnLookTrigger.activationMaxDistance = 20.0f;
    chargeOnLookTriggerModule.chargePlayerOnLookTrigger.chargeSpeed = 60.0f;
    chargeOnLookTriggerModule.chargePlayerOnLookTrigger.targetYOffset = -2.5f;
    chargeOnLookTriggerModule.chargePlayerOnLookTrigger.damagePoints = 5;
    outDefinition.defaultModules.push_back(chargeOnLookTriggerModule);

    return true;
}

CachedEnemyDefinition& cacheForEnemy(EnemyType type) {
    static CachedEnemyDefinition guyCache{};

    switch (type) {
        case EnemyType::Guy:
            return guyCache;
    }

    return guyCache;
}

bool initializeEnemyDefinition(EnemyType type, CachedEnemyDefinition& cache) {
    cache.initialized = true;
    cache.valid = false;
    cache.definition = EnemyDefinition{};
    cache.error.clear();

    switch (type) {
        case EnemyType::Guy:
            cache.valid = buildGuyDefinition(cache.definition, cache.error);
            return cache.valid;
    }

    cache.error = "Unsupported enemy type.";
    return false;
}

}  // namespace

const EnemyDefinition* getEnemyDefinition(EnemyType type, std::string* outError) {
    CachedEnemyDefinition& cache = cacheForEnemy(type);
    if (!cache.initialized) {
        initializeEnemyDefinition(type, cache);
    }

    if (!cache.valid) {
        if (outError) {
            *outError = cache.error;
        }
        return nullptr;
    }

    return &cache.definition;
}

bool createWorldEnemyInstance(EnemyType type, const glm::vec3& position,
                              float yawDegrees, WorldEnemy& outEnemy,
                              std::string& outError) {
    const EnemyDefinition* definition = getEnemyDefinition(type, &outError);
    if (!definition) {
        return false;
    }

    outEnemy = WorldEnemy{};
    outEnemy.type = type;
    outEnemy.position = position;
    outEnemy.yawDegrees = yawDegrees;
    outEnemy.collider = definition->collider;
    outEnemy.modules = definition->defaultModules;
    return true;
}
