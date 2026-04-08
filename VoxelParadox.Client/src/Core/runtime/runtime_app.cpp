// runtime_app.cpp
// Unity mental model: this is the top-level game bootstrap script.
// Keep this file high level. Detailed bootstrap and frame logic live in the
// split runtime_app_bootstrap.cpp and runtime_app_loop.cpp files.

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <stdexcept>
#include <string>

// Third-party
#include <GLFW/glfw3.h>

// Internal - Config & Base
#include "client_defaults.hpp"
#include "path/app_paths.hpp"
#include "engine/bootstrap.hpp"
#include "engine/engine.hpp"

// Internal - Runtime & Logic
#include "runtime/runtime_app.hpp"
#include "runtime/runtime_app_internal.hpp"
#include "runtime/game_chat.hpp"
#include "player/player.hpp"

// Internal - World & Audio
#include "world/biome_registry.hpp"
#include "world/world_stack.hpp"
#include "audio/game_audio_controller.hpp"

// Internal - Rendering & UI
#include "render/renderer.hpp"
#include "render/hud/hud.hpp"
#include "ui/biome_teleport_window.hpp"
#include "ui/imgui_layer.hpp"

namespace {

    void printStartupBanner() {
        std::printf("-------------------------------------------------------------");
        std::printf("\n\nThe fractal world is nothing more than a poorly told dream...\n\n");
        std::printf("\n- With love, Saitox.\n\n");
        std::printf("-------------------------------------------------------------\n\n\n\n");
        std::fflush(stdout);
    }

    Player preparePlayer(const GameSettings& settings,
                         const glm::vec3& resolvedSpawnPosition) {
        RuntimeAppInternal::printBootstrapInfo("Preparing the player...");

        Player player;
        player.camera.position = resolvedSpawnPosition;
        player.camera.sensitivity = settings.mouseSensitivity;

        char spawnBuffer[64];
        std::snprintf(spawnBuffer, sizeof(spawnBuffer), "%.2f, %.2f, %.2f",
            player.camera.position.x, player.camera.position.y,
            player.camera.position.z);

        RuntimeAppInternal::printBootstrapDetail("Player Spawn Pos:", spawnBuffer);
        RuntimeAppInternal::printBootstrapSuccess("Player initialized!");

        return player;
    }

    void initializeDeveloperUi(GLFWwindow* window, const BiomeSelection& rootBiomeSelection) {
#if defined(VP_ENABLE_DEV_TOOLS)
        const bool imguiInitialized = ImGuiLayer::initialize(window);
        Bootstrap::reportImGuiStatus(imguiInitialized);

        if (!imguiInitialized) {
            throw std::runtime_error("ImGui initialization failed.");
        }

        BiomeTeleportWindow::setAvailableBiomes(BiomeRegistry::instance().buildSelectableBiomes());
        BiomeTeleportWindow::setSelectedBiome(rootBiomeSelection);
#else
        (void)window;
        (void)rootBiomeSelection;
#endif
    }

} // namespace

namespace VoxelParadox {

    int runRuntimeApp() {
        // --- 1. Basic Initialization ---
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        printStartupBanner();

        std::string settingsLoadError;
        RuntimeAppInternal::RuntimeSettingsBundle settingsBundle =
            RuntimeAppInternal::loadRuntimeSettings(&settingsLoadError);

        if (!settingsLoadError.empty()) {
            RuntimeAppInternal::printBootstrapError(settingsLoadError.c_str());
        }

        // --- 2. Biome & Resource Loading ---
        BiomeSelection rootBiomeSelection;
        std::shared_ptr<const VoxelGame::BiomePreset> rootBiomePreset;

        if (!RuntimeAppInternal::loadRequiredBiomePresets(rootBiomeSelection, rootBiomePreset)) {
            return -1;
        }

        // --- 3. Window & Graphics Context ---
        Renderer renderer;
        GLFWwindow* window = nullptr;
        const Bootstrap::Config bootstrapConfig = RuntimeAppInternal::makeBootstrapConfig(settingsBundle.applied);

        if (!Bootstrap::initialize(bootstrapConfig, window)) {
            return -1;
        }

        if (!renderer.init()) {
            Bootstrap::shutdownWindow(window);
            return -1;
        }
        renderer.setRenderScale(settingsBundle.applied.renderScale);

        if (!HUD::init()) {
            RuntimeAppInternal::shutdownGame(window, renderer, nullptr);
            return -1;
        }

        // --- 4. UI & Developer Tools ---
        try {
            initializeDeveloperUi(window, rootBiomeSelection);
        }
        catch (const std::exception&) {
            RuntimeAppInternal::shutdownGame(window, renderer, nullptr);
            return -1;
        }

        // --- 5. World & Player Setup ---
        WorldStack worldStack;
        worldStack.setRenderDistancePreset(settingsBundle.applied.renderDistance);
        WorldStack::setSaveWorldDirectory(AppPaths::resolveString(bootstrapConfig.saveDirectory));

        if (!RuntimeAppInternal::prepareRootWorld(
            worldStack, ClientDefaults::kRootSeed, rootBiomeSelection,
            rootBiomePreset, ClientDefaults::kPlayerSpawnPosition)) {
            RuntimeAppInternal::shutdownGame(window, renderer, &worldStack);
            return -1;
        }

        const glm::vec3 resolvedSpawnPosition =
            worldStack.resolveCurrentWorldSpawnPosition(
                ClientDefaults::kPlayerSpawnPosition,
                Player::kDefaultPlayerRadius,
                Player::kDefaultStandingHeight,
                Player::kDefaultStandingEyeHeight);
        Player player = preparePlayer(settingsBundle.applied, resolvedSpawnPosition);

        // --- 6. Audio Subsystem ---
        RuntimeAppInternal::printBootstrapInfo("Preparing the audio subsystem...");

        ENGINE::AUDIO::AudioManager audioManager;
        std::string audioStatusMessage;
        audioManager.initialize({}, &audioStatusMessage);

        GameAudioController audioController(audioManager);
        audioController.applySettings(settingsBundle.applied.audioSettings);
        player.setAudioController(&audioController);

        RuntimeAppInternal::printBootstrapDetail("Audio Backend:", audioManager.backendReady() ? "OpenAL" : "Silent");
        if (!audioStatusMessage.empty()) {
            RuntimeAppInternal::printBootstrapDetail("Audio Status:", audioStatusMessage);
        }
        RuntimeAppInternal::printBootstrapSuccess("Audio initialized!");

        // --- 7. HUD & Social Systems ---
        RuntimeAppInternal::printBootstrapInfo("Preparing the HUD...");

        GameChat gameChat;
        hudPortalInfo* portalInfo = RuntimeUI::setupHUD(
            player, worldStack, renderer, audioController, window,
            settingsBundle.applied, settingsBundle.pending,
            settingsBundle.availableFonts, settingsBundle.availableResolutions,
            settingsBundle.uiState
        );

        gameChat.setupHud();
        gameChat.syncHudState();
        RuntimeAppInternal::printBootstrapSuccess("HUD initialized!");

        // --- 8. Main Loop & Shutdown ---
        RuntimeAppInternal::runMainLoop(
            window, renderer, worldStack, player,
            audioController, gameChat, portalInfo,
            settingsBundle, rootBiomeSelection
        );

        RuntimeAppInternal::shutdownGame(window, renderer, &worldStack);
        RuntimeAppInternal::terminateRuntimeProcess(0);

        return 0; // Adicionado retorno padrÃ£o de sucesso
    }

} // namespace VoxelParadox
