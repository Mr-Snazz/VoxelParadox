#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "enemies/enemy_types.hpp"

// GPU-ready vertex for imported textured entity meshes.
struct EntityModelVertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec2 uv{0.0f};
};

// One named part from the imported FBX model.
struct EntityModelPartAsset {
    std::string name{};
    std::vector<EntityModelVertex> vertices{};
    glm::mat4 nodeTransform{1.0f};
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};

    bool valid() const {
        return !vertices.empty();
    }
};

// Static model data shared by all instances of a given entity type.
struct EntityModelAsset {
    EnemyType type = EnemyType::Guy;
    std::vector<EntityModelPartAsset> parts{};
    std::string texturePath{};
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
    float yawOffsetDegrees = 0.0f;

    bool valid() const {
        return !parts.empty() && !texturePath.empty();
    }
};

// Loads an FBX model and preserves each authored sub-model plus its node transform.
bool loadFbxEntityModel(const std::filesystem::path& modelAssetPath,
                        EnemyType enemyType, EntityModelAsset& outModel,
                        std::string& outError);
