// runtime_app_loop.cpp
// Per-frame runtime flow: input, gameplay, audio, rendering, and shutdown.

#include "runtime_app_internal.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

// External
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// Solution dependencies
#include "engine/engine.hpp"
#include "engine/input.hpp"
#include "path/app_paths.hpp"

// Client
#include "audio/game_audio_controller.hpp"
#include "client_defaults.hpp"
#include "debug/biome_debug_tools.hpp"
#include "player/player.hpp"
#include "render/hud/hud.hpp"
#include "render/hud/hud_portal_info.hpp"
#include "render/hud/hud_portal_tracker.hpp"
#include "render/renderer.hpp"
#include "runtime/game_chat.hpp"
#include "world/world_save_service.hpp"
#include "runtime/runtime_ui.hpp"
#include "ui/biome_teleport_window.hpp"
#include "ui/imgui_layer.hpp"
#include "world/biome_registry.hpp"
#include "world/world_stack.hpp"

namespace {

bool canCaptureGameplayScreenshot(const Player& player, const GameChat& gameChat,
                                  hudPortalInfo* portalInfo,
                                  hudPortalTracker* portalTracker) {
  return !ENGINE::ISPAUSED() && !Input::hasUiFocus() && !player.isInventoryOpen() &&
         !gameChat.isOpen() && player.transition == PlayerTransition::NONE &&
         (!portalInfo || !portalInfo->isEditing()) &&
         (!portalTracker || !portalTracker->isMenuOpen());
}

std::filesystem::path makeScreenshotPath() {
  std::filesystem::path screenshotDir = AppPaths::savesRoot() / "Screenshots";
  const std::string& saveDirectory = WorldStack::getSaveWorldDirectory();
  if (!saveDirectory.empty()) {
    const std::filesystem::path universesDirectory(saveDirectory);
    const std::filesystem::path worldDirectory = universesDirectory.parent_path();
    if (!worldDirectory.empty()) {
      screenshotDir = worldDirectory / "screenshots";
    }
  }
  std::error_code ec;
  std::filesystem::create_directories(screenshotDir, ec);
  if (ec) {
    return {};
  }

  const auto now = std::chrono::system_clock::now();
  const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
  std::tm localTime{};
  localtime_s(&localTime, &nowTime);

  std::ostringstream filename;
  filename << "screenshot_" << std::put_time(&localTime, "%Y%m%d_%H%M%S");

  const auto millis =
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) %
      1000;
  filename << "_" << std::setw(3) << std::setfill('0') << millis.count() << ".png";
  return screenshotDir / filename.str();
}

bool captureGameplayScreenshot(GLFWwindow* window) {
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  if (width <= 0 || height <= 0) {
    return false;
  }

  const std::filesystem::path screenshotPath = makeScreenshotPath();
  if (screenshotPath.empty()) {
    return false;
  }

  std::vector<unsigned char> pixels(static_cast<std::size_t>(width) *
                                    static_cast<std::size_t>(height) * 4u);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadBuffer(GL_BACK);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

  // OpenGL reads bottom-to-top. Flip rows so the PNG matches the on-screen view.
  const std::size_t rowBytes = static_cast<std::size_t>(width) * 4u;
  std::vector<unsigned char> flipped(pixels.size());
  for (int y = 0; y < height; ++y) {
    const std::size_t srcOffset =
        static_cast<std::size_t>(height - 1 - y) * rowBytes;
    const std::size_t dstOffset = static_cast<std::size_t>(y) * rowBytes;
    std::copy_n(pixels.data() + srcOffset, rowBytes, flipped.data() + dstOffset);
  }

  const int writeOk = stbi_write_png(screenshotPath.string().c_str(), width, height, 4,
                                     flipped.data(), width * 4);
  if (writeOk == 0) {
    return false;
  }

  std::printf("[Screenshot] Saved to %s\n", screenshotPath.string().c_str());
  return true;
}

