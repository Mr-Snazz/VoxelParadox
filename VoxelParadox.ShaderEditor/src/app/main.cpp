#include <cstdio>

#include "editor/shader_editor_app.hpp"

int main() {
  std::printf("-------------------------------------------------------------\n");
  std::printf("VoxelParadox Shader Editor\n");
  std::printf("-------------------------------------------------------------\n");

  ShaderEditor::ShaderEditorApp app;
  return app.run() ? 0 : -1;
}
