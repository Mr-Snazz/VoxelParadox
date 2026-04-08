#pragma once

#include <vector>

#include "visibility_types.hpp"

namespace ENGINE::Visibility {

inline const glm::ivec3 kDefaultClusterChunkDimensions{2, 2, 2};
inline constexpr float kDefaultOccluderSolidityThreshold = 0.35f;

ClusterCoord computeClusterCoord(const glm::ivec3& chunkCoord,
                                 const glm::ivec3& clusterChunkDimensions =
                                     kDefaultClusterChunkDimensions);

ClusterBounds computeClusterBounds(const ClusterCoord& clusterCoord, int chunkSize,
                                   const glm::ivec3& clusterChunkDimensions =
                                       kDefaultClusterChunkDimensions);

bool isClusterOccluded(const ClusterBounds& candidateBounds,
                       const glm::mat4& viewProjection,
                       const std::vector<ClusterSummary>& occluderSummaries);

VisibleChunkList buildVisibleChunkList(const ClusterVisibilityContext& context);

} // namespace ENGINE::Visibility
