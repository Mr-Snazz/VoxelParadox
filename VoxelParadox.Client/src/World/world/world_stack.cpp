#include "world_stack.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>

#include "enemies/enemy_system.hpp"
#include "world_spawn_locator.hpp"

// WorldStack core:
// - active world lifecycle
// - descend/ascend traversal
// - render/update forwarding
// - nested preview world management

void WorldStack::setSaveWorldDirectory(const std::string& directory) {
    if (!directory.empty()) {
        saveDirectory = directory;
    }
}

const std::string& WorldStack::getSaveWorldDirectory() {
    return saveDirectory;
}

void WorldStack::cycleRenderDistancePreset() {
    stepRenderDistancePreset(1);
}

void WorldStack::stepRenderDistancePreset(int delta) {
    constexpr int presetCount = 4;
    int index = static_cast<int>(renderDistancePreset);
    index = (index + (delta % presetCount) + presetCount) % presetCount;
    setRenderDistancePreset(static_cast<RenderDistancePreset>(index));
}

void WorldStack::setRenderDistancePreset(RenderDistancePreset preset) {
    renderDistancePreset = preset;
    if (activeWorld) {
        applyRenderDistancePreset(*activeWorld);
    }
    if (previewWorld) {
        applyRenderDistancePreset(*previewWorld);
    }
}

WorldStack::RenderDistancePreset WorldStack::getRenderDistancePreset() const {
    return renderDistancePreset;
}

const char* WorldStack::currentRenderDistanceName() const {
    switch (renderDistancePreset) {
    case RenderDistancePreset::SHORT: return "Short";
    case RenderDistancePreset::NORMAL: return "Normal";
    case RenderDistancePreset::FAR: return "Far";
    case RenderDistancePreset::ULTRA: return "Ultra";
    }
    return "Normal";
}

void WorldStack::updateEnemies(Player& player, GameAudioController& audioController,
                               float dt) {
    if (!activeWorld) {
        return;
    }

    updateWorldEnemies(*activeWorld, player, audioController, dt);
}

int WorldStack::configuredRenderDistanceFor(const FractalWorld& world) const {
    const int base = world.defaultRenderDistance;
    switch (renderDistancePreset) {
    case RenderDistancePreset::SHORT: return std::max(2, base - 2);
    case RenderDistancePreset::NORMAL: return base;
    case RenderDistancePreset::FAR: return std::min(10, base + 2);
    case RenderDistancePreset::ULTRA: return std::min(10, base + 5);
    }
    return base;
}

void WorldStack::applyRenderDistancePreset(FractalWorld& world) const {
    world.renderDistance = configuredRenderDistanceFor(world);
}

void WorldStack::init(uint32_t rootSeed, const BiomeSelection& rootSelection,
                      std::shared_ptr<const VoxelGame::BiomePreset> rootPreset) {
    // Reinitializing also invalidates warm preview/shell state from older runs.
    stack.clear();
    globalCache.clear();
    resolvedSpawnCache.clear();
    previewWorld.reset();
    previewWorldSeed = 0;
    previewWorldSelection = defaultPresetSelection().selection;
    previewParentSeed = 0;
    returnWorldShell.reset();

    WorldLevel level;
    level.seed = rootSeed;
    level.biomeSelection = rootSelection;
    level.biomePreset = std::move(rootPreset);
    if (!level.biomePreset && !level.biomeSelection.presetId.empty()) {
        level.biomePreset =
            BiomeRegistry::instance().findPreset(level.biomeSelection.presetId);
    }
    level.returnPosition = glm::vec3(0.0f);
    level.portalBlock = glm::ivec3(0);

    stack.push_back(level);
    activeWorld = std::make_unique<FractalWorld>(currentDepth(), rootSeed,
                                                 level.biomeSelection,
                                                 level.biomePreset);
    applyRenderDistancePreset(*activeWorld);
    applyCacheToActive();
#if defined(VP_ENABLE_DEV_TOOLS)
    injectStartupStructure();
#endif
}

void WorldStack::shutdown() {
    saveActiveToCache();
    activeWorld.reset();
    previewWorld.reset();
    returnWorldShell.reset();
    stack.clear();
    globalCache.clear();
    resolvedSpawnCache.clear();
    previewWorldSeed = 0;
    previewWorldSelection = defaultPresetSelection().selection;
    previewParentSeed = 0;
}

