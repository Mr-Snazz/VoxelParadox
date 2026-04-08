#include "block_shader_session.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <optional>
#include <regex>
#include <sstream>

#include "path/app_paths.hpp"

namespace ShaderEditor {
namespace {

std::string trim(std::string value) {
  auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
  value.erase(value.begin(),
              std::find_if(value.begin(), value.end(), notSpace));
  value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(),
              value.end());
  return value;
}

std::optional<int> tryParseShaderLogLineNumber(const std::string& line) {
  static const std::regex kParenPattern(R"(0\((\d+)\))");
  static const std::regex kColonPattern(R"(0:(\d+):)");
  std::smatch match;
  if (std::regex_search(line, match, kParenPattern) && match.size() > 1) {
    return std::stoi(match[1].str());
  }
  if (std::regex_search(line, match, kColonPattern) && match.size() > 1) {
    return std::stoi(match[1].str());
  }
  return std::nullopt;
}

std::string toGenericString(const std::filesystem::path& path) {
  return path.lexically_normal().generic_string();
}

} // namespace

bool BlockShaderSession::initialize(double nowSeconds) {
  shaderRootAbs_ = AppPaths::shadersRoot();
  refreshAvailableFiles();
  observedFingerprint_ = readFingerprint();
  appliedFingerprint_ = observedFingerprint_;
  fingerprintInitialized_ = true;
  sourceDirty_ = false;
  return compileCurrentSelection(nowSeconds, "Initial load");
}

void BlockShaderSession::update(double nowSeconds) {
  const SourceFingerprint currentFingerprint = readFingerprint();
  if (!fingerprintInitialized_ || currentFingerprint != observedFingerprint_) {
    observedFingerprint_ = currentFingerprint;
    fingerprintInitialized_ = true;
    sourceDirty_ = true;
    lastChangeDetectedSeconds_ = nowSeconds;
    statusMessage_ = "Detected shader file change on disk.";
    refreshAvailableFiles();
  }

  if (!hotReloadEnabled_ || !sourceDirty_) {
    return;
  }

  const bool debounceElapsed =
      (nowSeconds - lastChangeDetectedSeconds_) >= kDebounceSeconds;
  const bool retryElapsed =
      (nowSeconds - lastAttemptSeconds_) >= kRetrySeconds;
  if (debounceElapsed && retryElapsed) {
    compileCurrentSelection(nowSeconds, "Hot reload");
  }
}

bool BlockShaderSession::requestReload(double nowSeconds, const char* trigger) {
  refreshAvailableFiles();
  observedFingerprint_ = readFingerprint();
  fingerprintInitialized_ = true;
  sourceDirty_ = false;
  return compileCurrentSelection(nowSeconds, trigger);
}

bool BlockShaderSession::trySelectFile(const std::filesystem::path& candidate,
                                       double nowSeconds) {
  const std::string normalized = toGenericString(candidate);
  if (normalized != "block.frag" &&
      normalized != "Assets/Shaders/block.frag") {
    diagnostics_.clear();
    rawLog_.clear();
    statusMessage_ =
        "Rejected shader selection: only block.frag inside Resources/Assets/Shaders "
        "is supported, and block.vert is loaded automatically.";
    lastAttemptText_ = formatCurrentTimestamp();
    lastAttemptSeconds_ = nowSeconds;
    diagnostics_.push_back(
        {"selection", shaderRootAbs_ / candidate, -1, statusMessage_});
    sourceDirty_ = false;
    return false;
  }

  fragmentPath_ = std::filesystem::path("Assets/Shaders/block.frag");
  vertexPath_ = std::filesystem::path("Assets/Shaders/block.vert");
  return requestReload(nowSeconds, "Selected block.frag");
}

