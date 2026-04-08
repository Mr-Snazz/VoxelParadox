// Arquivo: Engine/src/engine/bootstrap.cpp
// Papel: implementa um componente do nÃºcleo compartilhado do engine.
// Fluxo: apoia bootstrap, timing, input, cÃ¢mera ou render base do projeto.
// DependÃªncias principais: GLFW, GLM e utilitÃ¡rios do engine.
#include "bootstrap.hpp"

// Arquivo: Engine/src/engine/bootstrap.cpp
// Papel: executa a validaÃƒÂ§ÃƒÂ£o do ambiente de runtime e sobe a janela principal com OpenGL.
// Fluxo: valida configuraÃƒÂ§ÃƒÂ£o, roots de recursos, assets obrigatÃƒÂ³rios e diretÃƒÂ³rio de save;
// depois inicializa GLFW, GLAD, viewport, VSync e input.
// DependÃƒÂªncias principais: `AppPaths`, `GraphicsSetup`, `Input`, GLFW, GLAD e Win32.

#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#ifdef FAR
#undef FAR
#endif
#ifdef NEAR
#undef NEAR
#endif
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "graphics_setup.hpp"
#include "input.hpp"

namespace Bootstrap {
namespace {

namespace ConsoleColor {
constexpr const char* RESET = "\x1b[0m";
constexpr const char* LIGHT_BLUE = "\x1b[96m";
constexpr const char* GRAY = "\x1b[90m";
constexpr const char* GREEN = "\x1b[92m";
constexpr const char* RED = "\x1b[91m";
} // namespace ConsoleColor

bool consoleColorsEnabled() {
  static const bool enabled = []() {
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console == INVALID_HANDLE_VALUE || console == nullptr) {
      return false;
    }

    DWORD mode = 0;
    if (!GetConsoleMode(console, &mode)) {
      return false;
    }

    if ((mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0) {
      return true;
    }

    return SetConsoleMode(console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
    return true;
#endif
  }();
  return enabled;
}

void printColoredLine(const char* color, const char* format, ...) {
  if (consoleColorsEnabled()) {
    std::printf("%s", color);
  }

  va_list args;
  va_start(args, format);
  std::vprintf(format, args);
  va_end(args);

  if (consoleColorsEnabled()) {
    std::printf("%s", ConsoleColor::RESET);
  }
  std::fflush(stdout);
}

void logInfo(const char* message) {
  printColoredLine(ConsoleColor::LIGHT_BLUE, "[INFO] %s\n", message);
}

void logSuccess(const char* message) {
  printColoredLine(ConsoleColor::GREEN, "[Bootstrap : SUCCESS] %s\n\n", message);
}

void logError(const char* message) {
  printColoredLine(ConsoleColor::RED, "[Bootstrap : ERROR] %s\n", message);
}

void printDetail(const char* label, const std::string& value) {
  printColoredLine(ConsoleColor::GRAY, "%s %s\n", label, value.c_str());
}

void printDetail(const char* label, int value) {
  printColoredLine(ConsoleColor::GRAY, "%s %d\n", label, value);
}

void printDetail(const char* label, int valueA, int valueB, const char* separator = "x") {
  printColoredLine(ConsoleColor::GRAY, "%s %d%s%d\n", label, valueA, separator, valueB);
}

bool verifyConfiguration(const Config& config) {
  // Falhas nesta etapa sÃƒÂ£o tratadas como erro de setup do executÃƒÂ¡vel, nÃƒÂ£o como falha de gameplay.
  logInfo("Checking bootstrap configuration...");

  printDetail("Window Title:", config.windowTitle);
  printDetail("Window Size:", config.windowSize.x, config.windowSize.y);
  printDetail("Window Type:", viewportModeName(config.viewportMode));
  printDetail("OpenGL Target:",
              std::to_string(config.openGLMajor) + "." +
                  std::to_string(config.openGLMinor));
  printDetail("Default Font:", config.defaultFontPath);
  printDetail("VSync:", config.vSyncEnabled ? "Enabled" : "Disabled");
  printDetail("Save Directory:", config.saveDirectory);

  if (config.windowTitle.empty()) {
    logError("Window title is empty.");
    return false;
  }
  if (config.windowSize.x <= 0 || config.windowSize.y <= 0) {
    logError("Window size is invalid.");
    return false;
  }
  if (config.openGLMajor <= 0 || config.openGLMinor < 0) {
    logError("OpenGL target is invalid.");
    return false;
  }
  if (config.defaultFontPath.empty()) {
    logError("Default font path is empty.");
    return false;
  }

  logSuccess("Configuration verified!");
  return true;
}

bool verifyRuntimeEnvironment() {
  logInfo("Checking the runtime environment...");

  std::error_code ec;
  AppPaths::initialize();
  const std::filesystem::path cwd = std::filesystem::current_path(ec);

  printDetail("Working Directory:", ec ? std::string("<unavailable>") : cwd.string());
  printDetail("Executable:", AppPaths::executablePath().string());
  printDetail("Workspace Root:", AppPaths::workspaceRoot().string());
  printDetail("Resources Root:", AppPaths::resourcesRoot().string());

  if (ec) {
    logError("Failed to resolve the working directory.");
    return false;
  }

  logSuccess("Runtime environment verified!");
  return true;
}

bool verifyAssets(const Config& config) {
  // O bootstrap falha cedo para evitar entrar no loop principal com recursos essenciais faltando.
  logInfo("Checking critical assets...");

  std::error_code ec;
  for (const std::string& dir : config.requiredAssetDirectories) {
    const std::filesystem::path path = AppPaths::resolve(dir);
    const bool exists = std::filesystem::exists(path, ec);
    const bool isDir = exists && std::filesystem::is_directory(path, ec);
    printDetail("Directory:", path.string());
    if (!exists || !isDir) {
      logError(("Missing required directory: " + path.string()).c_str());
      return false;
    }
  }

  const std::filesystem::path defaultFontPath =
      AppPaths::resolve(config.defaultFontPath);
  const bool fontExists = std::filesystem::exists(defaultFontPath, ec);
  const bool fontIsFile = fontExists && std::filesystem::is_regular_file(defaultFontPath, ec);
  const auto fontFileSize = fontIsFile ? std::filesystem::file_size(defaultFontPath, ec) : 0;
  printDetail("Default Font Asset:", defaultFontPath.string());
  if (!fontExists || !fontIsFile || fontFileSize == 0) {
    logError(("Missing or empty default font file: " + defaultFontPath.string()).c_str());
    return false;
  }

  for (const std::string& file : config.requiredAssetFiles) {
    const std::filesystem::path path = AppPaths::resolve(file);
    const bool exists = std::filesystem::exists(path, ec);
    const bool isFile = exists && std::filesystem::is_regular_file(path, ec);
    const auto fileSize = isFile ? std::filesystem::file_size(path, ec) : 0;
    printDetail("Asset File:", path.string());
    if (!exists || !isFile || fileSize == 0) {
      logError(("Missing or empty asset file: " + path.string()).c_str());
      return false;
    }
  }

  std::size_t ruinVoxCount = 0;
  const std::filesystem::path ruinsRoot = AppPaths::resolve("Assets/Voxs/ruins");
  for (const auto& entry :
       std::filesystem::recursive_directory_iterator(ruinsRoot, ec)) {
    if (ec) {
      break;
    }
    if (!entry.is_regular_file()) {
      continue;
    }
    if (entry.path().extension() == ".vox") {
      ruinVoxCount++;
    }
  }
  printDetail("Ruin VOX Files:", static_cast<int>(ruinVoxCount));
  if (ec || ruinVoxCount == 0) {
    logError("Ancient Ruins assets are missing or unreadable.");
    return false;
  }

  logSuccess("Critical assets verified!");
  return true;
}

bool ensureSaveWorldDirectory(const Config& config) {
  logInfo("Preparing save world data...");

  std::error_code ec;
  const std::filesystem::path savePath = AppPaths::resolve(config.saveDirectory);
  std::filesystem::create_directories(savePath, ec);
  printDetail("Save Directory:", savePath.string());

  if (ec || !std::filesystem::exists(savePath, ec) ||
      !std::filesystem::is_directory(savePath, ec)) {
    logError("Failed to prepare the save directory.");
    return false;
  }

  logSuccess("Save world directory ready!");
  return true;
}

void configureWindowHints(const Config& config) {
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, config.openGLMajor);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, config.openGLMinor);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_STENCIL_BITS, 8);

