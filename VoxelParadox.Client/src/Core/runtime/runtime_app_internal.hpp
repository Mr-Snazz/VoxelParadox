// runtime_app_internal.hpp
// Shared helpers used by the split runtime app files.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "engine/bootstrap.hpp"
#include "runtime/game_settings.hpp"
#include "runtime/runtime_ui.hpp"
#include "world/biome.hpp"
#include "world/biome_preset.hpp"

struct GLFWwindow;

class GameAudioController;
class GameChat;
class Player;
class Renderer;
class WorldStack;
class FractalWorld;
class hudPortalInfo;

namespace RuntimeAppInternal {

struct RuntimeSettingsBundle {
  GameSettings applied{};
  GameSettings pending{};
  std::vector<std::string> availableFonts{};
  std::vector<glm::ivec2> availableResolutions{};
  RuntimeUI::RuntimeUiState uiState{};
};

struct RuntimeDebugFlags {
  bool wireframeMode = false;
  bool debugThirdPersonView = false;
};

struct RootWorldChunkCheck {
  int radiusChunks = 1;
  int expectedChunks = 0;
  int loadedChunks = 0;
  int generatedChunks = 0;
  bool passed = false;
};

RuntimeSettingsBundle loadRuntimeSettings(std::string* outLoadError);

void printBootstrapInfo(const char* message);
void printBootstrapSuccess(const char* message);
void printBootstrapError(const char* message);
void printBootstrapDetail(const char* label, const std::string& value);

bool loadRequiredBiomePresets(
    BiomeSelection& outRootSelection,
    std::shared_ptr<const VoxelGame::BiomePreset>& outRootPreset);
Bootstrap::Config makeBootstrapConfig(const GameSettings& settings);
bool prepareRootWorld(WorldStack& worldStack, uint32_t rootSeed,
                      const BiomeSelection& rootBiomeSelection,
                      std::shared_ptr<const VoxelGame::BiomePreset> rootPreset,
                      const glm::vec3& spawnPosition);

void runMainLoop(GLFWwindow* window, Renderer& renderer, WorldStack& worldStack,
                 Player& player, GameAudioController& audioController,
                 GameChat& gameChat, hudPortalInfo*& portalInfo,
                 RuntimeSettingsBundle& settingsBundle,
                 const BiomeSelection& rootBiomeSelection);

void shutdownGame(GLFWwindow*& window, Renderer& renderer, WorldStack* worldStack);
[[noreturn]] void terminateRuntimeProcess(int code);

}  // namespace RuntimeAppInternal
