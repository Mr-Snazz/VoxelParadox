// Arquivo: VoxelParadox.Client/src/Renderer/render/hud/hud_button_text.cpp
// Papel: implementa "hud button text" dentro do subsistema "render hud" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma region Includes
#include "hud_button_text.hpp"

#include "engine/input.hpp"
#include "hud_text.hpp"
#pragma endregion

#pragma region HudButtonTextImplementation
// Funcao: executa 'hudButtonText' no botao de texto do HUD.
// Detalhe: usa 'text', 'positioner', 'fontSize', 'normalColor', 'hoverColor', 'onClick', 'visible', 'fontPath' para encapsular esta etapa especifica do subsistema.
hudButtonText::hudButtonText(const std::string& text,
                             const Positioner& positioner,
                             int fontSize,
                             const glm::vec3& normalColor,
                             const glm::vec3& hoverColor,
                             const std::function<void()>& onClick,
                             const std::function<bool()>& visible,
                             const std::string& fontPath)
    : positioner(positioner), onClick(onClick), visible(visible),
      normalColor(normalColor), hoverColor(hoverColor) {
    label = new hudText(text, 0, 0, glm::vec2(1.0f), fontSize, fontPath);
    label->setColor(normalColor);
}

// Funcao: libera '~hudButtonText' no botao de texto do HUD.
// Detalhe: centraliza a logica necessaria para encerrar a etapa e descartar recursos associados.
// Retorno: devolve 'hudButtonText::' com o resultado composto por esta chamada.
hudButtonText::~hudButtonText() {
    delete label;
    label = nullptr;
}

// Funcao: executa 'layout' no botao de texto do HUD.
// Detalhe: usa 'screenWidth', 'screenHeight' para encapsular esta etapa especifica do subsistema.
void hudButtonText::layout(int screenWidth, int screenHeight) {
    if (!label) return;
    cachedSize = label->measure();
    if (positioner) {
        cachedPos = positioner(screenWidth, screenHeight, cachedSize);
        label->setPosition(cachedPos.x, cachedPos.y);
    } else {
        cachedPos = glm::ivec2(label->getX(), label->getY());
    }
}

// Funcao: executa 'hitTest' no botao de texto do HUD.
// Detalhe: usa 'mx', 'my' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool hudButtonText::hitTest(float mx, float my) const {
    return (mx >= (float)cachedPos.x && mx <= (float)cachedPos.x + cachedSize.x &&
            my >= (float)cachedPos.y && my <= (float)cachedPos.y + cachedSize.y);
}

// Funcao: atualiza 'update' no botao de texto do HUD.
// Detalhe: usa 'screenWidth', 'screenHeight' para sincronizar o estado derivado com o frame atual.
void hudButtonText::update(int screenWidth, int screenHeight) {
    if (visible && !visible()) return;
    if (!label) return;

    layout(screenWidth, screenHeight);

    float mx = 0.0f, my = 0.0f;
    Input::getMousePosFramebuffer(mx, my, screenWidth, screenHeight);

    hovered = hitTest(mx, my);
    label->setColor(hovered ? hoverColor : normalColor);

    if (hovered && onClick && Input::mousePressed(GLFW_MOUSE_BUTTON_LEFT)) {
        onClick();
    }
}

// Funcao: renderiza 'draw' no botao de texto do HUD.
// Detalhe: usa 'shader', 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
void hudButtonText::draw(Shader& shader, int screenWidth, int screenHeight) {
    if (visible && !visible()) return;
    if (!label) return;

    // Keep layout stable even if update() isn't called.
    layout(screenWidth, screenHeight);
    label->draw(shader, screenWidth, screenHeight);
}
#pragma endregion
