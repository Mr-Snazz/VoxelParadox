// Arquivo: VoxelParadox.Client/src/Renderer/render/hud/hud_watch_text.hpp
// Papel: declara "hud watch text" dentro do subsistema "render hud" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma once

#pragma region Includes
#include "hud_text.hpp"
#include <functional>
#include <string>
#pragma endregion

#pragma region HudWatchTextApi
class hudWatchText : public hudText {
public:
    using Binder = std::function<void(std::string&)>;
    using VisualBinder = std::function<void(hudWatchText&)>;

    // Funcao: executa 'hudWatchText' no watcher de texto do HUD.
    // Detalhe: usa 'binder', 'x', 'y', 'scale', 'fontSize', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
    hudWatchText(Binder binder, int x, int y, glm::vec2 scale, int fontSize,
                 double updateIntervalSeconds = 0.0,
                 const std::string& fontPath = "");
    // Funcao: executa 'hudWatchText' no watcher de texto do HUD.
    // Detalhe: usa 'binder', 'layout', 'scale', 'fontSize', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
    hudWatchText(Binder binder, const HUDLayout& layout, glm::vec2 scale, int fontSize,
                 double updateIntervalSeconds = 0.0,
                 const std::string& fontPath = "");

    // Funcao: renderiza 'draw' no watcher de texto do HUD.
    // Detalhe: usa 'shader', 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
    void draw(class Shader& shader, int screenWidth, int screenHeight) override;
    void setVisualBinder(VisualBinder newVisualBinder) {
        visualBinder = std::move(newVisualBinder);
    }

private:
    Binder binder;
    VisualBinder visualBinder;
    std::string buffer;
    double updateIntervalSeconds = 0.0;
    double lastUpdateSeconds = -1.0;
};
#pragma endregion
