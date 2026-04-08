#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace ENGINE {
namespace AUDIO {

inline constexpr std::uint32_t kInvalidAudioId = 0u;

inline std::string normalizeAudioToken(std::string value) {
  for (char& ch : value) {
    if (ch >= 'A' && ch <= 'Z') {
      ch = static_cast<char>(ch - 'A' + 'a');
    } else if (ch == ' ' || ch == '-') {
      ch = '_';
    }
  }
  return value;
}

struct SoundEventId {
  std::uint32_t value = kInvalidAudioId;

  bool valid() const { return value != kInvalidAudioId; }
};

struct BlockSoundProfileId {
  std::uint32_t value = kInvalidAudioId;

  bool valid() const { return value != kInvalidAudioId; }
};

struct MusicContextId {
  std::uint32_t value = kInvalidAudioId;

  bool valid() const { return value != kInvalidAudioId; }
};

struct AudioSourceHandle {
  std::uint32_t value = kInvalidAudioId;

  bool valid() const { return value != kInvalidAudioId; }
};

struct AudioBufferHandle {
  std::uint32_t value = kInvalidAudioId;

  bool valid() const { return value != kInvalidAudioId; }
};

enum class SoundCategoryId : std::uint8_t {
  Master = 0,
  Music,
  Ambient,
  Block,
  Player,
  Footsteps,
  Hostile,
  Neutral,
  Weather,
  Ui,
  Count
};

inline constexpr std::size_t kSoundCategoryCount =
    static_cast<std::size_t>(SoundCategoryId::Count);

inline constexpr std::array<SoundCategoryId, kSoundCategoryCount>
    kSoundCategoryOrder = {
        SoundCategoryId::Master,  SoundCategoryId::Music,
        SoundCategoryId::Ambient, SoundCategoryId::Block,
        SoundCategoryId::Player,  SoundCategoryId::Footsteps,
        SoundCategoryId::Hostile,
        SoundCategoryId::Neutral, SoundCategoryId::Weather,
        SoundCategoryId::Ui,
    };

inline std::size_t toIndex(SoundCategoryId category) {
  return static_cast<std::size_t>(category);
}

inline const char* soundCategoryIdToString(SoundCategoryId category) {
  switch (category) {
  case SoundCategoryId::Master:
    return "master";
  case SoundCategoryId::Music:
    return "music";
  case SoundCategoryId::Ambient:
    return "ambient";
  case SoundCategoryId::Block:
    return "block";
  case SoundCategoryId::Player:
    return "player";
  case SoundCategoryId::Footsteps:
    return "footsteps";
  case SoundCategoryId::Hostile:
    return "hostile";
  case SoundCategoryId::Neutral:
    return "neutral";
  case SoundCategoryId::Weather:
    return "weather";
  case SoundCategoryId::Ui:
    return "ui";
  default:
    return "master";
  }
}

inline bool tryParseSoundCategoryId(const std::string& rawValue,
                                    SoundCategoryId& outCategory) {
  const std::string value = normalizeAudioToken(rawValue);
  for (SoundCategoryId candidate : kSoundCategoryOrder) {
    if (value == soundCategoryIdToString(candidate)) {
      outCategory = candidate;
      return true;
    }
  }

  outCategory = SoundCategoryId::Master;
  return false;
}

enum class BlockSoundAction : std::uint8_t {
  Break = 0,
  Step,
  Place,
  Hit,
  Count
};

inline constexpr std::size_t kBlockSoundActionCount =
    static_cast<std::size_t>(BlockSoundAction::Count);

inline std::size_t toIndex(BlockSoundAction action) {
  return static_cast<std::size_t>(action);
}

inline const char* blockSoundActionToString(BlockSoundAction action) {
  switch (action) {
  case BlockSoundAction::Break:
    return "break";
  case BlockSoundAction::Step:
    return "step";
  case BlockSoundAction::Place:
    return "place";
  case BlockSoundAction::Hit:
    return "hit";
  default:
    return "break";
  }
}

inline bool tryParseBlockSoundAction(const std::string& rawValue,
                                     BlockSoundAction& outAction) {
  const std::string value = normalizeAudioToken(rawValue);
  for (std::size_t index = 0; index < kBlockSoundActionCount; index++) {
    const BlockSoundAction candidate = static_cast<BlockSoundAction>(index);
    if (value == blockSoundActionToString(candidate)) {
      outAction = candidate;
      return true;
    }
  }

  outAction = BlockSoundAction::Break;
  return false;
}

struct FloatRange {
  float min = 1.0f;
  float max = 1.0f;

  void sanitize(float fallbackMin = 1.0f, float fallbackMax = 1.0f) {
    if (!(min == min) || !(max == max)) {
      min = fallbackMin;
      max = fallbackMax;
    }
    if (min > max) {
      const float temp = min;
      min = max;
      max = temp;
    }
  }
};

struct AudioSettings {
  std::array<float, kSoundCategoryCount> categoryVolumes{};
  std::array<bool, kSoundCategoryCount> categoryMuted{};
  bool musicEnabled = true;
  bool globalMute = false;

  AudioSettings() { reset(); }

  void reset() {
    categoryVolumes.fill(1.0f);
    categoryMuted.fill(false);
    musicEnabled = true;
    globalMute = false;
  }

  void sanitize() {
    for (float& volume : categoryVolumes) {
      if (!(volume == volume)) {
        volume = 1.0f;
      }
      if (volume < 0.0f) {
        volume = 0.0f;
      }
      if (volume > 1.0f) {
        volume = 1.0f;
      }
    }
  }

  float volumeFor(SoundCategoryId category) const {
    return categoryVolumes[toIndex(category)];
  }

  bool mutedFor(SoundCategoryId category) const {
    if (globalMute) {
      return true;
    }
    if (category == SoundCategoryId::Music && !musicEnabled) {
      return true;
    }
    return categoryMuted[toIndex(category)];
  }
};

struct SoundPlaybackRequest {
  glm::vec3 position{0.0f};
  glm::vec3 velocity{0.0f};
  float gain = 1.0f;
  float pitch = 1.0f;
  float fadeInSeconds = 0.0f;
  bool hasPosition = false;
  bool positional = false;
  bool hasLoopOverride = false;
  bool hasFadeInOverride = false;
  bool looping = false;
  int priorityBias = 0;
};

struct MusicPlaybackRequest {
  std::vector<std::string> tags;
  bool immediate = false;
};

struct AudioListenerState {
  glm::vec3 position{0.0f};
  glm::vec3 forward{0.0f, 0.0f, -1.0f};
  glm::vec3 up{0.0f, 1.0f, 0.0f};
  glm::vec3 velocity{0.0f};
};

} // namespace AUDIO
} // namespace ENGINE
