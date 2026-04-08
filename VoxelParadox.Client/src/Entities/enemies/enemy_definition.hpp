#pragma once

#include <string>
#include <vector>

#include "enemy_types.hpp"
#include "entities/fbx_entity_model.hpp"

// Shared enemy definition built once from content assets and reused by spawn/render.
struct EnemyDefinition {
    EnemyType type = EnemyType::Guy;
    EntityModelAsset modelAsset{};
    EnemyBoxCollider collider{};
    std::vector<EnemyModule> defaultModules{};
};

// Returns the cached definition for an enemy type.
const EnemyDefinition* getEnemyDefinition(EnemyType type,
                                          std::string* outError = nullptr);

// Builds one runtime enemy instance from the cached definition.
bool createWorldEnemyInstance(EnemyType type, const glm::vec3& position,
                              float yawDegrees, WorldEnemy& outEnemy,
                              std::string& outError);
