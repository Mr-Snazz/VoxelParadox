// Arquivo: VoxelParadox.Client/src/Renderer/render/hud/hud_watch_text.cpp
// Papel: implementa "hud watch text" dentro do subsistema "render hud" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma region Includes
#include "hud_watch_text.hpp"
#include "engine/engine.hpp"
#pragma endregion

#pragma region HudWatchTextImplementation
// Funcao: executa 'hudWatchText' no watcher de texto do HUD.
// Detalhe: usa 'binder', 'x', 'y', 'scale', 'fontSize', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
hudWatchText::hudWatchText(Binder binder, int x, int y, glm::vec2 scale,
                           int fontSize, double updateIntervalSeconds,
                           const std::string& fontPath)
    : hudText("", x, y, scale, fontSize, fontPath), binder(std::move(binder)),
      updateIntervalSeconds(updateIntervalSeconds) {
  buffer.reserve(64);
}

// Funcao: executa 'hudWatchText' no watcher de texto do HUD.
// Detalhe: usa 'binder', 'layout', 'scale', 'fontSize', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
hudWatchText::hudWatchText(Binder binder, const HUDLayout& layout, glm::vec2 scale,
                           int fontSize, double updateIntervalSeconds,
                           const std::string& fontPath)
    : hudText("", layout, scale, fontSize, fontPath), binder(std::move(binder)),
      updateIntervalSeconds(updateIntervalSeconds) {
  buffer.reserve(64);
}

// Funcao: renderiza 'draw' no watcher de texto do HUD.
// Detalhe: usa 'shader', 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
void hudWatchText::draw(Shader& shader, int screenWidth, int screenHeight) {
  if (binder) {
    const double now = ENGINE::GETTIME();
    const double interval = updateIntervalSeconds;
    const bool due = (interval <= 0.0) || (lastUpdateSeconds < 0.0) ||
                     ((now - lastUpdateSeconds) >= interval);
    if (due) {
      binder(buffer);
      setText(buffer);
      lastUpdateSeconds = now;
    }
  }
  if (visualBinder) {
    visualBinder(*this);
  }
  hudText::draw(shader, screenWidth, screenHeight);
}
#pragma endregion
