#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "engine/shader.hpp"

namespace ShaderEditor {

struct ShaderDiagnostic {
  std::string stage;
  std::filesystem::path filePath{};
  int line = -1;
  std::string message;
};

struct ShaderDirectoryEntry {
  std::filesystem::path relativePath{};
  bool supported = false;
};

class BlockShaderSession {
public:
  bool initialize(double nowSeconds);
  void update(double nowSeconds);
  bool requestReload(double nowSeconds, const char* trigger = "Manual reload");
  bool trySelectFile(const std::filesystem::path& candidate, double nowSeconds);
  void refreshAvailableFiles();

  void setHotReloadEnabled(bool enabled) { hotReloadEnabled_ = enabled; }
  bool hotReloadEnabled() const { return hotReloadEnabled_; }

  const std::filesystem::path& fragmentPath() const { return fragmentPath_; }
  const std::filesystem::path& vertexPath() const { return vertexPath_; }
  const Shader* activeShader() const { return valid_ ? &shader_ : nullptr; }
  bool hasValidShader() const { return valid_; }
  bool sourceDirty() const { return sourceDirty_; }

  const std::string& statusMessage() const { return statusMessage_; }
  const std::string& lastAttemptText() const { return lastAttemptText_; }
  const std::string& lastSuccessText() const { return lastSuccessText_; }
  const std::vector<ShaderDiagnostic>& diagnostics() const { return diagnostics_; }
  const std::string& rawLog() const { return rawLog_; }
  const std::vector<ShaderDirectoryEntry>& availableFiles() const {
    return availableFiles_;
  }

private:
  struct SourceFingerprint {
    bool vertexExists = false;
    bool fragmentExists = false;
    std::filesystem::file_time_type vertexWrite{};
    std::filesystem::file_time_type fragmentWrite{};

    bool operator==(const SourceFingerprint& other) const {
      return vertexExists == other.vertexExists &&
             fragmentExists == other.fragmentExists &&
             (!vertexExists || vertexWrite == other.vertexWrite) &&
             (!fragmentExists || fragmentWrite == other.fragmentWrite);
    }

    bool operator!=(const SourceFingerprint& other) const {
      return !(*this == other);
    }
  };

  static constexpr double kDebounceSeconds = 0.20;
  static constexpr double kRetrySeconds = 0.75;

  std::filesystem::path shaderRootAbs_{};
  std::filesystem::path fragmentPath_{"Assets/Shaders/block.frag"};
  std::filesystem::path vertexPath_{"Assets/Shaders/block.vert"};

  Shader shader_{};
  std::vector<ShaderDirectoryEntry> availableFiles_{};
  std::vector<ShaderDiagnostic> diagnostics_{};
  std::string rawLog_{};
  std::string statusMessage_{"Waiting for the initial shader compile."};
  std::string lastAttemptText_{"Never"};
  std::string lastSuccessText_{"Never"};

  bool valid_ = false;
  bool hotReloadEnabled_ = true;
  bool sourceDirty_ = false;
  bool fingerprintInitialized_ = false;
  double lastChangeDetectedSeconds_ = 0.0;
  double lastAttemptSeconds_ = -1000.0;
  SourceFingerprint observedFingerprint_{};
  SourceFingerprint appliedFingerprint_{};

  SourceFingerprint readFingerprint() const;
  bool compileCurrentSelection(double nowSeconds, const char* trigger);
  static std::string formatCurrentTimestamp();
  static bool readTextFile(const std::filesystem::path& path, std::string& outText,
                           std::string& outError);
  static std::string readShaderLog(GLuint shader);
  static std::string readProgramLog(GLuint program);
  static bool compileStage(GLenum shaderType, const std::string& source,
                           const std::filesystem::path& filePath,
                           const char* stageName, GLuint& outShader,
                           std::string& outLog,
                           std::vector<ShaderDiagnostic>& outDiagnostics);
  static void appendDiagnosticsFromLog(
      std::vector<ShaderDiagnostic>& outDiagnostics, const std::string& log,
      const std::filesystem::path& filePath, const char* stageName);
};

} // namespace ShaderEditor
