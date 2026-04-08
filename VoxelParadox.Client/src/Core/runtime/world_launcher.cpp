// World launcher:
// - presents the pre-game world selection screen
// - creates or loads a world session
// - keeps all launcher rendering inside the game's HUD system

#include "runtime/world_launcher.hpp"

#include <chrono>
#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "engine/engine.hpp"
#include "engine/input.hpp"
#include "render/hud/hud.hpp"
#include "render/hud/hud_world_launcher.hpp"

namespace WorldLauncher {
namespace {

struct LauncherTaskResult {
  bool success = false;
  std::string error;
  WorldSaveService::WorldSession session{};
};

LauncherTaskResult runCreateTask(
    const std::string& worldName,
    const BiomeSelection& rootBiomeSelection) {
  LauncherTaskResult result;
  if (!WorldSaveService::createWorld(worldName, rootBiomeSelection, result.session,
                                     &result.error)) {
    return result;
  }

  result.success = true;
  return result;
}

LauncherTaskResult runLoadTask(const std::filesystem::path& worldDirectory) {
  LauncherTaskResult result;
  if (!WorldSaveService::loadPlayerAndWorldSession(worldDirectory,
                                                   result.session, &result.error)) {
    return result;
  }

  result.success = true;
  return result;
}

void restoreGameplayInput() {
  Input::enableTextInput(false);
  Input::setFocusMode(Input::FocusMode::GAMEPLAY);
  Input::setCursorVisible(false);
}

}  // namespace

RunResult run(GLFWwindow* window, const BiomeSelection& rootBiomeSelection,
              WorldSaveService::WorldSession& outSession,
              std::string* outError) {
  if (!window) {
    if (outError) {
      *outError = "Launcher window is not available.";
    }
    return RunResult::Error;
  }

  HUD::clear();
  HUD::setVisible(true);

  auto* launcherHud = new hudWorldLauncher();
  launcherHud->setWorlds(WorldSaveService::listWorlds());
  HUD::add(launcherHud);

  bool loading = false;
  std::future<LauncherTaskResult> taskFuture;
  std::string statusMessage;

  Input::setFocusMode(Input::FocusMode::UI);
  Input::setCursorVisible(true);
  Input::enableTextInput(true);

  double lastTime = glfwGetTime();
  while (!glfwWindowShouldClose(window)) {
    const double currentTime = glfwGetTime();
    const float rawDt = static_cast<float>(currentTime - lastTime);
    lastTime = currentTime;
    ENGINE::UPDATE(currentTime, rawDt);
    Input::update();

    if (loading && taskFuture.valid()) {
      const std::future_status status =
          taskFuture.wait_for(std::chrono::milliseconds(0));
      if (status == std::future_status::ready) {
        LauncherTaskResult result = taskFuture.get();
        loading = false;
        launcherHud->setLoading(false);
        if (result.success) {
          outSession = std::move(result.session);
          if (outError) {
            outError->clear();
          }
          restoreGameplayInput();
          HUD::clear();
          return RunResult::StartWorld;
        }

        statusMessage = result.error;
        launcherHud->setStatusMessage(statusMessage);
        launcherHud->setWorlds(WorldSaveService::listWorlds());
      }
    }

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    if (framebufferWidth > 0 && framebufferHeight > 0) {
      glViewport(0, 0, framebufferWidth, framebufferHeight);
      glClearColor(0.04f, 0.04f, 0.05f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      HUD::update(framebufferWidth, framebufferHeight);
      HUD::render(framebufferWidth, framebufferHeight);
    }

    if (!loading) {
      const hudWorldLauncher::ActionRequest request = launcherHud->consumeRequest();
      switch (request.type) {
      case hudWorldLauncher::ActionType::CreateWorld:
        loading = true;
        statusMessage.clear();
        launcherHud->setStatusMessage(statusMessage);
        launcherHud->setLoading(true);
        taskFuture = std::async(std::launch::async, [request, rootBiomeSelection]() {
          return runCreateTask(request.worldName, rootBiomeSelection);
        });
        break;
      case hudWorldLauncher::ActionType::LoadWorld:
        loading = true;
        statusMessage.clear();
        launcherHud->setStatusMessage(statusMessage);
        launcherHud->setLoading(true);
        taskFuture = std::async(std::launch::async, [request]() {
          return runLoadTask(request.worldDirectory);
        });
        break;
      case hudWorldLauncher::ActionType::ExitGame:
        restoreGameplayInput();
        HUD::clear();
        if (outError) {
          outError->clear();
        }
        return RunResult::ExitGame;
      case hudWorldLauncher::ActionType::None:
      default:
        break;
      }
    }

    glfwSwapBuffers(window);
  }

  restoreGameplayInput();
  HUD::clear();
  if (outError) {
    outError->clear();
  }
  return RunResult::ExitGame;
}

}  // namespace WorldLauncher
