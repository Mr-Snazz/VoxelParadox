// Arquivo: VoxelParadox.Client/src/Renderer/render/hud/hud_hotbar.hpp
// Papel: declara "hud hotbar" dentro do subsistema "render hud" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma once

#pragma region Includes
#include "hud_element.hpp"
#include "hud_text.hpp"
#pragma endregion

#pragma region HudHotbarApi
class Player;
class Renderer;
class WorldStack;

enum class HotbarVisualPart {
    BACKGROUND = 0,
    SELECTION,
    COUNTS,
};

class hudHotbar : public HUDElement {
public:
    // Funcao: executa 'hudHotbar' na exibicao visual da hotbar.
    // Detalhe: usa 'player', 'part' para encapsular esta etapa especifica do subsistema.
    hudHotbar(const Player* player, HotbarVisualPart part);
    // Funcao: renderiza 'draw' na exibicao visual da hotbar.
    // Detalhe: usa 'shader', 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
    void draw(class Shader& shader, int screenWidth, int screenHeight) override;

private:
    const Player* player = nullptr;
    HotbarVisualPart part = HotbarVisualPart::BACKGROUND;
    hudText countText;

    // Funcao: renderiza 'drawRect' na exibicao visual da hotbar.
    // Detalhe: usa 'shader', 'rect', 'color' para desenhar a saida visual correspondente usando o estado atual.
    void drawRect(class Shader& shader, const glm::ivec4& rect, const glm::vec4& color);
    // Funcao: renderiza 'drawBackground' na exibicao visual da hotbar.
    // Detalhe: usa 'shader', 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
    void drawBackground(class Shader& shader, int screenWidth, int screenHeight);
    // Funcao: renderiza 'drawSelection' na exibicao visual da hotbar.
    // Detalhe: usa 'shader', 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
    void drawSelection(class Shader& shader, int screenWidth, int screenHeight);
    // Funcao: renderiza 'drawCounts' na exibicao visual da hotbar.
    // Detalhe: usa 'shader', 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
    void drawCounts(class Shader& shader, int screenWidth, int screenHeight);
};

class hudHotbarPreview : public HUDElement {
public:
    // Funcao: executa 'hudHotbarPreview' na exibicao visual da hotbar.
    // Detalhe: usa 'renderer', 'player', 'worldStack' para encapsular esta etapa especifica do subsistema.
    hudHotbarPreview(Renderer* renderer, const Player* player, const WorldStack* worldStack);
    // Funcao: renderiza 'draw' na exibicao visual da hotbar.
    // Detalhe: usa 'shader', 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
    void draw(class Shader& shader, int screenWidth, int screenHeight) override;

private:
    Renderer* renderer = nullptr;
    const Player* player = nullptr;
    const WorldStack* worldStack = nullptr;
};
#pragma endregion
