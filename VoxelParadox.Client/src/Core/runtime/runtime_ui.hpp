#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "engine/engine.hpp"

struct GLFWwindow;
struct GameSettings;

class GameAudioController;
class Player;
class Renderer;
class WorldStack;
class hudPortalInfo;

namespace RuntimeUI {

enum class SettingsMenuTab : std::uint8_t {
  General = 0,
  Sound,
};

struct RuntimeUiState {
  bool settingsMenuOpen = false;
  bool settingsDiscardConfirmOpen = false;
  bool hudRebuildRequested = false;
  bool debugTextVisible = false;
  SettingsMenuTab activeSettingsTab = SettingsMenuTab::General;
  ENGINE::VIEWPORTMODE lastNonFullscreenMode = ENGINE::VIEWPORTMODE::BORDERLESS;
};

glm::ivec2 appliedViewportSizeForSettings(const GameSettings& settings);
void saveGameSettings(const GameSettings& settings);

void syncHudMenuState(RuntimeUiState& uiState);
void syncDebugHudState(const RuntimeUiState& uiState,
                       const GameSettings& settings);
void syncCursorVisibility(const Player& player);

hudPortalInfo* setupHUD(Player& player, WorldStack& worldStack, Renderer& renderer,
                        GameAudioController& audioController,
                        GLFWwindow* window, GameSettings& appliedSettings,
                        GameSettings& pendingSettings,
                        const std::vector<std::string>& availableFonts,
                        const std::vector<glm::ivec2>& availableResolutions,
                        RuntimeUiState& uiState);

void handleGlobalShortcuts(hudPortalInfo* portalInfo, WorldStack& worldStack,
                           Player& player, GameAudioController& audioController,
                           RuntimeUiState& uiState,
                           GameSettings& appliedSettings,
                           GameSettings& pendingSettings);

bool shouldRenderDeveloperUi();
void drawDeveloperUi(const WorldStack& worldStack, const Player& player);

} // namespace RuntimeUI
