// Arquivo: Engine/src/engine/engine.h
// Papel: declara um componente do núcleo compartilhado do engine.
// Fluxo: apoia bootstrap, timing, input, câmera ou render base do projeto.
// Dependências principais: GLFW, GLM e utilitários do engine.
#pragma once

// Arquivo: Engine/src/engine/engine.h
// Papel: declara o estado global leve do runtime para timing, viewport, pausa e mÃ©tricas.
// Fluxo: o bootstrap anexa a janela, o loop chama `UPDATE` a cada frame e os subsistemas
// consultam FPS, tempos e modo de viewport sem carregar dependÃªncia direta de GLFW.
// DependÃªncias principais: GLFW, GLM e `std::function`.

#include <cstdint>
#include <functional>
#include <glm/vec2.hpp>

struct GLFWwindow;

namespace ENGINE {

enum class VIEWPORTMODE {
  WINDOWMODE = 0,
  BORDERLESS,
  FULLSCREEN
};

// Must be called once (or UPDATE will auto-init).
void INIT(double nowSeconds);

// Call once per frame (typically right after computing dt).
void UPDATE(double nowSeconds, float dtSeconds);

// Stats / timing
float GETFPS();
float GETDELTATIME();
float GETFRAMETIMEMS();
double GETTIME();
double GETTIMESINCESTART();
std::uint64_t GETFRAMECOUNT();
float GETCPUFRAMETIMEMS();
float GETGPUFRAMETIMEMS();
float GETCHUNKRENDERTIMEMS();
int GETRENDEREDCHUNKCOUNT();

// Performance instrumentation
void BEGINPERFFRAME();
void BEGINGPUFRAMEQUERY();
void ENDGPUFRAMEQUERY();
void ACCUMULATECHUNKRENDER(float totalMs, int chunkCount);
void ENDPERFFRAME(float cpuFrameMs, float fpsInstant);
void CLEANUPPERF();

// FPS sampling configuration (default: 1.0s = 1000ms true sample window)
void SETFPSSAMPLEINTERVAL(double seconds);
double GETFPSSAMPLEINTERVAL();

// Pause state + global pause notifications
bool ISPAUSED();
void SETPAUSED(bool paused);
void TOGGLEPAUSED();

using PauseListener = std::function<void(bool paused)>;
std::uint64_t ADDPAUSELISTENER(PauseListener listener);
void REMPAUSELISTENER(std::uint64_t id);

// Window / viewport state
glm::vec2 GETDEFAULTVIEWPORTSIZE();
glm::vec2 GETVIEWPORTSIZE();
VIEWPORTMODE GETVIEWPORTMODE();
glm::vec2 GETMONITORSIZE();
glm::vec2 GETWINDOWPOS();
bool ISVSYNCENABLED();

void SETDEFAULTVIEWPORTSIZE(const glm::vec2& size);
void SETVIEWPORTSIZE(const glm::vec2& size);
void SETVIEWPORTMODE(VIEWPORTMODE mode);
void SETVSYNC(bool enabled);
void ATTACHWINDOW(GLFWwindow* window);
void SHUTDOWN();

} // namespace ENGINE
