#pragma once

#include <array>

#include <glm/glm.hpp>

namespace ENGINE::Visibility {

class Frustum {
public:
    static Frustum fromViewProjection(const glm::mat4& viewProjection);

    bool intersectsAabb(const glm::vec3& minCorner,
                        const glm::vec3& maxCorner) const;

    const std::array<glm::vec4, 6>& planes() const { return planes_; }

private:
    std::array<glm::vec4, 6> planes_{};
};

} // namespace ENGINE::Visibility