bool saveCurrentWorldSession(WorldSaveService::WorldSession& worldSession,
                             Player& player, WorldStack& worldStack,
                             hudPortalTracker* portalTracker,
                             RuntimeUI::RuntimeUiState& uiState,
                             double totalPlaytimeSeconds, bool showToast,
                             std::string* outError = nullptr) {
  if (portalTracker) {
    worldSession.playerData.hasPortalTrackerState = true;
    worldSession.playerData.portalTrackerState =
        portalTracker->capturePersistentState();
  } else {
    worldSession.playerData.hasPortalTrackerState = false;
    worldSession.playerData.portalTrackerState = {};
  }

  if (!WorldSaveService::saveSession(worldSession, player, worldStack,
                                     totalPlaytimeSeconds, outError)) {
    return false;
  }

  worldSession.totalPlaytimeSeconds = totalPlaytimeSeconds;
  if (showToast) {
    RuntimeUI::triggerSaveToast(uiState);
  }
  return true;
}

void updateGame(Player& player, WorldStack& worldStack,
                GameAudioController& audioController, hudPortalInfo* portalInfo,
                hudPortalTracker* portalTracker,
                const GameChat& gameChat, float dt) {
  if (ENGINE::ISPAUSED()) {
    return;
  }

  PlayerUpdateMode playerUpdateMode = PlayerUpdateMode::FullGameplay;
  if (Input::hasUiFocus() || (portalInfo && portalInfo->isEditing()) ||
      (portalTracker && portalTracker->isMenuOpen())) {
    playerUpdateMode = PlayerUpdateMode::Frozen;
  } else if (player.isInventoryOpen() || gameChat.isOpen()) {
    playerUpdateMode = PlayerUpdateMode::SimulationOnly;
  }

  player.update(dt, worldStack, playerUpdateMode);
  worldStack.update(player.camera.position, player.camera.getForward(), dt);
  worldStack.updateEnemies(player, audioController, dt);
  worldStack.updateDroppedItems(player.camera.position, dt,
                                [&player, &audioController](const InventoryItem& pickedItem) {
                                  if (!player.tryAddItemToInventory(pickedItem, 1)) {
                                    return false;
                                  }
                                  audioController.onItemCollected();
                                  return true;
                                });
}

void renderFrame(GLFWwindow* window, Renderer& renderer, WorldStack& worldStack,
                 Player& player, float currentTime,
                 const RuntimeAppInternal::RuntimeDebugFlags& debugFlags) {
  ENGINE::BEGINPERFFRAME();

  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  if (width <= 0 || height <= 0) {
    return;
  }

  glViewport(0, 0, width, height);
  const float aspect = static_cast<float>(width) / static_cast<float>(height);
  ENGINE::BEGINGPUFRAMEQUERY();
  renderer.render(worldStack, player, aspect, currentTime, debugFlags.wireframeMode,
                  debugFlags.debugThirdPersonView);
  HUD::update(width, height);
  HUD::render(width, height);
#if defined(VP_ENABLE_DEV_TOOLS)
  if (RuntimeUI::shouldRenderDeveloperUi()) {
    ImGuiLayer::beginFrame();
    if (Input::hasUiFocus()) {
      BiomeTeleportWindow::syncCurrentBiome(worldStack.currentBiomeSelection());
      BiomeTeleportWindow::draw();
    }
    RuntimeUI::drawDeveloperUi(worldStack, player);
    ImGuiLayer::render();
  }
#endif
  ENGINE::ENDGPUFRAMEQUERY();
}

ENGINE::AUDIO::AudioListenerState buildListenerState(const Player& player) {
  ENGINE::AUDIO::AudioListenerState state;
  state.position = player.camera.position;
  state.forward = player.camera.getForward();
  state.up = player.camera.getUp();
  state.velocity = player.velocity;
  return state;
}

