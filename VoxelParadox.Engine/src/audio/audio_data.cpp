#include "audio/audio_data.hpp"

#include <algorithm>
#include <fstream>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "path/app_paths.hpp"

namespace ENGINE {
namespace AUDIO {
namespace {

using json = nlohmann::json;

const char* defaultCategoryLabel(SoundCategoryId id) {
  switch (id) {
  case SoundCategoryId::Master:
    return "Master";
  case SoundCategoryId::Music:
    return "Music";
  case SoundCategoryId::Ambient:
    return "Ambient";
  case SoundCategoryId::Block:
    return "Block";
  case SoundCategoryId::Player:
    return "Player";
  case SoundCategoryId::Hostile:
    return "Hostile";
  case SoundCategoryId::Neutral:
    return "Neutral";
  case SoundCategoryId::Weather:
    return "Weather";
  case SoundCategoryId::Ui:
    return "UI";
  default:
    return "Master";
  }
}

FloatRange parseRange(const json& value, float fallbackMin, float fallbackMax) {
  FloatRange range{fallbackMin, fallbackMax};
  if (value.is_array() && value.size() >= 2 && value[0].is_number() &&
      value[1].is_number()) {
    range.min = value[0].get<float>();
    range.max = value[1].get<float>();
  } else if (value.is_object()) {
    if (value.contains("min") && value["min"].is_number()) {
      range.min = value["min"].get<float>();
    }
    if (value.contains("max") && value["max"].is_number()) {
      range.max = value["max"].get<float>();
    }
  } else if (value.is_number()) {
    const float scalar = value.get<float>();
    range.min = scalar;
    range.max = scalar;
  }

  range.sanitize(fallbackMin, fallbackMax);
  return range;
}

AntiRepeatPolicy parseAntiRepeatPolicy(const json& value, std::size_t clipCount) {
  if (value.is_string()) {
    const std::string mode = normalizeAudioToken(value.get<std::string>());
    if (mode == "none" || mode == "allow_immediate_repeat") {
      return AntiRepeatPolicy::AllowImmediateRepeat;
    }
    if (mode == "shuffle" || mode == "shuffle_bag") {
      return AntiRepeatPolicy::ShuffleBag;
    }
    if (mode == "recent" || mode == "recent_history") {
      return AntiRepeatPolicy::RecentHistory;
    }
  }

  return clipCount >= 3 ? AntiRepeatPolicy::ShuffleBag
                        : AntiRepeatPolicy::RecentHistory;
}

bool loadJsonDocument(const std::filesystem::path& path, json& outDocument,
                      std::string& outError) {
  std::ifstream input(path, std::ios::binary);
  if (!input.is_open()) {
    outError = "Failed to open " + path.string();
    return false;
  }

  outDocument = json::parse(input, nullptr, false, true);
  if (outDocument.is_discarded()) {
    outError = "Failed to parse JSON document: " + path.string();
    return false;
  }

  return true;
}

std::filesystem::path resolveDefinitionPath(
    const std::filesystem::path& manifestPath,
    const std::filesystem::path& defaultRelativePath, const json& manifest,
    const char* key) {
  if (manifest.is_object() && manifest.contains(key) && manifest[key].is_string()) {
    return manifestPath.parent_path() / manifest[key].get<std::string>();
  }
  return manifestPath.parent_path() / defaultRelativePath;
}

std::vector<std::string> parseStringArray(const json& value) {
  std::vector<std::string> result;
  if (!value.is_array()) {
    return result;
  }

  for (const json& entry : value) {
    if (entry.is_string()) {
      result.push_back(normalizeAudioToken(entry.get<std::string>()));
    }
  }
  return result;
}

void registerBlockAliases(const json& value,
                          std::vector<std::string>& outBlockIds) {
  if (!value.is_array()) {
    return;
  }

  for (const json& entry : value) {
    if (entry.is_string()) {
      outBlockIds.push_back(normalizeAudioToken(entry.get<std::string>()));
    }
  }
}

} // namespace

AudioDatabase::AudioDatabase() {
  for (SoundCategoryId category : kSoundCategoryOrder) {
    CategoryDefinition definition;
    definition.id = category;
    definition.label = defaultCategoryLabel(category);
    definition.defaultVolume = 1.0f;
    definition.mutedByDefault = false;
    categories_[toIndex(category)] = definition;
  }
}

bool AudioDatabase::loadFromManifest(const std::filesystem::path& manifestPath,
                                     std::vector<std::string>& outWarnings) {
  *this = AudioDatabase();

  events_.clear();
  blockProfiles_.clear();
  musicContexts_.clear();
  eventLookup_.clear();
  blockLookup_.clear();
  musicContextLookup_.clear();
  fallbackBlockProfileId_ = {};

  const std::filesystem::path resolvedManifestPath =
      manifestPath.empty()
          ? (AppPaths::assetsRoot() / "Audio" / "Definitions" / "audio_manifest.json")
          : manifestPath;
  const std::filesystem::path definitionsRoot = resolvedManifestPath.parent_path();
  const std::filesystem::path audioRoot = definitionsRoot.parent_path();

  json manifest;
  if (std::error_code ec; !std::filesystem::exists(resolvedManifestPath, ec)) {
    outWarnings.push_back("Audio manifest not found. Falling back to default file layout.");
  } else {
    std::string manifestError;
    if (!loadJsonDocument(resolvedManifestPath, manifest, manifestError)) {
      outWarnings.push_back(manifestError);
    }
  }

  const std::filesystem::path categoriesPath =
      resolveDefinitionPath(resolvedManifestPath, "audio_categories.json", manifest,
                            "categories");
  const std::filesystem::path eventsPath =
      resolveDefinitionPath(resolvedManifestPath, "audio_events.json", manifest,
                            "events");
  const std::filesystem::path profilesPath =
      resolveDefinitionPath(resolvedManifestPath, "block_sound_profiles.json", manifest,
                            "profiles");
  const std::filesystem::path musicContextsPath =
      resolveDefinitionPath(resolvedManifestPath, "music_contexts.json", manifest,
                            "music_contexts");

  json categoriesDocument;
  std::string loadError;
  if (loadJsonDocument(categoriesPath, categoriesDocument, loadError)) {
    const json categories = categoriesDocument.value("categories", json::array());
    if (categories.is_array()) {
      for (const json& entry : categories) {
        if (!entry.is_object() || !entry.contains("id") || !entry["id"].is_string()) {
          continue;
        }

        SoundCategoryId category = SoundCategoryId::Master;
        if (!tryParseSoundCategoryId(entry["id"].get<std::string>(), category)) {
          outWarnings.push_back("Unknown audio category id in " + categoriesPath.string());
          continue;
        }

        CategoryDefinition& definition = categories_[toIndex(category)];
        definition.label = entry.value("label", std::string(defaultCategoryLabel(category)));
        definition.defaultVolume = entry.value("default_volume", 1.0f);
        definition.mutedByDefault = entry.value("muted_by_default", false);
        if (definition.defaultVolume < 0.0f) {
          definition.defaultVolume = 0.0f;
        }
        if (definition.defaultVolume > 1.0f) {
          definition.defaultVolume = 1.0f;
        }
      }
    }
  } else {
    outWarnings.push_back(loadError);
  }

  json eventsDocument;
  loadError.clear();
  if (loadJsonDocument(eventsPath, eventsDocument, loadError)) {
    const json eventsValue = eventsDocument.value("events", json::array());
    if (eventsValue.is_array()) {
      for (const json& entry : eventsValue) {
        if (!entry.is_object()) {
          continue;
        }

        const std::string rawName = entry.value("id", std::string());
        const std::string eventName = makeLookupKey(rawName);
        if (eventName.empty()) {
          outWarnings.push_back("Audio event without id in " + eventsPath.string());
          continue;
        }

        SoundCategoryId category = SoundCategoryId::Block;
        if (!tryParseSoundCategoryId(entry.value("category", std::string("block")),
                                     category)) {
          outWarnings.push_back("Unknown category '" +
                                entry.value("category", std::string()) +
                                "' for event '" + rawName + "'.");
          category = SoundCategoryId::Block;
        }

        std::vector<AudioClipDefinition> clips;
        const json clipsValue = entry.value("clips", json::array());
        if (clipsValue.is_array()) {
          for (const json& clipValue : clipsValue) {
            std::string clipPath;
            if (clipValue.is_string()) {
              clipPath = clipValue.get<std::string>();
            } else if (clipValue.is_object() && clipValue.contains("path") &&
                       clipValue["path"].is_string()) {
              clipPath = clipValue["path"].get<std::string>();
            }

            if (clipPath.empty()) {
              continue;
            }

            AudioClipDefinition clip;
            clip.path = resolveAudioPath(audioRoot, clipPath);
            clips.push_back(clip);
          }
        }

        if (clips.empty()) {
          outWarnings.push_back("Audio event '" + rawName +
                                "' does not define any clips.");
          continue;
        }

        SoundEventDefinition definition;
        definition.id.value =
            static_cast<std::uint32_t>(events_.size() + 1u);
        definition.name = eventName;
        definition.category = category;
        definition.clips = std::move(clips);
        definition.baseGain = entry.value("base_gain", 1.0f);
        definition.fadeInSeconds =
            std::max(0.0f, entry.value("fade_in_seconds", 0.0f));
        definition.pitchRange =
            parseRange(entry.value("pitch_range", json()), 1.0f, 1.0f);
        definition.volumeRange =
            parseRange(entry.value("volume_range", json()), 1.0f, 1.0f);
        definition.positional = entry.value("positional", category != SoundCategoryId::Ui &&
                                                              category != SoundCategoryId::Music);
        definition.loop = entry.value("loop", false);
        definition.maxConcurrency =
            std::max(1, entry.value("max_concurrency", 8));
        definition.priority = entry.value("priority", 0);
        definition.antiRepeat =
            parseAntiRepeatPolicy(entry.value("anti_repeat", json()),
                                  definition.clips.size());
        definition.referenceDistance =
            std::max(0.1f, entry.value("reference_distance", 3.0f));
        definition.maxDistance =
            std::max(definition.referenceDistance,
                     entry.value("max_distance", 48.0f));
        definition.rolloffFactor =
            std::max(0.0f, entry.value("rolloff_factor", 1.0f));

        events_.push_back(definition);
        eventLookup_[definition.name] = definition.id;
      }
    }
  } else {
    outWarnings.push_back(loadError);
  }

  json profilesDocument;
  loadError.clear();
  if (loadJsonDocument(profilesPath, profilesDocument, loadError)) {
    std::unordered_map<std::string, BlockSoundProfileId> profileNameLookup;
    std::unordered_map<std::string, std::vector<std::string>> profileBlocks;

    const json profilesValue = profilesDocument.value("profiles", json::array());
    if (profilesValue.is_array()) {
      for (const json& entry : profilesValue) {
        if (!entry.is_object()) {
          continue;
        }

        const std::string rawId = entry.value("id", std::string());
        const std::string profileName = makeLookupKey(rawId);
        if (profileName.empty()) {
          outWarnings.push_back("Block sound profile without id in " +
                                profilesPath.string());
          continue;
        }

        BlockSoundProfileDefinition definition;
        definition.id.value =
            static_cast<std::uint32_t>(blockProfiles_.size() + 1u);
        definition.name = profileName;
        definition.actions.fill({});

        for (std::size_t actionIndex = 0; actionIndex < kBlockSoundActionCount;
             actionIndex++) {
          const BlockSoundAction action = static_cast<BlockSoundAction>(actionIndex);
          const char* key = blockSoundActionToString(action);
          if (!entry.contains(key) || !entry[key].is_string()) {
            continue;
          }

          const std::string eventName = makeLookupKey(entry[key].get<std::string>());
          auto eventIt = eventLookup_.find(eventName);
          if (eventIt == eventLookup_.end()) {
            outWarnings.push_back("Unknown event '" + eventName +
                                  "' referenced by block profile '" + profileName +
                                  "'.");
            continue;
          }

          definition.actions[actionIndex] = eventIt->second;
        }

        std::vector<std::string> aliases;
        registerBlockAliases(entry.value("blocks", json::array()), aliases);

        profileNameLookup[definition.name] = definition.id;
        profileBlocks[definition.name] = aliases;
        blockProfiles_.push_back(definition);
      }
    }

    const json mappingsValue = profilesDocument.value("block_map", json::array());
    if (mappingsValue.is_array()) {
      for (const json& entry : mappingsValue) {
        if (!entry.is_object()) {
          continue;
        }
        const std::string blockId =
            makeLookupKey(entry.value("block", std::string()));
        const std::string profileId =
            makeLookupKey(entry.value("profile", std::string()));
        if (blockId.empty() || profileId.empty()) {
          continue;
        }
        profileBlocks[profileId].push_back(blockId);
      }
    }

    for (const auto& pair : profileBlocks) {
      auto profileIt = profileNameLookup.find(pair.first);
      if (profileIt == profileNameLookup.end()) {
        continue;
      }

      for (const std::string& blockId : pair.second) {
        if (!blockId.empty()) {
          blockLookup_[blockId] = profileIt->second;
        }
      }
    }

    const std::string fallbackProfile =
        makeLookupKey(profilesDocument.value("fallback_profile", std::string("generic")));
    auto fallbackIt = profileNameLookup.find(fallbackProfile);
    if (fallbackIt != profileNameLookup.end()) {
      fallbackBlockProfileId_ = fallbackIt->second;
    }
  } else {
    outWarnings.push_back(loadError);
  }

  json musicDocument;
  loadError.clear();
  if (loadJsonDocument(musicContextsPath, musicDocument, loadError)) {
    const json contextsValue = musicDocument.value("contexts", json::array());
    if (contextsValue.is_array()) {
      for (const json& entry : contextsValue) {
        if (!entry.is_object()) {
          continue;
        }

        const std::string contextName =
            makeLookupKey(entry.value("id", std::string()));
        if (contextName.empty()) {
          outWarnings.push_back("Music context without id in " +
                                musicContextsPath.string());
          continue;
        }

        MusicContextDefinition definition;
        definition.id.value =
            static_cast<std::uint32_t>(musicContexts_.size() + 1u);
        definition.name = contextName;
        definition.priority = entry.value("priority", 0);
        definition.requiredTags =
            parseStringArray(entry.value("required_tags", json::array()));
        definition.startDelaySeconds =
            parseRange(entry.value("start_delay_seconds", json()), 8.0f, 18.0f);
        definition.silenceSeconds =
            parseRange(entry.value("silence_seconds", json()), 20.0f, 45.0f);
        definition.fadeInSeconds =
            std::max(0.0f, entry.value("fade_in_seconds", 2.0f));
        definition.fadeOutSeconds =
            std::max(0.0f, entry.value("fade_out_seconds", 2.0f));

        const json tracksValue = entry.value("tracks", json::array());
        if (tracksValue.is_array()) {
          for (const json& trackValue : tracksValue) {
            MusicTrackDefinition track;
            if (trackValue.is_string()) {
              const std::string rawPath = trackValue.get<std::string>();
              track.path = resolveAudioPath(audioRoot, rawPath);
              track.id = makeLookupKey(track.path.stem().string());
              track.weight = 1.0f;
            } else if (trackValue.is_object()) {
              track.id = makeLookupKey(trackValue.value("id", std::string()));
              track.path = resolveAudioPath(audioRoot,
                                            trackValue.value("path", std::string()));
              track.weight = std::max(0.0f, trackValue.value("weight", 1.0f));
              if (track.id.empty()) {
                track.id = makeLookupKey(track.path.stem().string());
              }
            }

            if (!track.id.empty() && !track.path.empty()) {
              definition.tracks.push_back(track);
            }
          }
        }

        musicContexts_.push_back(definition);
        musicContextLookup_[definition.name] = definition.id;
      }
    }
  } else {
    outWarnings.push_back(loadError);
  }

  return true;
}

const SoundEventDefinition* AudioDatabase::findEvent(SoundEventId id) const {
  if (!id.valid()) {
    return nullptr;
  }

  const std::size_t index = static_cast<std::size_t>(id.value - 1u);
  return index < events_.size() ? &events_[index] : nullptr;
}

const SoundEventDefinition* AudioDatabase::findEventByName(
    const std::string& name) const {
  const auto it = eventLookup_.find(makeLookupKey(name));
  return it != eventLookup_.end() ? findEvent(it->second) : nullptr;
}

const BlockSoundProfileDefinition* AudioDatabase::findBlockProfile(
    BlockSoundProfileId id) const {
  if (!id.valid()) {
    return nullptr;
  }

  const std::size_t index = static_cast<std::size_t>(id.value - 1u);
  return index < blockProfiles_.size() ? &blockProfiles_[index] : nullptr;
}

const BlockSoundProfileDefinition* AudioDatabase::findBlockProfileByBlockId(
    const std::string& blockId) const {
  const auto it = blockLookup_.find(makeLookupKey(blockId));
  if (it != blockLookup_.end()) {
    return findBlockProfile(it->second);
  }
  return findBlockProfile(fallbackBlockProfileId_);
}

SoundEventId AudioDatabase::resolveBlockAction(const std::string& blockId,
                                               BlockSoundAction action) const {
  const BlockSoundProfileDefinition* profile = findBlockProfileByBlockId(blockId);
  if (!profile) {
    return {};
  }
  return profile->actions[toIndex(action)];
}

const MusicContextDefinition* AudioDatabase::findMusicContext(
    MusicContextId id) const {
  if (!id.valid()) {
    return nullptr;
  }

  const std::size_t index = static_cast<std::size_t>(id.value - 1u);
  return index < musicContexts_.size() ? &musicContexts_[index] : nullptr;
}

const MusicContextDefinition* AudioDatabase::resolveMusicContext(
    const std::vector<std::string>& activeTags) const {
  const MusicContextDefinition* bestMatch = nullptr;
  std::unordered_set<std::string> tagSet;
  for (const std::string& tag : activeTags) {
    tagSet.insert(makeLookupKey(tag));
  }

  for (const MusicContextDefinition& context : musicContexts_) {
    bool matches = true;
    for (const std::string& requiredTag : context.requiredTags) {
      if (tagSet.find(requiredTag) == tagSet.end()) {
        matches = false;
        break;
      }
    }

    if (!matches) {
      continue;
    }

    if (!bestMatch || context.priority > bestMatch->priority) {
      bestMatch = &context;
    }
  }

  if (bestMatch) {
    return bestMatch;
  }

  for (const MusicContextDefinition& context : musicContexts_) {
    if (context.requiredTags.empty()) {
      return &context;
    }
  }

  return nullptr;
}

const CategoryDefinition* AudioDatabase::category(SoundCategoryId id) const {
  return &categories_[toIndex(id)];
}

std::string AudioDatabase::makeLookupKey(const std::string& value) {
  return normalizeAudioToken(value);
}

std::filesystem::path AudioDatabase::resolveAudioPath(
    const std::filesystem::path& audioRoot, const std::string& rawPath) {
  if (rawPath.empty()) {
    return {};
  }

  std::filesystem::path path(rawPath);
  if (path.is_absolute()) {
    return path;
  }

  const std::string lower = makeLookupKey(path.generic_string());
  if (lower.rfind("assets/", 0) == 0 || lower.rfind("res/", 0) == 0 ||
      lower.rfind("resources/", 0) == 0) {
    return AppPaths::resolve(path);
  }

  return audioRoot / path;
}

} // namespace AUDIO
} // namespace ENGINE