  ENGINE::SETDEFAULTVIEWPORTSIZE(
      glm::vec2(static_cast<float>(config.windowSize.x),
                static_cast<float>(config.windowSize.y)));
  ENGINE::SETVIEWPORTMODE(config.viewportMode);

  glfwWindowHint(GLFW_DECORATED,
                 config.viewportMode == ENGINE::VIEWPORTMODE::WINDOWMODE
                     ? GLFW_TRUE
                     : GLFW_FALSE);
}

GLFWwindow* createMainWindow(const Config& config) {
  const glm::vec2 defaultViewportSize = ENGINE::GETDEFAULTVIEWPORTSIZE();
  GLFWwindow* window =
      glfwCreateWindow(static_cast<int>(defaultViewportSize.x),
                       static_cast<int>(defaultViewportSize.y),
                       config.windowTitle.c_str(), nullptr, nullptr);
  if (!window) {
    return nullptr;
  }

  ENGINE::ATTACHWINDOW(window);
  glfwMakeContextCurrent(window);
  ENGINE::SETVSYNC(config.vSyncEnabled);
  return window;
}

void initializeInput(GLFWwindow* window) {
  Input::init(window);
  ENGINE::ADDPAUSELISTENER([](bool paused) { Input::setCursorVisible(paused); });
}

