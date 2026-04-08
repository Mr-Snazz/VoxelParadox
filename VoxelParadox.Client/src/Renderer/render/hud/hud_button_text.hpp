// Arquivo: VoxelParadox.Client/src/Renderer/render/hud/hud_button_text.hpp
// Papel: declara "hud button text" dentro do subsistema "render hud" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma once

#pragma region Includes
#include "hud_element.hpp"
#include <functional>
#include <glm/glm.hpp>
#include <string>
#pragma endregion

#pragma region HudButtonTextApi
class hudText;

// Clickable text label.
class hudButtonText : public HUDElement {
public:
    // positioner returns the top-left position in framebuffer pixel coords.
    using Positioner = std::function<glm::ivec2(int screenWidth, int screenHeight, glm::vec2 textSize)>;

    // Funcao: executa 'hudButtonText' no botao de texto do HUD.
    // Detalhe: usa 'text', 'positioner', 'fontSize', 'normalColor', 'hoverColor', 'onClick', 'visible', 'fontPath' para encapsular esta etapa especifica do subsistema.
    hudButtonText(const std::string& text,
                  const Positioner& positioner,
                  int fontSize,
                  const glm::vec3& normalColor,
                  const glm::vec3& hoverColor,
                  const std::function<void()>& onClick,
                  const std::function<bool()>& visible = {},
                  const std::string& fontPath = "");

    // Funcao: libera '~hudButtonText' no botao de texto do HUD.
    // Detalhe: centraliza a logica necessaria para encerrar a etapa e descartar recursos associados.
    ~hudButtonText() override;

    // Funcao: atualiza 'update' no botao de texto do HUD.
    // Detalhe: usa 'screenWidth', 'screenHeight' para sincronizar o estado derivado com o frame atual.
    void update(int screenWidth, int screenHeight) override;
    // Funcao: renderiza 'draw' no botao de texto do HUD.
    // Detalhe: usa 'shader', 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
    void draw(class Shader& shader, int screenWidth, int screenHeight) override;

private:
    hudText* label = nullptr;
    Positioner positioner;
    // Funcao: executa 'void' no botao de texto do HUD.
    // Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
    // Retorno: devolve 'std::function<' com o resultado composto por esta chamada.
    std::function<void()> onClick;
    // Funcao: executa 'bool' no botao de texto do HUD.
    // Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
    // Retorno: devolve 'std::function<' com o resultado composto por esta chamada.
    std::function<bool()> visible;
    glm::vec3 normalColor;
    glm::vec3 hoverColor;

    glm::ivec2 cachedPos{0};
    glm::vec2 cachedSize{0.0f};
    bool hovered = false;

    // Funcao: executa 'layout' no botao de texto do HUD.
    // Detalhe: usa 'screenWidth', 'screenHeight' para encapsular esta etapa especifica do subsistema.
    void layout(int screenWidth, int screenHeight);
    // Funcao: executa 'hitTest' no botao de texto do HUD.
    // Detalhe: usa 'mx', 'my' para encapsular esta etapa especifica do subsistema.
    // Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
    bool hitTest(float mx, float my) const;
};
#pragma endregion
