#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "audio/audio_manager.hpp"
#include "audio/audio_types.hpp"
#include "world/block.hpp"

struct GameAudioFrameState {
  bool paused = false;
  bool settingsMenuOpen = false;
  bool inventoryOpen = false;
  bool portalPreviewActive = false;
  std::string biomePresetId;
};

class GameAudioController {
public:
  explicit GameAudioController(ENGINE::AUDIO::AudioManager& audioManager);

  void applySettings(const ENGINE::AUDIO::AudioSettings& settings);
  void syncFrame(const ENGINE::AUDIO::AudioListenerState& listenerState,
                 const GameAudioFrameState& frameState, float dtSeconds);

  void onHotbarSelectionChanged();
  void onInventoryStateChanged(bool open);
  void onPauseMenuToggled(bool open);
  void onBlockHit(BlockType blockType, const glm::ivec3& blockPos,
                  bool initialHit);
  void onBlockBroken(BlockType blockType, const glm::ivec3& blockPos);
  void onBlockPlaced(BlockType blockType, const glm::ivec3& blockPos);
  void onItemCollected();
  void onPlayerDamaged();
  void onPlayerFootstep(BlockType blockType, const glm::vec3& worldPosition,
                        float gain, float pitch);
  void onEnemyTriggerActivated(const glm::vec3& worldPosition);
  void onPortalEntered(const glm::ivec3& blockPos,
                       const std::string& nextBiomePresetId);
  void onPortalExited(const glm::ivec3& blockPos);

private:
  ENGINE::AUDIO::SoundPlaybackRequest makeBlockRequest(
      const glm::ivec3& blockPos, bool disableFadeIn = false) const;
  ENGINE::AUDIO::SoundPlaybackRequest makeWorldRequest(
      const glm::vec3& worldPosition, bool disableFadeIn = false) const;
  void playUiEvent(const char* eventName);

  ENGINE::AUDIO::AudioManager& audioManager_;
  std::string pendingBiomeOverridePresetId_;
  bool musicBootstrapped_ = false;
  bool hasPendingBiomeOverride_ = false;
  bool forceImmediateMusicRefresh_ = false;
};