bool WorldStack::descendInto(glm::ivec3 blockPos, glm::vec3 returnPos,
                             const glm::quat& returnOrientation,
                             glm::ivec3 portalNormal) {
    // Descend saves the current world, warms a return shell, then swaps active world.
    FractalWorld* current = currentWorld();
    if (!current) {
        return false;
    }

    uint32_t childSeed = deriveChildSeed(current->seed, blockPos);
    bool createdPortal = false;
    auto portalIt = current->portalBlocks.find(blockPos);
    if (portalIt == current->portalBlocks.end()) {
        current->portalBlocks[blockPos] = childSeed;
        createdPortal = true;
    } else {
        childSeed = portalIt->second;
    }
    const ResolvedBiomeSelection childBiome =
        ensurePortalBiomeSelection(*current, blockPos, childSeed, createdPortal);
    current->setBlock(blockPos, BlockType::PORTAL);

    saveActiveToCache();

    const glm::vec3 normal = glm::normalize(glm::vec3(portalNormal));
    const glm::vec3 portalFocus =
        glm::vec3(blockPos) + glm::vec3(0.5f) + normal * 0.6f;
    current->prepareReturnShell({returnPos, portalFocus}, 1, 2);
    returnWorldShell = std::move(activeWorld);

    WorldLevel level;
    level.seed = childSeed;
    level.biomeSelection = childBiome.selection;
    level.biomePreset = childBiome.preset;
    level.returnPosition = returnPos;
    level.returnOrientation = returnOrientation;
    level.portalBlock = blockPos;
    level.portalNormal = portalNormal;
    stack.push_back(level);

    const bool reusePreviewWorld = previewWorld && previewWorldSeed == childSeed &&
                                   previewWorldSelection == level.biomeSelection &&
                                   previewParentSeed == returnWorldShell->seed;
    if (reusePreviewWorld) {
        activeWorld = std::move(previewWorld);
        applyRenderDistancePreset(*activeWorld);
    } else {
        activeWorld = std::make_unique<FractalWorld>(currentDepth(), childSeed,
                                                     level.biomeSelection,
                                                     level.biomePreset);
        applyRenderDistancePreset(*activeWorld);
        applyCacheToActive();
    }
    previewWorld.reset();
    previewWorldSeed = 0;
    previewWorldSelection = defaultPresetSelection().selection;
    previewParentSeed = 0;

    std::cout << "[Dimension Portal] Descended into Depth " << currentDepth()
              << " | Universe ID (Seed): " << childSeed
              << " | Biome: " << level.biomeSelection.displayName << std::endl;

    return true;
}

bool WorldStack::canAscend() const {
    return stack.size() > 1;
}

bool WorldStack::ascend(glm::vec3& outPlayerPos, glm::quat& outPlayerOrientation,
                        glm::ivec3& outPortalBlock, glm::ivec3& outPortalNormal) {
    // Ascend promotes the child world to preview and restores a warm parent shell if possible.
    if (!canAscend()) {
        return false;
    }

    saveActiveToCache();

    const std::uint32_t childSeed = activeWorld ? activeWorld->seed : 0;
    const BiomeSelection childSelection =
        activeWorld ? activeWorld->biomeSelection
                    : defaultPresetSelection().selection;
    auto childWorld = std::move(activeWorld);

    outPlayerPos = stack.back().returnPosition;
    outPlayerOrientation = stack.back().returnOrientation;
    outPortalBlock = stack.back().portalBlock;
    outPortalNormal = stack.back().portalNormal;
    stack.pop_back();

    const uint32_t parentSeed = stack.back().seed;
    const BiomeSelection parentSelection = stack.back().biomeSelection;
    const bool hasWarmReturnShell =
        returnWorldShell && returnWorldShell->seed == parentSeed &&
        returnWorldShell->biomeSelection == parentSelection;
    if (hasWarmReturnShell) {
        activeWorld = std::move(returnWorldShell);
        applyRenderDistancePreset(*activeWorld);
    } else {
        activeWorld = std::make_unique<FractalWorld>(currentDepth(), parentSeed,
                                                     parentSelection,
                                                     stack.back().biomePreset);
        applyRenderDistancePreset(*activeWorld);
        applyCacheToActive();
    }

    previewWorld = std::move(childWorld);
    previewWorldSeed = childSeed;
    previewWorldSelection = childSelection;
    previewParentSeed = parentSeed;

    std::cout << "[Dimension Portal] Ascended to Depth " << currentDepth()
              << " | Universe ID (Seed): " << parentSeed
              << " | Biome: " << parentSelection.displayName << std::endl;

    return true;
}

void WorldStack::update(const glm::vec3& playerPos, const glm::vec3& viewForward, float dt) {
    (void)dt;
    if (activeWorld) {
        activeWorld->update(playerPos, viewForward);
    }
}

void WorldStack::markAllWorldMeshesDirty() {
    const auto markWorld = [](std::unique_ptr<FractalWorld>& world) {
        if (world) {
            world->markAllChunksDirty();
        }
    };

    markWorld(activeWorld);
    markWorld(previewWorld);
    markWorld(returnWorldShell);
}

void WorldStack::render(const glm::vec3& cameraPos, const glm::mat4& viewProjection) {
    if (activeWorld) {
        activeWorld->render(cameraPos, viewProjection);
    }
}

void WorldStack::render(const glm::vec3& renderCameraPos,
                        const glm::mat4& renderViewProjection,
                        const glm::vec3& cullingCameraPos,
                        const glm::mat4& cullingViewProjection) {
    if (activeWorld) {
        activeWorld->render(renderCameraPos, renderViewProjection, cullingCameraPos,
                            cullingViewProjection);
    }
}

