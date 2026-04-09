// game_audio_controller.cpp
// Unity mental model: Audio system bridge and event coordinator.
// Maps gameplay events to audio playback requests and manages listener/music state.

#pragma region Includes

// 1. Local Project Modules
#include "audio/game_audio_controller.hpp"

#pragma endregion

#pragma region 1. Lifecycle & Core Setup
// --- 1. Lifecycle & Core Setup ---

GameAudioController::GameAudioController(ENGINE::AUDIO::AudioManager& audioManager)
    : audioManager_(audioManager) {
}

void GameAudioController::applySettings(const ENGINE::AUDIO::AudioSettings& settings) {
    audioManager_.applySettings(settings);
}

#pragma endregion

#pragma region 2. Frame Synchronization
// --- 2. Frame Synchronization ---

void GameAudioController::syncFrame(const ENGINE::AUDIO::AudioListenerState& listenerState,
    const GameAudioFrameState& frameState,
    float dtSeconds) {
    // --- 1. Listener & Pause State ---
    audioManager_.setGameplayPaused(frameState.paused);
    audioManager_.setListenerState(listenerState);

    // --- 2. Biome & Music Context ---
    const bool useBiomeOverride = hasPendingBiomeOverride_;
    const std::string& effectiveBiomePresetId =
        useBiomeOverride ? pendingBiomeOverridePresetId_ : frameState.biomePresetId;

    ENGINE::AUDIO::MusicPlaybackRequest musicRequest;

    // Pause/settings should keep the current in-world music context alive.
    musicRequest.tags.push_back("state.gameplay");

    if (!frameState.paused && frameState.settingsMenuOpen) {
        musicRequest.tags.push_back("menu.settings");
    }

    if (!frameState.paused && frameState.inventoryOpen) {
        musicRequest.tags.push_back("menu.inventory");
    }

    if (!frameState.paused && frameState.portalPreviewActive) {
        musicRequest.tags.push_back("world.portal_preview");
    }

    if (!effectiveBiomePresetId.empty()) {
        musicRequest.tags.push_back("biome." + ENGINE::AUDIO::normalizeAudioToken(effectiveBiomePresetId));
    }

    musicRequest.immediate = !musicBootstrapped_ || forceImmediateMusicRefresh_;

    // --- 3. Apply Request & Update ---
    audioManager_.setMusicRequest(musicRequest);
    audioManager_.update(dtSeconds);

    // --- 4. Cleanup Frame State ---
    musicBootstrapped_ = true;
    forceImmediateMusicRefresh_ = false;

    if (useBiomeOverride && effectiveBiomePresetId == frameState.biomePresetId) {
        hasPendingBiomeOverride_ = false;
        pendingBiomeOverridePresetId_.clear();
    }
}

#pragma endregion

#pragma region 3. UI Events
// --- 3. UI Events ---

void GameAudioController::onHotbarSelectionChanged() {
    playUiEvent("ui.hotbar.select");
}

void GameAudioController::onInventoryStateChanged(bool open) {
    playUiEvent("ui.menu.click");
}

void GameAudioController::onPauseMenuToggled(bool open) {
    playUiEvent("ui.menu.click");
}

#pragma endregion

#pragma region 4. Gameplay Events (Blocks)
// --- 4. Gameplay Events (Blocks) ---

void GameAudioController::onBlockHit(BlockType blockType, const glm::ivec3& blockPos, bool initialHit) {
    audioManager_.playBlockAction(
        getBlockId(blockType),
        ENGINE::AUDIO::BlockSoundAction::Hit,
        makeBlockRequest(blockPos, !initialHit)
    );
}

void GameAudioController::onBlockBroken(BlockType blockType, const glm::ivec3& blockPos) {
    audioManager_.playBlockAction(
        getBlockId(blockType),
        ENGINE::AUDIO::BlockSoundAction::Break,
        makeBlockRequest(blockPos)
    );
}

void GameAudioController::onBlockPlaced(BlockType blockType, const glm::ivec3& blockPos) {
    audioManager_.playBlockAction(
        getBlockId(blockType),
        ENGINE::AUDIO::BlockSoundAction::Place,
        makeBlockRequest(blockPos)
    );
}

#pragma endregion

#pragma region 5. Gameplay Events (Player & Items)
// --- 5. Gameplay Events (Player & Items) ---

void GameAudioController::onItemCollected() {
    audioManager_.playEvent("player.item.pickup");
}

void GameAudioController::onPlayerDamaged() {
    audioManager_.playEvent("player.damage.hit");
}

void GameAudioController::onPlayerDoubleJump() {
    audioManager_.playEvent("player.double_jump");
}

void GameAudioController::onPlayerFootstep(BlockType blockType, const glm::vec3& worldPosition,
    float gain, float pitch) {
    ENGINE::AUDIO::SoundPlaybackRequest request = makeWorldRequest(worldPosition);
    request.gain = gain;
    request.pitch = pitch;

    audioManager_.playBlockAction(
        getBlockId(blockType),
        ENGINE::AUDIO::BlockSoundAction::Step,
        request
    );
}

#pragma endregion

#pragma region 6. Gameplay Events (World & Entities)
// --- 6. Gameplay Events (World & Entities) ---

void GameAudioController::onEnemyTriggerActivated(const glm::vec3& worldPosition) {
    audioManager_.playEvent(
        "enemy.trigger.activate",
        makeWorldRequest(worldPosition)
    );
}

void GameAudioController::onPortalEntered(const glm::ivec3& blockPos, const std::string& nextBiomePresetId) {
    pendingBiomeOverridePresetId_ = nextBiomePresetId;
    hasPendingBiomeOverride_ = true;
    forceImmediateMusicRefresh_ = true;

    audioManager_.playEvent("portal.enter", makeBlockRequest(blockPos));
}

void GameAudioController::onPortalExited(const glm::ivec3& blockPos) {
    hasPendingBiomeOverride_ = false;
    pendingBiomeOverridePresetId_.clear();
    forceImmediateMusicRefresh_ = true;

    audioManager_.playEvent("portal.exit", makeBlockRequest(blockPos));
}

#pragma endregion

#pragma region 7. Internal Helpers
// --- 7. Internal Helpers ---

ENGINE::AUDIO::SoundPlaybackRequest GameAudioController::makeBlockRequest(const glm::ivec3& blockPos,
    bool disableFadeIn) const {
    return makeWorldRequest(glm::vec3(blockPos) + glm::vec3(0.5f), disableFadeIn);
}

ENGINE::AUDIO::SoundPlaybackRequest GameAudioController::makeWorldRequest(const glm::vec3& worldPosition,
    bool disableFadeIn) const {
    ENGINE::AUDIO::SoundPlaybackRequest request;
    request.hasPosition = true;
    request.positional = true;
    request.position = worldPosition;
    request.gain = 1.0f;
    request.pitch = 1.0f;
    request.hasFadeInOverride = disableFadeIn;
    request.fadeInSeconds = 0.0f;
    return request;
}

void GameAudioController::playUiEvent(const char* eventName) {
    if (eventName) {
        audioManager_.playEvent(eventName);
    }
}

#pragma endregion