// Arquivo: Engine/src/engine/input.cpp
// Papel: implementa um componente do nÃºcleo compartilhado do engine.
// Fluxo: apoia bootstrap, timing, input, cÃ¢mera ou render base do projeto.
// DependÃªncias principais: GLFW, GLM e utilitÃ¡rios do engine.
#include "input.hpp"

// Arquivo: Engine/src/engine/input.cpp
// Papel: define o armazenamento estÃƒÂ¡tico do sistema de input.
// Fluxo: este arquivo sÃƒÂ³ materializa o estado compartilhado declarado em `input.h`.

GLFWwindow* Input::window = nullptr;
bool Input::keys[512] = {};
bool Input::prevKeys[512] = {};
bool Input::mouseBtn[8] = {};
bool Input::prevMouseBtn[8] = {};
float Input::lastMX = 0, Input::lastMY = 0;
float Input::mouseDX = 0, Input::mouseDY = 0;
float Input::scrollY = 0;
float Input::mouseX = 0;
float Input::mouseY = 0;
bool Input::firstMouse = true;
bool Input::textInputEnabled = false;
bool Input::cursorVisible = false;
Input::FocusMode Input::focusMode = Input::FocusMode::GAMEPLAY;
std::string Input::typedChars;
