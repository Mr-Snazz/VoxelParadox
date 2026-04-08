#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>

namespace ENGINE {
namespace AUDIO {

// OpenAL consumes decoded PCM buffers. The original file format is handled by
// the backend decoder before the audio manager uploads or streams the data.
struct PcmFormatInfo {
  ALenum alFormat = 0;
  std::uint16_t channels = 0;
  std::uint16_t bitsPerSample = 0;
  std::uint32_t sampleRate = 0;
  std::uint32_t bytesPerFrame = 0;
};

struct DecodedAudioData {
  PcmFormatInfo format{};
  std::vector<char> pcmData;
  double durationSeconds = 0.0;
};

class OpenAlContext {
public:
  ~OpenAlContext();

  bool initialize(std::string& outError);
  void shutdown();

  bool ready() const { return device_ != nullptr && context_ != nullptr; }
  ALCdevice* device() const { return device_; }
  ALCcontext* context() const { return context_; }

private:
  ALCdevice* device_ = nullptr;
  ALCcontext* context_ = nullptr;
};

// Supports .wav, .ogg and .mp3 and always returns decoded PCM suitable for
// OpenAL buffer uploads.
bool loadAudioFile(const std::filesystem::path& path, DecodedAudioData& outData,
                   std::string& outError);

// Simple chunk reader used by the music streamer. It decodes the source file
// once on open and then serves PCM data in fixed-size chunks.
class AudioStreamReader {
public:
  bool open(const std::filesystem::path& path, std::string& outError);
  bool open(const std::shared_ptr<const DecodedAudioData>& decodedData);
  void close();

  const PcmFormatInfo& format() const { return format_; }
  bool valid() const {
    if (format_.alFormat == 0) {
      return false;
    }
    if (usesFileStreaming_) {
      return streamedFile_.is_open() && streamDataSize_ > 0u;
    }
    return decodedData_ != nullptr && !decodedData_->pcmData.empty();
  }
  bool finished() const {
    if (usesFileStreaming_) {
      return !streamedFile_.is_open() || readOffset_ >= streamDataSize_;
    }
    return decodedData_ == nullptr || readOffset_ >= decodedData_->pcmData.size();
  }
  double durationSeconds() const { return durationSeconds_; }
  std::size_t read(char* destination, std::size_t maxBytes);
  void rewind();

private:
  std::shared_ptr<const DecodedAudioData> decodedData_;
  std::ifstream streamedFile_;
  PcmFormatInfo format_{};
  double durationSeconds_ = 0.0;
  std::size_t readOffset_ = 0u;
  std::uint64_t streamDataOffset_ = 0u;
  std::uint64_t streamDataSize_ = 0u;
  bool usesFileStreaming_ = false;
};

} // namespace AUDIO
} // namespace ENGINE
