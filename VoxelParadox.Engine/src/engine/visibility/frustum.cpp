#include "frustum.hpp"

namespace ENGINE::Visibility {

Frustum Frustum::fromViewProjection(const glm::mat4& viewProjection) {
    const glm::vec4 row0(viewProjection[0][0], viewProjection[1][0],
                         viewProjection[2][0], viewProjection[3][0]);
    const glm::vec4 row1(viewProjection[0][1], viewProjection[1][1],
                         viewProjection[2][1], viewProjection[3][1]);
    const glm::vec4 row2(viewProjection[0][2], viewProjection[1][2],
                         viewProjection[2][2], viewProjection[3][2]);
    const glm::vec4 row3(viewProjection[0][3], viewProjection[1][3],
                         viewProjection[2][3], viewProjection[3][3]);

    Frustum frustum;
    frustum.planes_[0] = row3 + row0;
    frustum.planes_[1] = row3 - row0;
    frustum.planes_[2] = row3 + row1;
    frustum.planes_[3] = row3 - row1;
    frustum.planes_[4] = row3 + row2;
    frustum.planes_[5] = row3 - row2;

    for (glm::vec4& plane : frustum.planes_) {
        const float length = glm::length(glm::vec3(plane));
        if (length > 1e-5f) {
            plane /= length;
        }
    }

    return frustum;
}

bool Frustum::intersectsAabb(const glm::vec3& minCorner,
                             const glm::vec3& maxCorner) const {
    for (const glm::vec4& plane : planes_) {
        const glm::vec3 positiveVertex(
            plane.x >= 0.0f ? maxCorner.x : minCorner.x,
            plane.y >= 0.0f ? maxCorner.y : minCorner.y,
            plane.z >= 0.0f ? maxCorner.z : minCorner.z);

        if (glm::dot(glm::vec3(plane), positiveVertex) + plane.w < 0.0f) {
            return false;
        }
    }

    return true;
}

} // namespace ENGINE::Visibility
