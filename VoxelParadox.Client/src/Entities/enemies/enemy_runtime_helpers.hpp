#pragma once

#include <algorithm>
#include <cctype>
#include <string>

#include <glm/gtc/matrix_transform.hpp>

#include "enemy_definition.hpp"

namespace EnemyRuntime {

inline std::string lowercaseCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

inline bool partNameMatches(const std::string& partName,
                            const std::string& targetName) {
    const std::string normalizedPart = lowercaseCopy(partName);
    const std::string normalizedTarget = lowercaseCopy(targetName);
    return normalizedPart == normalizedTarget ||
           normalizedPart.find(normalizedTarget) != std::string::npos ||
           normalizedTarget.find(normalizedPart) != std::string::npos;
}

inline const EntityModelPartAsset* findModelPart(const EntityModelAsset& model,
                                                 const std::string& targetName) {
    for (const EntityModelPartAsset& part : model.parts) {
        if (partNameMatches(part.name, targetName)) {
            return &part;
        }
    }
    return nullptr;
}

inline const EnemyLookAtPlayerCameraModule* findLookAtPlayerCameraModule(
    const WorldEnemy& enemy) {
    for (const EnemyModule& module : enemy.modules) {
        if (module.type == EnemyModuleType::LookAtPlayerCamera) {
            return &module.lookAtPlayerCamera;
        }
    }
    return nullptr;
}

inline EnemyLookAtPlayerCameraModule* findLookAtPlayerCameraModule(WorldEnemy& enemy) {
    for (EnemyModule& module : enemy.modules) {
        if (module.type == EnemyModuleType::LookAtPlayerCamera) {
            return &module.lookAtPlayerCamera;
        }
    }
    return nullptr;
}

inline const EnemyChargePlayerOnLookTriggerModule* findChargePlayerOnLookTriggerModule(
    const WorldEnemy& enemy) {
    for (const EnemyModule& module : enemy.modules) {
        if (module.type == EnemyModuleType::ChargePlayerOnLookTrigger) {
            return &module.chargePlayerOnLookTrigger;
        }
    }
    return nullptr;
}

inline EnemyChargePlayerOnLookTriggerModule* findChargePlayerOnLookTriggerModule(
    WorldEnemy& enemy) {
    for (EnemyModule& module : enemy.modules) {
        if (module.type == EnemyModuleType::ChargePlayerOnLookTrigger) {
            return &module.chargePlayerOnLookTrigger;
        }
    }
    return nullptr;
}

inline glm::mat4 buildEnemyModelMatrix(const WorldEnemy& enemy,
                                       const EnemyDefinition& definition) {
    glm::mat4 modelMatrix(1.0f);
    modelMatrix = glm::translate(modelMatrix, enemy.position);
    modelMatrix = glm::rotate(modelMatrix,
                              glm::radians(enemy.yawDegrees +
                                           definition.modelAsset.yawOffsetDegrees),
                              glm::vec3(0.0f, 1.0f, 0.0f));
    return modelMatrix;
}

inline glm::mat4 buildEnemyPartWorldMatrix(const WorldEnemy& enemy,
                                           const EnemyDefinition& definition,
                                           const EntityModelPartAsset& part) {
    glm::mat4 partWorldMatrix = buildEnemyModelMatrix(enemy, definition) * part.nodeTransform;
    const EnemyLookAtPlayerCameraModule* lookAtModule =
        findLookAtPlayerCameraModule(enemy);
    if (lookAtModule && partNameMatches(part.name, lookAtModule->partName)) {
        partWorldMatrix =
            partWorldMatrix *
            glm::rotate(glm::mat4(1.0f),
                        glm::radians(lookAtModule->currentYawDegrees +
                                     lookAtModule->localYawOffsetDegrees),
                        glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(glm::mat4(1.0f),
                        glm::radians(-lookAtModule->currentPitchDegrees +
                                     lookAtModule->localPitchOffsetDegrees),
                        glm::vec3(1.0f, 0.0f, 0.0f));
    }
    return partWorldMatrix;
}

inline bool tryBuildLookTriggerWorldTransform(
    const WorldEnemy& enemy, const EnemyDefinition& definition,
    const EnemyChargePlayerOnLookTriggerModule& module,
    glm::mat4& outTransform) {
    const EntityModelPartAsset* part =
        findModelPart(definition.modelAsset, module.partName);
    if (!part) {
        return false;
    }

    outTransform = buildEnemyPartWorldMatrix(enemy, definition, *part) *
                   glm::translate(glm::mat4(1.0f), module.triggerCenterOffset);
    return true;
}

}  // namespace EnemyRuntime