void BlockShaderSession::refreshAvailableFiles() {
  availableFiles_.clear();

  std::error_code ec;
  if (!std::filesystem::exists(shaderRootAbs_, ec)) {
    return;
  }

  for (const auto& entry : std::filesystem::directory_iterator(shaderRootAbs_, ec)) {
    if (ec || !entry.is_regular_file()) {
      continue;
    }

    const std::filesystem::path extension = entry.path().extension();
    if (extension != ".frag" && extension != ".vert") {
      continue;
    }

    availableFiles_.push_back(
        {entry.path().filename(),
         entry.path().filename() == std::filesystem::path("block.frag")});
  }

  std::sort(availableFiles_.begin(), availableFiles_.end(),
            [](const ShaderDirectoryEntry& a, const ShaderDirectoryEntry& b) {
              return a.relativePath.generic_string() < b.relativePath.generic_string();
            });
}

BlockShaderSession::SourceFingerprint BlockShaderSession::readFingerprint() const {
  SourceFingerprint fingerprint;

  std::error_code ec;
  const std::filesystem::path vertexAbs = AppPaths::resolve(vertexPath_);
  fingerprint.vertexExists = std::filesystem::exists(vertexAbs, ec);
  if (!ec && fingerprint.vertexExists) {
    fingerprint.vertexWrite = std::filesystem::last_write_time(vertexAbs, ec);
  }

  ec.clear();
  const std::filesystem::path fragmentAbs = AppPaths::resolve(fragmentPath_);
  fingerprint.fragmentExists = std::filesystem::exists(fragmentAbs, ec);
  if (!ec && fingerprint.fragmentExists) {
    fingerprint.fragmentWrite = std::filesystem::last_write_time(fragmentAbs, ec);
  }

  return fingerprint;
}

bool BlockShaderSession::compileCurrentSelection(double nowSeconds,
                                                 const char* trigger) {
  lastAttemptSeconds_ = nowSeconds;
  lastAttemptText_ = formatCurrentTimestamp();
  diagnostics_.clear();
  rawLog_.clear();

  const std::filesystem::path vertexAbs = AppPaths::resolve(vertexPath_);
  const std::filesystem::path fragmentAbs = AppPaths::resolve(fragmentPath_);

  std::string vertexSource;
  std::string fragmentSource;
  std::string ioError;

  if (!readTextFile(vertexAbs, vertexSource, ioError)) {
    valid_ = false;
    shader_.release();
    statusMessage_ =
        "Vertex shader read failed. The preview will stay alive with the fallback shader.";
    rawLog_ = ioError;
    diagnostics_.push_back({"io", vertexAbs, -1, ioError});
    sourceDirty_ = true;
    return false;
  }

  if (!readTextFile(fragmentAbs, fragmentSource, ioError)) {
    valid_ = false;
    shader_.release();
    statusMessage_ =
        "Fragment shader read failed. The preview will stay alive with the fallback shader.";
    rawLog_ = ioError;
    diagnostics_.push_back({"io", fragmentAbs, -1, ioError});
    sourceDirty_ = true;
    return false;
  }

  GLuint vertexShader = 0;
  GLuint fragmentShader = 0;
  std::vector<ShaderDiagnostic> compileDiagnostics;
  std::string compileLog;

  if (!compileStage(GL_VERTEX_SHADER, vertexSource, vertexAbs, "vertex",
                    vertexShader, compileLog, compileDiagnostics)) {
    valid_ = false;
    shader_.release();
    statusMessage_ =
        "Vertex shader compilation failed. Showing the fallback magenta preview.";
    rawLog_ = compileLog;
    diagnostics_ = std::move(compileDiagnostics);
    sourceDirty_ = true;
    return false;
  }

  if (!compileStage(GL_FRAGMENT_SHADER, fragmentSource, fragmentAbs, "fragment",
                    fragmentShader, compileLog, compileDiagnostics)) {
    valid_ = false;
    shader_.release();
    glDeleteShader(vertexShader);
    statusMessage_ =
        "Fragment shader compilation failed. Showing the fallback magenta preview.";
    rawLog_ = compileLog;
    diagnostics_ = std::move(compileDiagnostics);
    sourceDirty_ = true;
    return false;
  }

  GLuint program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  GLint linkStatus = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  if (linkStatus != GL_TRUE) {
    valid_ = false;
    shader_.release();
    rawLog_ = readProgramLog(program);
    glDeleteProgram(program);
    statusMessage_ =
        "Shader link failed. Showing the fallback magenta preview.";
    diagnostics_.push_back({"link", fragmentAbs, -1, rawLog_});
    sourceDirty_ = true;
    return false;
  }

  shader_.release();
  shader_.program = program;
  valid_ = true;
  sourceDirty_ = false;
  appliedFingerprint_ = observedFingerprint_;
  lastSuccessText_ = formatCurrentTimestamp();
  statusMessage_ = std::string("Shader compiled successfully (") + trigger + ").";
  diagnostics_.clear();
  rawLog_.clear();
  return true;
}

