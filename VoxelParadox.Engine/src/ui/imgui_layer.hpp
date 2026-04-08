// Arquivo: Engine/src/ui/imgui_layer.h
// Papel: declara a integração compartilhada de ImGui do engine.
// Fluxo: expõe a API enxuta usada pelo editor para iniciar frame, renderizar e encerrar a camada ImGui.
// Dependências principais: ImGui, GLFW e OpenGL.
#pragma once

#include <string>

struct GLFWwindow;

namespace ImGuiLayer {

struct Config {
  bool dockingEnabled = false;
  std::string iniFilename{};
};

bool initialize(GLFWwindow* window, const Config& config = {});
void beginFrame();
void render();
void shutdown();

} // namespace ImGuiLayer
