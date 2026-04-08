// Arquivo: Engine/src/engine/graphics_setup.h
// Papel: declara a inicialização mínima de GLFW, GLAD e estado base de OpenGL.
// Fluxo: fornece a superfície enxuta usada pelo bootstrap antes de o runtime subir o renderer.
// Dependências principais: GLFW, GLAD e OpenGL.
#pragma once

namespace GraphicsSetup {

bool initializeGLFW();
bool initializeGlad();
void configureOpenGLState();

} // namespace GraphicsSetup
