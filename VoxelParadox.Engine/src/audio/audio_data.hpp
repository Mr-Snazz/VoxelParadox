#pragma once

#include <array>
#include <deque>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "audio/audio_types.hpp"

namespace ENGINE {
namespace AUDIO {

enum class AntiRepeatPolicy : std::uint8_t {
  AllowImmediateRepeat = 0,
  RecentHistory,
  ShuffleBag,
};

struct CategoryDefinition {
  SoundCategoryId id = SoundCategoryId::Master;
  std::string label;
  float defaultVolume = 1.0f;
  bool mutedByDefault = false;
};

struct AudioClipDefinition {
  std::filesystem::path path;
};

struct SoundEventDefinition {
  SoundEventId id{};
  std::string name;
  SoundCategoryId category = SoundCategoryId::Block;
  std::vector<AudioClipDefinition> clips;
  float baseGain = 1.0f;
  float fadeInSeconds = 0.0f;
  FloatRange pitchRange{1.0f, 1.0f};
  FloatRange volumeRange{1.0f, 1.0f};
  bool positional = true;
  bool loop = false;
  int maxConcurrency = 8;
  int priority = 0;
  AntiRepeatPolicy antiRepeat = AntiRepeatPolicy::RecentHistory;
  float referenceDistance = 3.0f;
  float maxDistance = 48.0f;
  float rolloffFactor = 1.0f;
};

struct BlockSoundProfileDefinition {
  BlockSoundProfileId id{};
  std::string name;
  std::array<SoundEventId, kBlockSoundActionCount> actions{};
};

struct MusicTrackDefinition {
  std::string id;
  std::filesystem::path path;
  float weight = 1.0f;
};

struct MusicContextDefinition {
  MusicContextId id{};
  std::string name;
  int priority = 0;
  std::vector<std::string> requiredTags;
  FloatRange startDelaySeconds{8.0f, 18.0f};
  FloatRange silenceSeconds{20.0f, 45.0f};
  float fadeInSeconds = 2.0f;
  float fadeOutSeconds = 2.0f;
  std::vector<MusicTrackDefinition> tracks;
};

class AudioDatabase {
public:
  AudioDatabase();

  bool loadFromManifest(const std::filesystem::path& manifestPath,
                        std::vector<std::string>& outWarnings);

  const SoundEventDefinition* findEvent(SoundEventId id) const;
  const SoundEventDefinition* findEventByName(const std::string& name) const;

  const BlockSoundProfileDefinition* findBlockProfile(
      BlockSoundProfileId id) const;
  const BlockSoundProfileDefinition* findBlockProfileByBlockId(
      const std::string& blockId) const;
  SoundEventId resolveBlockAction(const std::string& blockId,
                                  BlockSoundAction action) const;

  const MusicContextDefinition* findMusicContext(MusicContextId id) const;
  const MusicContextDefinition* resolveMusicContext(
      const std::vector<std::string>& activeTags) const;

  const CategoryDefinition* category(SoundCategoryId id) const;
  const std::vector<SoundEventDefinition>& events() const { return events_; }
  const std::vector<MusicContextDefinition>& musicContexts() const {
    return musicContexts_;
  }

private:
  static std::string makeLookupKey(const std::string& value);
  static std::filesystem::path resolveAudioPath(
      const std::filesystem::path& audioRoot, const std::string& rawPath);

  std::array<CategoryDefinition, kSoundCategoryCount> categories_{};
  std::vector<SoundEventDefinition> events_;
  std::vector<BlockSoundProfileDefinition> blockProfiles_;
  std::vector<MusicContextDefinition> musicContexts_;
  std::unordered_map<std::string, SoundEventId> eventLookup_;
  std::unordered_map<std::string, BlockSoundProfileId> blockLookup_;
  std::unordered_map<std::string, MusicContextId> musicContextLookup_;
  BlockSoundProfileId fallbackBlockProfileId_{};
};

} // namespace AUDIO
} // namespace ENGINE