BiomeSelection WorldStack::getResolvedPortalBiomeSelection(const FractalWorld& world,
                                                           const glm::ivec3& blockPos,
                                                           std::uint32_t childSeed) const {
    auto it = world.portalBiomeSelections.find(blockPos);
    if (it != world.portalBiomeSelections.end()) {
        return lookupPresetSelection(it->second).selection;
    }
    return pickPortalBiomeSelectionFromSeed(childSeed).selection;
}

bool WorldStack::ensureNestedWorldAtBlock(
    const glm::ivec3& blockPos, std::uint32_t* outChildSeed,
    BiomeSelection* outChildBiome,
    std::shared_ptr<const VoxelGame::BiomePreset>* outChildPreset) {
    FractalWorld* current = currentWorld();
    if (!current) {
        return false;
    }

    std::uint32_t childSeed = 0;
    auto it = current->portalBlocks.find(blockPos);
    bool createdPortal = false;
    if (it == current->portalBlocks.end()) {
        childSeed = deriveChildSeed(current->seed, blockPos);
        current->portalBlocks[blockPos] = childSeed;
        createdPortal = true;
        current->setBlock(blockPos, BlockType::PORTAL);
        saveActiveToCache();
    } else {
        childSeed = it->second;
        if (current->getBlock(blockPos) != BlockType::PORTAL) {
            current->setBlock(blockPos, BlockType::PORTAL);
        }
    }

    const ResolvedBiomeSelection childBiome =
        ensurePortalBiomeSelection(*current, blockPos, childSeed, createdPortal);
    if (outChildSeed) {
        *outChildSeed = childSeed;
    }
    if (outChildBiome) {
        *outChildBiome = childBiome.selection;
    }
    if (outChildPreset) {
        *outChildPreset = childBiome.preset;
    }
    return true;
}

glm::vec3 WorldStack::resolveSpawnPositionForWorld(FractalWorld& world,
                                                   const glm::vec3& spawnAnchor,
                                                   float playerRadius,
                                                   float bodyHeight,
                                                   float eyeHeight) {
    const std::string key = universeKey(world.seed, world.biomeSelection);
    const auto cached = resolvedSpawnCache.find(key);
    if (cached != resolvedSpawnCache.end()) {
        return cached->second;
    }

    const glm::vec3 resolved = findNearestGroundSpawnPosition(
        world, spawnAnchor, playerRadius, bodyHeight, eyeHeight);
    resolvedSpawnCache[key] = resolved;
    return resolved;
}

glm::vec3 WorldStack::resolveCurrentWorldSpawnPosition(const glm::vec3& spawnAnchor,
                                                       float playerRadius,
                                                       float bodyHeight,
                                                       float eyeHeight) {
    if (!activeWorld) {
        return spawnAnchor + glm::vec3(0.0f, eyeHeight, 0.0f);
    }

    return resolveSpawnPositionForWorld(*activeWorld, spawnAnchor, playerRadius,
                                        bodyHeight, eyeHeight);
}

FractalWorld* WorldStack::getOrCreateNestedPreviewWorld(const glm::ivec3& blockPos) {
    if (!activeWorld) {
        return nullptr;
    }

    std::uint32_t childSeed = 0;
    BiomeSelection childBiome = defaultPresetSelection().selection;
    std::shared_ptr<const VoxelGame::BiomePreset> childPreset;
    if (!ensureNestedWorldAtBlock(blockPos, &childSeed, &childBiome, &childPreset)) {
        return nullptr;
    }

    if (!previewWorld || previewWorldSeed != childSeed ||
        previewWorldSelection != childBiome ||
        previewParentSeed != activeWorld->seed) {
        previewWorld =
            std::make_unique<FractalWorld>(currentDepth() + 1, childSeed,
                                           childBiome, childPreset);
        applyRenderDistancePreset(*previewWorld);
        applyCacheToWorld(*previewWorld);
        previewWorldSeed = childSeed;
        previewWorldSelection = childBiome;
        previewParentSeed = activeWorld->seed;
    }

    return previewWorld.get();
}

void WorldStack::clearNestedPreviewWorld() {
    previewWorld.reset();
    previewWorldSeed = 0;
    previewWorldSelection = defaultPresetSelection().selection;
    previewParentSeed = 0;
}

void WorldStack::injectStartupStructure() {
    if (!activeWorld || currentDepth() != 0) {
        return;
    }

    const char* enabled = std::getenv(ClientAssets::kStartupStructureEnvVar);
    if (!enabled || enabled[0] != '1') {
        return;
    }

    const VoxStructureData* structure =
        loadVoxStructure(ClientAssets::kStartupStructureAsset);
    if (!structure) {
        std::cout << "[VOX] Failed to load "
                  << ClientAssets::kStartupStructureAsset << "\n";
        return;
    }

    const glm::ivec3 origin(10, -8, 10);
    stampVoxStructure(*activeWorld, *structure, origin);
    activeWorld->rebuildSparseEditIndex();
    std::cout << "[VOX] Spawned test structure from "
              << ClientAssets::kStartupStructureAsset << " at "
              << origin.x << ", " << origin.y << ", " << origin.z << std::endl;
}
