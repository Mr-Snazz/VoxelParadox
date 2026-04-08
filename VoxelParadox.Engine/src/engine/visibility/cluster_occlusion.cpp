#include "cluster_occlusion.hpp"

#include <algorithm>

namespace ENGINE::Visibility {
namespace {

constexpr float kDepthBias = 0.005f;
constexpr float kScreenMargin = 0.005f;

struct ProjectedBounds {
    bool valid = false;
    glm::vec2 minNdc{0.0f};
    glm::vec2 maxNdc{0.0f};
    float nearestDepth = 1.0f;
    float farthestDepth = 1.0f;
};

struct ClusterGroup {
    ClusterSummary summary{};
    std::vector<std::size_t> chunkIndices;
    float cameraDistanceSquared = 0.0f;
};

int floorDivide(int value, int divisor) {
    if (divisor <= 0) {
        return 0;
    }
    if (value >= 0) {
        return value / divisor;
    }
    return -(((-value) + divisor - 1) / divisor);
}

ProjectedBounds projectBounds(const ClusterBounds& bounds, const glm::mat4& viewProjection) {
    const glm::vec3 corners[8] = {
        {bounds.minCorner.x, bounds.minCorner.y, bounds.minCorner.z},
        {bounds.maxCorner.x, bounds.minCorner.y, bounds.minCorner.z},
        {bounds.minCorner.x, bounds.maxCorner.y, bounds.minCorner.z},
        {bounds.maxCorner.x, bounds.maxCorner.y, bounds.minCorner.z},
        {bounds.minCorner.x, bounds.minCorner.y, bounds.maxCorner.z},
        {bounds.maxCorner.x, bounds.minCorner.y, bounds.maxCorner.z},
        {bounds.minCorner.x, bounds.maxCorner.y, bounds.maxCorner.z},
        {bounds.maxCorner.x, bounds.maxCorner.y, bounds.maxCorner.z},
    };

    ProjectedBounds result{};
    glm::vec2 rawMinNdc(1e9f);
    glm::vec2 rawMaxNdc(-1e9f);
    result.minNdc = glm::vec2(1.0f);
    result.maxNdc = glm::vec2(-1.0f);
    result.nearestDepth = 1.0f;
    result.farthestDepth = 0.0f;
    bool hasProjectedCorner = false;

    for (const glm::vec3& corner : corners) {
        const glm::vec4 clip = viewProjection * glm::vec4(corner, 1.0f);
        if (clip.w <= 1e-4f) {
            continue;
        }

        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        hasProjectedCorner = true;
        rawMinNdc = glm::min(rawMinNdc, glm::vec2(ndc));
        rawMaxNdc = glm::max(rawMaxNdc, glm::vec2(ndc));

        const glm::vec2 clampedNdc = glm::clamp(glm::vec2(ndc), glm::vec2(-1.0f),
                                                glm::vec2(1.0f));
        result.minNdc = glm::min(result.minNdc, clampedNdc);
        result.maxNdc = glm::max(result.maxNdc, clampedNdc);

        const float depth = glm::clamp((ndc.z * 0.5f) + 0.5f, 0.0f, 1.0f);
        result.nearestDepth = std::min(result.nearestDepth, depth);
        result.farthestDepth = std::max(result.farthestDepth, depth);
    }

    if (!hasProjectedCorner) {
        return {};
    }

    if (rawMaxNdc.x < -1.0f || rawMinNdc.x > 1.0f ||
        rawMaxNdc.y < -1.0f || rawMinNdc.y > 1.0f) {
        return {};
    }

    result.valid = true;
    return result;
}

bool projectedFullyContains(const ProjectedBounds& occluder,
                            const ProjectedBounds& candidate) {
    return occluder.minNdc.x <= candidate.minNdc.x - kScreenMargin &&
           occluder.minNdc.y <= candidate.minNdc.y - kScreenMargin &&
           occluder.maxNdc.x >= candidate.maxNdc.x + kScreenMargin &&
           occluder.maxNdc.y >= candidate.maxNdc.y + kScreenMargin;
}

ClusterSummary makeFallbackSummary(const ClusterCoord& coord, int chunkSize,
                                   const glm::ivec3& clusterChunkDimensions) {
    ClusterSummary summary{};
    summary.coord = coord;
    summary.bounds = computeClusterBounds(coord, chunkSize, clusterChunkDimensions);
    return summary;
}

float clusterDistanceSquared(const ClusterBounds& bounds, const glm::vec3& cameraPos) {
    const glm::vec3 center = (bounds.minCorner + bounds.maxCorner) * 0.5f;
    const glm::vec3 delta = center - cameraPos;
    return glm::dot(delta, delta);
}

} // namespace

ClusterCoord computeClusterCoord(const glm::ivec3& chunkCoord,
                                 const glm::ivec3& clusterChunkDimensions) {
    const glm::ivec3 safeDimensions(
        std::max(clusterChunkDimensions.x, 1),
        std::max(clusterChunkDimensions.y, 1),
        std::max(clusterChunkDimensions.z, 1));

    return {
        floorDivide(chunkCoord.x, safeDimensions.x),
        floorDivide(chunkCoord.y, safeDimensions.y),
        floorDivide(chunkCoord.z, safeDimensions.z),
    };
}

ClusterBounds computeClusterBounds(const ClusterCoord& clusterCoord, int chunkSize,
                                   const glm::ivec3& clusterChunkDimensions) {
    const glm::ivec3 safeDimensions(
        std::max(clusterChunkDimensions.x, 1),
        std::max(clusterChunkDimensions.y, 1),
        std::max(clusterChunkDimensions.z, 1));
    const glm::ivec3 chunkMin(clusterCoord.x * safeDimensions.x,
                              clusterCoord.y * safeDimensions.y,
                              clusterCoord.z * safeDimensions.z);
    const glm::vec3 minCorner =
        glm::vec3(chunkMin) * static_cast<float>(chunkSize);
    const glm::vec3 extent =
        glm::vec3(safeDimensions) * static_cast<float>(chunkSize);

    return {minCorner, minCorner + extent};
}

bool isClusterOccluded(const ClusterBounds& candidateBounds,
                       const glm::mat4& viewProjection,
                       const std::vector<ClusterSummary>& occluderSummaries) {
    const ProjectedBounds candidateProjection =
        projectBounds(candidateBounds, viewProjection);
    if (!candidateProjection.valid) {
        return false;
    }

    for (const ClusterSummary& occluder : occluderSummaries) {
        const ProjectedBounds occluderProjection =
            projectBounds(occluder.bounds, viewProjection);
        if (!occluderProjection.valid) {
            continue;
        }

        if (!projectedFullyContains(occluderProjection, candidateProjection)) {
            continue;
        }

        if (occluderProjection.farthestDepth <=
            candidateProjection.nearestDepth - kDepthBias) {
            return true;
        }
    }

    return false;
}

VisibleChunkList buildVisibleChunkList(const ClusterVisibilityContext& context) {
    VisibleChunkList result;
    result.candidateChunkCount =
        static_cast<int>(context.chunkCandidates.size());

    std::unordered_map<ClusterCoord, ClusterGroup, ClusterCoordHash> clusterGroups;
    clusterGroups.reserve(context.chunkCandidates.size());

    for (std::size_t index = 0; index < context.chunkCandidates.size(); ++index) {
        const ChunkRenderCandidate& candidate = context.chunkCandidates[index];
        if (candidate.vertexCount <= 0) {
            continue;
        }

        if (context.enableFrustumCulling &&
            !context.frustum.intersectsAabb(candidate.minCorner, candidate.maxCorner)) {
            result.frustumCulledChunkCount += 1;
            continue;
        }

        const ClusterCoord clusterCoord =
            computeClusterCoord(candidate.chunkCoord, context.clusterChunkDimensions);
        ClusterGroup& group = clusterGroups[clusterCoord];
        if (group.chunkIndices.empty()) {
            const auto summaryIt = context.clusterSummaries.find(clusterCoord);
            group.summary =
                summaryIt != context.clusterSummaries.end()
                    ? summaryIt->second
                    : makeFallbackSummary(clusterCoord, context.chunkSize,
                                          context.clusterChunkDimensions);
            group.cameraDistanceSquared =
                clusterDistanceSquared(group.summary.bounds, context.cameraPos);
        }

        group.chunkIndices.push_back(index);
    }

    std::vector<ClusterGroup*> orderedGroups;
    orderedGroups.reserve(clusterGroups.size());
    for (auto& entry : clusterGroups) {
        if (!entry.second.chunkIndices.empty()) {
            orderedGroups.push_back(&entry.second);
        }
    }

    std::sort(orderedGroups.begin(), orderedGroups.end(),
              [](const ClusterGroup* a, const ClusterGroup* b) {
                  return a->cameraDistanceSquared < b->cameraDistanceSquared;
              });

    std::vector<ClusterSummary> activeOccluders;
    activeOccluders.reserve(orderedGroups.size());

    for (const ClusterGroup* group : orderedGroups) {
        if (context.enableOcclusionCulling &&
            isClusterOccluded(group->summary.bounds, context.viewProjection,
                              activeOccluders)) {
            result.occlusionCulledChunkCount +=
                static_cast<int>(group->chunkIndices.size());
            continue;
        }

        result.visibleClusterCount += 1;
        for (const std::size_t chunkIndex : group->chunkIndices) {
            const ChunkRenderCandidate& candidate = context.chunkCandidates[chunkIndex];
            result.visibleChunkIndices.push_back(chunkIndex);
            result.visibleChunkCount += 1;
            result.totalSubmittedVertices += candidate.vertexCount;
        }

        if (context.enableOcclusionCulling &&
            group->summary.solidVoxelRatio >= context.occluderSolidityThreshold) {
            activeOccluders.push_back(group->summary);
        }
    }

    return result;
}

} // namespace ENGINE::Visibility