GameAudioFrameState buildAudioFrameState(const RuntimeUI::RuntimeUiState& uiState,
                                         const Player& player,
                                         const WorldStack& worldStack) {
  GameAudioFrameState state;
  state.paused = ENGINE::ISPAUSED();
  state.settingsMenuOpen = uiState.settingsMenuOpen;
  state.inventoryOpen = player.isInventoryOpen();
  state.portalPreviewActive =
      player.nestedPreview.active && player.nestedPreview.fade > 0.0f;
  state.biomePresetId = worldStack.currentBiomeSelection().presetId;
  return state;
}

#if defined(VP_ENABLE_DEV_TOOLS)
void handleDeveloperTools(WorldStack& worldStack, Player& player,
                          hudPortalInfo* portalInfo,
                          const WorldSaveService::WorldSession& worldSession,
                          double currentTime, double lastTime) {
  BiomeSelection targetBiome = worldSession.manifest.rootBiomeSelection;
  if (BiomeTeleportWindow::consumeTeleportRequest(targetBiome)) {
    if (DebugBiomeTools::teleportToBiome(
            worldStack, player, portalInfo, targetBiome, ClientDefaults::kRootSeed,
            ClientDefaults::kPlayerSpawnPosition, currentTime, lastTime)) {
      BiomeTeleportWindow::setSelectedBiome(targetBiome);
    }
  }

  if (BiomeTeleportWindow::consumeUpdateLocationRequest()) {
    DebugBiomeTools::updatePlayerLocationAroundPlayer(worldStack, player);
  }
}
#endif

void rebuildHudIfRequested(Player& player, WorldStack& worldStack, Renderer& renderer,
                           GameAudioController& audioController, GLFWwindow* window,
                           RuntimeAppInternal::RuntimeSettingsBundle& settingsBundle,
                           GameChat& gameChat, hudPortalInfo*& portalInfo,
                           hudPortalTracker*& portalTracker) {
  if (!settingsBundle.uiState.hudRebuildRequested) {
    return;
  }

  WorldSaveService::PlayerData::PortalTrackerState trackerState{};
  bool hasTrackerState = false;
  if (portalTracker) {
    trackerState = portalTracker->capturePersistentState();
    hasTrackerState = trackerState.trackingActive;
  }

  settingsBundle.uiState.hudRebuildRequested = false;
  HUD::clear();
  portalInfo = RuntimeUI::setupHUD(
      player, worldStack, renderer, audioController, window, settingsBundle.applied,
      settingsBundle.pending, settingsBundle.availableFonts,
      settingsBundle.availableResolutions, settingsBundle.uiState, &portalTracker);
  gameChat.setupHud();
  gameChat.syncHudState();
  if (portalTracker && hasTrackerState) {
    portalTracker->applyPersistentState(trackerState);
  }
  RuntimeUI::syncCursorVisibility(player, portalTracker);
}

void logShutdownStep(const char* message) {
  RuntimeAppInternal::printBootstrapInfo(message);
}

}  // namespace

