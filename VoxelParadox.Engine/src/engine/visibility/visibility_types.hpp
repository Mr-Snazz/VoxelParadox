#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "frustum.hpp"

namespace ENGINE::Visibility {

struct ClusterCoord {
    int x = 0;
    int y = 0;
    int z = 0;

    bool operator==(const ClusterCoord& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct ClusterCoordHash {
    std::size_t operator()(const ClusterCoord& coord) const {
        std::size_t hash = 0;
        hash ^= std::hash<int>()(coord.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<int>()(coord.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<int>()(coord.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        return hash;
    }
};

struct ClusterBounds {
    glm::vec3 minCorner{0.0f};
    glm::vec3 maxCorner{0.0f};
};

struct ChunkRenderCandidate {
    glm::ivec3 chunkCoord{0};
    glm::vec3 minCorner{0.0f};
    glm::vec3 maxCorner{0.0f};
    int vertexCount = 0;
    int solidVoxelCount = 0;
    const void* userData = nullptr;
};

struct ClusterSummary {
    ClusterCoord coord{};
    ClusterBounds bounds{};
    int solidVoxelCount = 0;
    int loadedChunkCount = 0;
    float solidVoxelRatio = 0.0f;
};

struct ClusterVisibilityContext {
    glm::vec3 cameraPos{0.0f};
    glm::mat4 viewProjection{1.0f};
    Frustum frustum{};
    glm::ivec3 clusterChunkDimensions{2, 2, 2};
    int chunkSize = 16;
    float occluderSolidityThreshold = 0.70f;
    bool enableFrustumCulling = true;
    bool enableOcclusionCulling = true;
    std::vector<ChunkRenderCandidate> chunkCandidates;
    std::unordered_map<ClusterCoord, ClusterSummary, ClusterCoordHash> clusterSummaries;
};

struct VisibleChunkList {
    std::vector<std::size_t> visibleChunkIndices;
    int candidateChunkCount = 0;
    int frustumCulledChunkCount = 0;
    int occlusionCulledChunkCount = 0;
    int visibleClusterCount = 0;
    int visibleChunkCount = 0;
    int totalSubmittedVertices = 0;
};

} // namespace ENGINE::Visibility
