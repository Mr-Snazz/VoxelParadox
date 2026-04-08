#pragma once

#include <array>
#include <cstdint>

#include <glm/glm.hpp>

namespace ENGINE::Meshing {

struct MeshVertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec4 color{1.0f};
    float ao = 1.0f;
    float material = 0.0f;
};

struct FaceMaterialDesc {
    glm::vec4 color{1.0f};
    float material = 0.0f;
    bool transparent = false;
};

struct AoSignature {
    std::array<std::uint8_t, 4> levels{};

    bool operator==(const AoSignature& other) const {
        return levels == other.levels;
    }
};

} // namespace ENGINE::Meshing