std::string BlockShaderSession::formatCurrentTimestamp() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
  std::tm localTime{};
#ifdef _WIN32
  localtime_s(&localTime, &nowTime);
#else
  localTime = *std::localtime(&nowTime);
#endif

  std::ostringstream output;
  output << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
  return output.str();
}

bool BlockShaderSession::readTextFile(const std::filesystem::path& path,
                                      std::string& outText,
                                      std::string& outError) {
  std::ifstream input(path, std::ios::in | std::ios::binary);
  if (!input.is_open()) {
    outText.clear();
    outError = "Failed to open shader file: " + path.string();
    return false;
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  outText = buffer.str();
  outError.clear();
  return true;
}

std::string BlockShaderSession::readShaderLog(GLuint shader) {
  GLint logLength = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
  if (logLength <= 1) {
    return {};
  }

  std::string log(static_cast<std::size_t>(logLength), '\0');
  glGetShaderInfoLog(shader, logLength, nullptr, log.data());
  return trim(log);
}

std::string BlockShaderSession::readProgramLog(GLuint program) {
  GLint logLength = 0;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
  if (logLength <= 1) {
    return {};
  }

  std::string log(static_cast<std::size_t>(logLength), '\0');
  glGetProgramInfoLog(program, logLength, nullptr, log.data());
  return trim(log);
}

bool BlockShaderSession::compileStage(
    GLenum shaderType, const std::string& source,
    const std::filesystem::path& filePath, const char* stageName, GLuint& outShader,
    std::string& outLog, std::vector<ShaderDiagnostic>& outDiagnostics) {
  outShader = glCreateShader(shaderType);
  const char* sourcePtr = source.c_str();
  glShaderSource(outShader, 1, &sourcePtr, nullptr);
  glCompileShader(outShader);

  GLint compileStatus = GL_FALSE;
  glGetShaderiv(outShader, GL_COMPILE_STATUS, &compileStatus);
  if (compileStatus == GL_TRUE) {
    outLog.clear();
    return true;
  }

  outLog = readShaderLog(outShader);
  appendDiagnosticsFromLog(outDiagnostics, outLog, filePath, stageName);
  glDeleteShader(outShader);
  outShader = 0;
  return false;
}

void BlockShaderSession::appendDiagnosticsFromLog(
    std::vector<ShaderDiagnostic>& outDiagnostics, const std::string& log,
    const std::filesystem::path& filePath, const char* stageName) {
  std::istringstream lines(log);
  std::string line;
  while (std::getline(lines, line)) {
    line = trim(line);
    if (line.empty()) {
      continue;
    }

    ShaderDiagnostic diagnostic;
    diagnostic.stage = stageName;
    diagnostic.filePath = filePath;
    diagnostic.message = line;
    if (const std::optional<int> parsedLine = tryParseShaderLogLineNumber(line)) {
      diagnostic.line = *parsedLine;
    }
    outDiagnostics.push_back(std::move(diagnostic));
  }

  if (outDiagnostics.empty()) {
    outDiagnostics.push_back({stageName, filePath, -1, trim(log)});
  }
}

} // namespace ShaderEditor
