// Arquivo: VoxelParadox.Client/src/Renderer/render/hud/hud_image.cpp
// Papel: implementa "hud image" dentro do subsistema "render hud" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma region Includes
#include "hud_image.hpp"
#include "hud.hpp"
#pragma endregion

#pragma region HudImageImplementation
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>

#include "path/app_paths.hpp"

// Funcao: executa 'hudImage' no elemento de imagem do HUD.
// Detalhe: usa 'path', 'x', 'y', 'scale', 'type', 'positioner' para encapsular esta etapa especifica do subsistema.
hudImage::hudImage(const std::string& path, int x, int y, glm::vec2 scale, ImageType type,
                   Positioner positioner)
    : textureID(0), posX(x), posY(y), scaleAmt(scale), scalingType(type),
      positioner(positioner), imgWidth(0), imgHeight(0), imgChannels(0)
{
    loadImage(path);
}

// Funcao: executa 'hudImage' no elemento de imagem do HUD.
// Detalhe: usa 'path', 'layout', 'scale', 'type' para encapsular esta etapa especifica do subsistema.
hudImage::hudImage(const std::string& path, const HUDLayout& layout, glm::vec2 scale,
                   ImageType type)
    : textureID(0), posX(0), posY(0), useLayout(true), layoutSpec(layout), scaleAmt(scale),
      scalingType(type), imgWidth(0), imgHeight(0), imgChannels(0) {
    loadImage(path);
}

// Funcao: libera '~hudImage' no elemento de imagem do HUD.
// Detalhe: centraliza a logica necessaria para encerrar a etapa e descartar recursos associados.
// Retorno: devolve 'hudImage::' com o resultado composto por esta chamada.
hudImage::~hudImage() {
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
    }
}

// Funcao: carrega 'loadImage' no elemento de imagem do HUD.
// Detalhe: usa 'path' para ler dados externos e adapta-los ao formato interno usado pelo jogo.
void hudImage::loadImage(const std::string& path) {
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Requested by user: interpolation set to Closest (Nearest Neighbor)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_set_flip_vertically_on_load(false);
    const std::string resolvedPath = AppPaths::resolveString(path);
    unsigned char *data = stbi_load(resolvedPath.c_str(), &imgWidth, &imgHeight, &imgChannels, 0);
    if (data) {
        GLenum format = GL_RGBA;
        if (imgChannels == 1) format = GL_RED;
        else if (imgChannels == 3) format = GL_RGB;
        else if (imgChannels == 4) format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, imgWidth, imgHeight, 0, format, GL_UNSIGNED_BYTE, data);
    } else {
        std::cerr << "hudImage: Failed to load texture " << resolvedPath << std::endl;
        textureID = 0;
    }
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Funcao: mede 'measureSize' no elemento de imagem do HUD.
// Detalhe: centraliza a logica necessaria para calcular o tamanho final necessario para layout ou ajuste visual.
// Retorno: devolve 'glm::vec2' com o resultado composto por esta chamada.
glm::vec2 hudImage::measureSize() const {
    float finalWidth = static_cast<float>(imgWidth);
    float finalHeight = static_cast<float>(imgHeight);

    if (scalingType == ImageType::STRETCH) {
        finalWidth = scaleAmt.x;
        finalHeight = scaleAmt.y;
    } else if (scalingType == ImageType::FILL) {
        if (imgWidth <= 0 || imgHeight <= 0) return glm::vec2(0.0f);
        float ratioX = scaleAmt.x / static_cast<float>(imgWidth);
        float ratioY = scaleAmt.y / static_cast<float>(imgHeight);
        float ratio = std::max(ratioX, ratioY);
        finalWidth = imgWidth * ratio;
        finalHeight = imgHeight * ratio;
    }
    return glm::vec2(finalWidth, finalHeight);
}

// Funcao: executa 'layout' no elemento de imagem do HUD.
// Detalhe: usa 'screenWidth', 'screenHeight' para encapsular esta etapa especifica do subsistema.
void hudImage::layout(int screenWidth, int screenHeight) {
    const glm::vec2 size = measureSize();
    if (useLayout) {
        const glm::vec2 resolved = resolveHUDPosition(layoutSpec, screenWidth, screenHeight, size);
        posX = static_cast<int>(std::round(resolved.x));
        posY = static_cast<int>(std::round(resolved.y));
        return;
    }
    if (!positioner) return;
    const glm::ivec2 pos = positioner(screenWidth, screenHeight, size);
    posX = pos.x;
    posY = pos.y;
}

// Funcao: executa 'onViewportChanged' no elemento de imagem do HUD.
// Detalhe: usa 'newWidth', 'newHeight', 'oldWidth', 'oldHeight' para encapsular esta etapa especifica do subsistema.
void hudImage::onViewportChanged(int newWidth, int newHeight, int oldWidth, int oldHeight) {
    (void)oldWidth;
    (void)oldHeight;
    layout(newWidth, newHeight);
}

// Funcao: renderiza 'draw' no elemento de imagem do HUD.
// Detalhe: usa 'shader', 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
void hudImage::draw(Shader& shader, int screenWidth, int screenHeight) {
    if (textureID == 0) return;

    const glm::vec2 finalSize = measureSize();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(posX, posY, 0.0f));
    model = glm::scale(model, glm::vec3(finalSize.x, finalSize.y, 1.0f));

    shader.setMat4("model", model);
    shader.setInt("isText", 0);
    shader.setVec4("color", glm::vec4(1.0f));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);

    HUD::bindQuad();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    HUD::unbindQuad();
}
#pragma endregion
