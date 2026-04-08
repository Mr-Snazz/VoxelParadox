// Arquivo: VoxelParadox.Client/src/Renderer/render/hud/hud_panel.hpp
// Papel: declara "hud panel" dentro do subsistema "render hud" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma once

#pragma region Includes
#include "hud_element.hpp"
#include <functional>
#include <glm/glm.hpp>
#pragma endregion

#pragma region HudPanelApi
class hudPanel : public HUDElement {
public:
    // Fullscreen panel by default.
    // Funcao: executa 'hudPanel' no painel base do HUD.
    // Detalhe: usa 'color', 'visible' para encapsular esta etapa especifica do subsistema.
    explicit hudPanel(const glm::vec4& color,
                      const std::function<bool()>& visible = {});
    // Funcao: executa 'hudPanel' no painel base do HUD.
    // Detalhe: usa 'color', 'layout', 'size', 'visible' para encapsular esta etapa especifica do subsistema.
    hudPanel(const glm::vec4& color, const HUDLayout& layout, glm::vec2 size,
             const std::function<bool()>& visible = {});

    // Funcao: renderiza 'draw' no painel base do HUD.
    // Detalhe: usa 'shader', 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
    void draw(class Shader& shader, int screenWidth, int screenHeight) override;

private:
    glm::vec4 color;
    bool useLayout = false;
    HUDLayout layoutSpec{};
    glm::vec2 sizeAmt{0.0f};
    // Funcao: executa 'bool' no painel base do HUD.
    // Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
    // Retorno: devolve 'std::function<' com o resultado composto por esta chamada.
    std::function<bool()> visible;
};
#pragma endregion
