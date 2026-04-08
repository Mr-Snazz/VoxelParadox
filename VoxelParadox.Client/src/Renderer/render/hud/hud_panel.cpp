// Arquivo: VoxelParadox.Client/src/Renderer/render/hud/hud_panel.cpp
// Papel: implementa "hud panel" dentro do subsistema "render hud" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma region Includes
#include "hud_panel.hpp"
#include "hud.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#pragma endregion

#pragma region HudPanelImplementation
// Funcao: executa 'hudPanel' no painel base do HUD.
// Detalhe: usa 'color', 'visible' para encapsular esta etapa especifica do subsistema.
hudPanel::hudPanel(const glm::vec4& color, const std::function<bool()>& visible)
    : color(color), visible(visible) {}

// Funcao: renderiza 'draw' no painel base do HUD.
// Detalhe: usa 'shader', 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
void hudPanel::draw(Shader& shader, int screenWidth, int screenHeight) {
    if (visible && !visible()) return;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3((float)screenWidth, (float)screenHeight, 1.0f));

    shader.setMat4("model", model);
    shader.setInt("isText", 0);
    shader.setVec4("color", color);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, HUD::getWhiteTexture());

    HUD::bindQuad();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    HUD::unbindQuad();
}
#pragma endregion