bool verifyMonitorSystem() {
  logInfo("Checking the monitor system...");

  GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
  if (!primaryMonitor) {
    logError("No primary monitor was detected.");
    return false;
  }

  const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);
  if (!videoMode) {
    logError("Failed to query the primary monitor video mode.");
    return false;
  }

  const char* monitorName = glfwGetMonitorName(primaryMonitor);
  printDetail("Primary Monitor:", monitorName ? monitorName : "Unknown");
  printDetail("Monitor Size:", videoMode->width, videoMode->height);

  logSuccess("Monitor system verified!");
  return true;
}

bool initializeGraphicsRuntime(const Config& config, GLFWwindow*& outWindow) {
  logInfo("Initializing GLFW...");
  if (!GraphicsSetup::initializeGLFW()) {
    logError("GLFW initialization failed.");
    return false;
  }
  logSuccess("GLFW Initialized!");

  if (!verifyMonitorSystem()) {
    glfwTerminate();
    return false;
  }

  logInfo("Preparing the window...");
  printDetail("Window Title:", config.windowTitle);
  printDetail("Window Size:", config.windowSize.x, config.windowSize.y);
  printDetail("Window Type:", viewportModeName(config.viewportMode));
  configureWindowHints(config);
  logSuccess("Window configuration prepared!");

  logInfo("Creating the main window...");
  outWindow = createMainWindow(config);
  if (!outWindow) {
    glfwTerminate();
    logError("Window creation failed.");
    return false;
  }
  logSuccess("Window Created!");

  logInfo("Initializing GLAD...");
  if (!GraphicsSetup::initializeGlad()) {
    shutdownWindow(outWindow);
    outWindow = nullptr;
    logError("GLAD initialization failed.");
    return false;
  }
  logSuccess("GLAD Initialized!");

  logInfo("Checking OpenGL runtime...");
  const char* version =
      reinterpret_cast<const char*>(glGetString(GL_VERSION));
  const char* renderer =
      reinterpret_cast<const char*>(glGetString(GL_RENDERER));
  const unsigned int hardwareThreads = std::thread::hardware_concurrency();

  printDetail("OpenGL Version:", version ? version : "Unknown");
  printDetail("OpenGL Renderer:", renderer ? renderer : "Unknown");
  printDetail("Hardware Threads:", static_cast<int>(hardwareThreads));

  if (!version || !renderer) {
    shutdownWindow(outWindow);
    outWindow = nullptr;
    logError("OpenGL runtime information is unavailable.");
    return false;
  }
  logSuccess("OpenGL runtime verified!");

  logInfo("Configuring OpenGL state...");
  // Todo o render do projeto assume depth test e face culling jÃƒÂ¡ configurados neste ponto.
  GraphicsSetup::configureOpenGLState();
  logSuccess("OpenGL state configured!");

  logInfo("Bootstrapping input system...");
  initializeInput(outWindow);
  logSuccess("Input system initialized!");

  return true;
}

} // namespace

bool initialize(const Config& config, GLFWwindow*& outWindow) {
  outWindow = nullptr;
  (void)consoleColorsEnabled();

  if (!verifyConfiguration(config) ||
      !verifyRuntimeEnvironment() ||
      !verifyAssets(config) ||
      !ensureSaveWorldDirectory(config)) {
    return false;
  }

  if (!initializeGraphicsRuntime(config, outWindow)) {
    return false;
  }

  return true;
}

void reportImGuiStatus(bool initialized) {
  logInfo("Initializing ImGui...");
  if (initialized) {
    logSuccess("ImGui initialized correctly!");
    return;
  }

  logError("ImGui failed to initialize.");
}

void shutdownWindow(GLFWwindow* window) {
  if (window) {
    glfwDestroyWindow(window);
  }
  glfwTerminate();
}

const char* viewportModeName(ENGINE::VIEWPORTMODE mode) {
  switch (mode) {
  case ENGINE::VIEWPORTMODE::WINDOWMODE:
    return "Windowed";
  case ENGINE::VIEWPORTMODE::BORDERLESS:
    return "Borderless";
  case ENGINE::VIEWPORTMODE::FULLSCREEN:
    return "Fullscreen";
  default:
    return "Unknown";
  }
}

} // namespace Bootstrap