namespace RuntimeAppInternal {

RuntimeLoopExitReason runMainLoop(GLFWwindow* window, Renderer& renderer,
                                  WorldStack& worldStack, Player& player,
                                  GameAudioController& audioController,
                                  GameChat& gameChat,
                                  WorldSaveService::WorldSession& worldSession,
                                  hudPortalInfo*& portalInfo,
                                  hudPortalTracker*& portalTracker,
                                  RuntimeSettingsBundle& settingsBundle) {
  // --- 1. Engine Timer Bootstrap ---
  RuntimeDebugFlags debugFlags;
  double lastTime = glfwGetTime();
  if (!ENGINE::ISINITIALIZED()) {
    printBootstrapInfo("Starting engine timers...");
    ENGINE::INIT(lastTime);
    printBootstrapSuccess("Bootstrap complete!");
  }

  double totalPlaytimeSeconds = worldSession.totalPlaytimeSeconds;
  double lastAutosavePlaytimeSeconds = totalPlaytimeSeconds;
  std::string autosaveError;
  const std::uint64_t pauseListenerId = ENGINE::ADDPAUSELISTENER(
      [&worldSession, &player, &worldStack, &settingsBundle,
       &totalPlaytimeSeconds, &lastAutosavePlaytimeSeconds, &autosaveError,
       portalTracker](
          bool paused) {
        if (!paused) {
          return;
        }

        if (!saveCurrentWorldSession(worldSession, player, worldStack,
                                     portalTracker, settingsBundle.uiState,
                                     totalPlaytimeSeconds, true, &autosaveError)) {
          if (!autosaveError.empty()) {
            RuntimeAppInternal::printBootstrapError(autosaveError.c_str());
          }
          return;
        }

        lastAutosavePlaytimeSeconds = totalPlaytimeSeconds;
      });

  // --- 2. Main Runtime Loop ---
  while (!glfwWindowShouldClose(window)) {
    // --- 2.1 Frame Timing & Input ---
    const double currentTime = glfwGetTime();
    // rawDt drives frame metrics/FPS; simDt is clamped to keep gameplay stable after hitches.
    const float rawDt = static_cast<float>(currentTime - lastTime);
    lastTime = currentTime;
    ENGINE::UPDATE(currentTime, rawDt);
    const float simDt = glm::min(ENGINE::GETDELTATIME(), 0.05f);
    const auto cpuFrameStart = std::chrono::steady_clock::now();
    Input::update();

    if (!ENGINE::ISPAUSED()) {
      totalPlaytimeSeconds += static_cast<double>(simDt);
    }

    // --- 2.2 UI Commands & Global Shortcuts ---
    const bool allowOpenChat =
        !ENGINE::ISPAUSED() && player.transition == PlayerTransition::NONE &&
        !player.isInventoryOpen() && (!portalInfo || !portalInfo->isEditing()) &&
        (!portalTracker || !portalTracker->isMenuOpen()) &&
        !Input::hasUiFocus();
    GameChatCommandContext chatCommandContext{
        player,
        worldStack,
        debugFlags.wireframeMode,
        debugFlags.debugThirdPersonView,
    };

    const bool chatConsumedInput =
        gameChat.handleFrameInput(chatCommandContext, allowOpenChat);
    if (!chatConsumedInput) {
      RuntimeUI::handleGlobalShortcuts(
          portalInfo, portalTracker, worldStack, player, audioController,
          settingsBundle.uiState,
          settingsBundle.applied, settingsBundle.pending);
    }

    gameChat.syncHudState();
    RuntimeUI::syncHudMenuState(settingsBundle.uiState);
    RuntimeUI::syncDebugHudState(settingsBundle.uiState, settingsBundle.applied);
    RuntimeUI::updateSaveToast(settingsBundle.uiState, rawDt);
    RuntimeUI::syncSaveToastState(settingsBundle.uiState);
    RuntimeUI::syncCursorVisibility(player, portalTracker);

    // --- 2.3 Gameplay & Audio ---
    updateGame(player, worldStack, audioController, portalInfo, portalTracker,
               gameChat, simDt);
    audioController.syncFrame(buildListenerState(player),
                              buildAudioFrameState(settingsBundle.uiState, player, worldStack),
                              rawDt);

    // --- 2.4 Rendering & Capture ---
    renderFrame(
        window,
        renderer,
        worldStack,
        player,
        static_cast<float>(ENGINE::GETTIME()),
        debugFlags
    );

    if (settingsBundle.uiState.returnToLauncherRequested) {
      break;
    }

    if (Input::keyPressed(GLFW_KEY_F2) &&
        canCaptureGameplayScreenshot(player, gameChat, portalInfo, portalTracker)) {
      if (!captureGameplayScreenshot(window)) {
        std::printf("[Screenshot] Failed to capture screenshot.\n");
      }
    }

    if (!ENGINE::ISPAUSED() &&
        totalPlaytimeSeconds - lastAutosavePlaytimeSeconds >= 300.0) {
      autosaveError.clear();
      if (saveCurrentWorldSession(worldSession, player, worldStack, portalTracker,
                                  settingsBundle.uiState, totalPlaytimeSeconds,
                                  true, &autosaveError)) {
        lastAutosavePlaytimeSeconds = totalPlaytimeSeconds;
      } else if (!autosaveError.empty()) {
        RuntimeAppInternal::printBootstrapError(autosaveError.c_str());
      }
    }

    // --- 2.5 Developer Tools & Frame Metrics ---
#if defined(VP_ENABLE_DEV_TOOLS)
    handleDeveloperTools(worldStack, player, portalInfo, worldSession, currentTime,
                         lastTime);
#endif

    const auto cpuFrameEnd = std::chrono::steady_clock::now();
    const float cpuFrameMs =
        std::chrono::duration<float, std::milli>(cpuFrameEnd - cpuFrameStart).count();
    const float fpsInstant = rawDt > 0.0f ? (1.0f / rawDt) : 0.0f;
    ENGINE::ENDPERFFRAME(cpuFrameMs, fpsInstant);

    // --- 2.6 Present & Deferred HUD Work ---
    glfwSwapBuffers(window);

    rebuildHudIfRequested(player, worldStack, renderer, audioController, window,
                          settingsBundle, gameChat, portalInfo, portalTracker);
  }

  autosaveError.clear();
  if (!saveCurrentWorldSession(worldSession, player, worldStack, portalTracker,
                               settingsBundle.uiState, totalPlaytimeSeconds, false,
                               &autosaveError) &&
      !autosaveError.empty()) {
    RuntimeAppInternal::printBootstrapError(autosaveError.c_str());
  }

  ENGINE::REMPAUSELISTENER(pauseListenerId);
  const bool returnToLauncher =
      settingsBundle.uiState.returnToLauncherRequested && !glfwWindowShouldClose(window);
  settingsBundle.uiState.returnToLauncherRequested = false;
  ENGINE::SETPAUSED(false);
  return returnToLauncher ? RuntimeLoopExitReason::ReturnToLauncher
                          : RuntimeLoopExitReason::QuitGame;
}

void shutdownGame(GLFWwindow*& window, Renderer& renderer, WorldStack* worldStack) {
  RuntimeAppInternal::printBootstrapInfo("Shutting down game runtime...");

  // --- 1. Developer UI Shutdown ---
#if defined(VP_ENABLE_DEV_TOOLS)
  logShutdownStep("Shutdown Step 1/7 - Developer UI");
  BiomeTeleportWindow::shutdown();
#endif
  logShutdownStep("Shutdown Step 2/7 - ImGui Layer");
  ImGuiLayer::shutdown();

  // --- 2. Gameplay Subsystems ---
  logShutdownStep("Shutdown Step 3/7 - HUD");
  HUD::cleanup();
  if (worldStack) {
    logShutdownStep("Shutdown Step 4/7 - World Stack");
    worldStack->shutdown();
  }
  logShutdownStep("Shutdown Step 5/7 - Renderer");
  renderer.cleanup();
  logShutdownStep("Shutdown Step 6/7 - Biome Registry");
  BiomeRegistry::instance().clear();
  logShutdownStep("Shutdown Step 7/7 - Input");
  Input::shutdown();

  // --- 3. Engine & Window Shutdown ---
  RuntimeAppInternal::printBootstrapInfo("Shutdown Step 8/8 - Engine and window");
  ENGINE::SHUTDOWN();
  Bootstrap::shutdownWindow(window);
  window = nullptr;
  RuntimeAppInternal::printBootstrapSuccess("Game runtime shutdown complete.");
}

[[noreturn]] void terminateRuntimeProcess(int code) {
  std::fflush(stdout);
  std::fflush(stderr);
  std::_Exit(code);
}

}  // namespace RuntimeAppInternal
