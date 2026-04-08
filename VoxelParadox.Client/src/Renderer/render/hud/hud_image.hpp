// Arquivo: VoxelParadox.Client/src/Renderer/render/hud/hud_image.hpp
// Papel: declara "hud image" dentro do subsistema "render hud" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma once

#pragma region Includes
#include "hud_element.hpp"
#include <functional>
#include <string>
#include <glm/glm.hpp>
#pragma endregion

#pragma region HudImageApi
class hudImage : public HUDElement {
public:
    using Positioner = std::function<glm::ivec2(int screenWidth, int screenHeight, glm::vec2 imageSize)>;

    // Funcao: executa 'hudImage' no elemento de imagem do HUD.
    // Detalhe: usa 'path', 'x', 'y', 'scale', 'type', 'positioner' para encapsular esta etapa especifica do subsistema.
    hudImage(const std::string& path, int x, int y, glm::vec2 scale, ImageType type,
             Positioner positioner = {});
    // Funcao: executa 'hudImage' no elemento de imagem do HUD.
    // Detalhe: usa 'path', 'layout', 'scale', 'type' para encapsular esta etapa especifica do subsistema.
    hudImage(const std::string& path, const HUDLayout& layout, glm::vec2 scale, ImageType type);
    // Funcao: libera '~hudImage' no elemento de imagem do HUD.
    // Detalhe: centraliza a logica necessaria para encerrar a etapa e descartar recursos associados.
    ~hudImage() override;

    // Funcao: executa 'onViewportChanged' no elemento de imagem do HUD.
    // Detalhe: usa 'newWidth', 'newHeight', 'oldWidth', 'oldHeight' para encapsular esta etapa especifica do subsistema.
    void onViewportChanged(int newWidth, int newHeight, int oldWidth, int oldHeight) override;
    // Funcao: renderiza 'draw' no elemento de imagem do HUD.
    // Detalhe: usa 'shader', 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
    void draw(class Shader& shader, int screenWidth, int screenHeight) override;

private:
    unsigned int textureID;
    int posX;
    int posY;
    bool useLayout = false;
    HUDLayout layoutSpec{};
    glm::vec2 scaleAmt;
    ImageType scalingType;
    Positioner positioner;
    int imgWidth;
    int imgHeight;
    int imgChannels;

    // Funcao: carrega 'loadImage' no elemento de imagem do HUD.
    // Detalhe: usa 'path' para ler dados externos e adapta-los ao formato interno usado pelo jogo.
    void loadImage(const std::string& path);
    // Funcao: mede 'measureSize' no elemento de imagem do HUD.
    // Detalhe: centraliza a logica necessaria para calcular o tamanho final necessario para layout ou ajuste visual.
    // Retorno: devolve 'glm::vec2' com o resultado composto por esta chamada.
    glm::vec2 measureSize() const;
    // Funcao: executa 'layout' no elemento de imagem do HUD.
    // Detalhe: usa 'screenWidth', 'screenHeight' para encapsular esta etapa especifica do subsistema.
    void layout(int screenWidth, int screenHeight);
};
#pragma endregion
