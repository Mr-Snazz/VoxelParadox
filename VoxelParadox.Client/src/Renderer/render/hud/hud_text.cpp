// Arquivo: VoxelParadox.Client/src/Renderer/render/hud/hud_text.cpp
// Papel: implementa "hud text" dentro do subsistema "render hud" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma region Includes
#include "hud_text.hpp"
#include "hud.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "path/app_paths.hpp"
#pragma endregion

#pragma region HudTextLifecycle
// Funcao: executa 'hudText' no elemento de texto do HUD.
// Detalhe: usa 'text', 'x', 'y', 'scale', 'fontSize', 'fontPath' para encapsular esta etapa especifica do subsistema.
hudText::hudText(const std::string& text, int x, int y, glm::vec2 scale,
                 int fontSize, const std::string& fontPath)
    : text(text),
      posX(x),
      posY(y),
      scaleAmt(scale),
      fontSize(fontSize),
      fontPath(fontPath),
      color(1.0f, 1.0f, 1.0f) {
    if (this->fontPath.empty()) {
        this->fontPath = HUD::getDefaultFont();
    }
    loadFont();
    setupBuffers();
}

// Funcao: executa 'hudText' no elemento de texto do HUD.
// Detalhe: usa 'text', 'layout', 'scale', 'fontSize', 'fontPath' para encapsular esta etapa especifica do subsistema.
hudText::hudText(const std::string& text, const HUDLayout& layout, glm::vec2 scale,
                 int fontSize, const std::string& fontPath)
    : text(text),
      posX(0),
      posY(0),
      useLayout(true),
      layout(layout),
      scaleAmt(scale),
      fontSize(fontSize),
      fontPath(fontPath),
      color(1.0f, 1.0f, 1.0f) {
    if (this->fontPath.empty()) {
        this->fontPath = HUD::getDefaultFont();
    }
    loadFont();
    setupBuffers();
}

hudText::~hudText() {
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
}

// Funcao: libera 'cleanupSharedFontCache' no elemento de texto do HUD.
// Detalhe: centraliza a logica necessaria para encerrar a etapa e descartar recursos associados.
#pragma endregion

#pragma region HudTextFontCache
void hudText::cleanupSharedFontCache() {
    auto& cache = sharedFontCache();
    for (auto& [key, fontData] : cache) {
        (void)key;
        if (!fontData) continue;
        for (size_t i = 0; i < fontData->characters.size(); i++) {
            if (!fontData->characterLoaded[i]) continue;
            GLuint texture = fontData->characters[i].TextureID;
            if (texture != 0) {
                glDeleteTextures(1, &texture);
            }
        }
    }
    cache.clear();
}

// Funcao: monta 'makeFontCacheKey' no elemento de texto do HUD.
// Detalhe: usa 'fontPath', 'fontSize' para derivar e compor um valor pronto para a proxima etapa do pipeline.
// Retorno: devolve 'std::string' com o texto pronto para exibicao, lookup ou serializacao.
std::string hudText::makeFontCacheKey(const std::string& fontPath, int fontSize) {
    return fontPath + "#" + std::to_string(fontSize);
}

std::unordered_map<std::string, std::shared_ptr<hudText::SharedFontData>>&
// Funcao: executa 'sharedFontCache' no elemento de texto do HUD.
// Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
hudText::sharedFontCache() {
    static std::unordered_map<std::string, std::shared_ptr<SharedFontData>> cache;
    return cache;
}

// Funcao: carrega 'loadFont' no elemento de texto do HUD.
// Detalhe: centraliza a logica necessaria para ler dados externos e adapta-los ao formato interno usado pelo jogo.
void hudText::loadFont() {
    if (fontPath.empty()) {
        std::cerr << "hudText: No font path specified!\n";
        return;
    }

    const std::string cacheKey = makeFontCacheKey(fontPath, fontSize);
    auto& cache = sharedFontCache();
    auto found = cache.find(cacheKey);
    if (found != cache.end()) {
        sharedFontData = found->second;
        return;
    }

    // Read the TTF file
    const std::filesystem::path resolvedFontPath = AppPaths::resolve(fontPath);
    std::ifstream file(resolvedFontPath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "hudText: Failed to open font file: "
                  << resolvedFontPath.string() << "\n";
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> buffer(size);
    if (!file.read((char*)buffer.data(), size)) {
        std::cerr << "hudText: Failed to read font file\n";
        return;
    }

    stbtt_fontinfo font;
    int offset = stbtt_GetFontOffsetForIndex(buffer.data(), 0);
    if (offset == -1) offset = 0; // Default to 0 for single-font TTF files

    if (!stbtt_InitFont(&font, buffer.data(), offset)) {
        std::cerr << "hudText: stbtt_InitFont failed for "
                  << resolvedFontPath.string() << "\n";
        return;
    }

    float scale = stbtt_ScaleForPixelHeight(&font, (float)fontSize);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
    ascent = (int)(ascent * scale);
    descent = (int)(descent * scale);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    auto fontData = std::make_shared<SharedFontData>();

    for (int c = 32; c < 127; c++) {
        int width = 0, height = 0, xoff = 0, yoff = 0;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(&font, scale, scale, c, &width, &height, &xoff, &yoff);

        int advanceWidth = 0, leftSideBearing = 0;
        stbtt_GetCodepointHMetrics(&font, c, &advanceWidth, &leftSideBearing);

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        if (bitmap) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);
            stbtt_FreeBitmap(bitmap, nullptr);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        Character character = {
            texture,
            glm::ivec2(width, height),
            glm::ivec2(xoff, yoff),
            (unsigned int)(advanceWidth * scale)
        };
        fontData->characters[static_cast<size_t>(c)] = character;
        fontData->characterLoaded[static_cast<size_t>(c)] = true;
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    cache[cacheKey] = fontData;
    sharedFontData = std::move(fontData);
}

// Funcao: prepara 'setupBuffers' no elemento de texto do HUD.
// Detalhe: centraliza a logica necessaria para configurar dados auxiliares ou buffers usados nas proximas chamadas.
#pragma endregion

#pragma region HudTextLayoutAndRendering
void hudText::setupBuffers() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Funcao: mede 'measure' no elemento de texto do HUD.
// Detalhe: centraliza a logica necessaria para calcular o tamanho final necessario para layout ou ajuste visual.
// Retorno: devolve 'glm::vec2' com o resultado composto por esta chamada.
glm::vec2 hudText::measure() const {
    return measureText(text);
}

// Funcao: mede 'measureText' no elemento de texto do HUD.
// Detalhe: usa 'value' para calcular o tamanho final necessario para layout ou ajuste visual.
// Retorno: devolve 'glm::vec2' com o resultado composto por esta chamada.
glm::vec2 hudText::measureText(const std::string& value) const {
    const float lineHeight = static_cast<float>(fontSize) * scaleAmt.y * 1.4f;
    if (!sharedFontData) {
        return glm::vec2(0.0f, lineHeight);
    }

    float maxWidth = 0.0f;
    float lineWidth = 0.0f;
    int lines = 1;

    for (char rawChar : value) {
        if (rawChar == '\n') {
            maxWidth = std::max(maxWidth, lineWidth);
            lineWidth = 0.0f;
            lines++;
            continue;
        }

        unsigned char c = static_cast<unsigned char>(rawChar);
        if (c >= sharedFontData->characters.size()) continue;
        if (!sharedFontData->characterLoaded[static_cast<size_t>(c)]) continue;
        const Character ch = sharedFontData->characters[static_cast<size_t>(c)];
        lineWidth += (ch.Advance * scaleAmt.x);
    }
    maxWidth = std::max(maxWidth, lineWidth);

    const float height = lineHeight * static_cast<float>(lines);
    return glm::vec2(maxWidth, height);
}

// Funcao: renderiza 'draw' no elemento de texto do HUD.
// Detalhe: usa 'shader', 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
void hudText::draw(Shader& shader, int screenWidth, int screenHeight) {
    if (!sharedFontData) {
        return;
    }

    shader.setVec4("color", glm::vec4(color, opacity));
    shader.setInt("isText", 1);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    int drawX = posX;
    int drawY = posY;
    if (useLayout) {
        const glm::vec2 size = measure();
        const glm::vec2 resolved = resolveHUDPosition(layout, screenWidth, screenHeight, size);
        drawX = static_cast<int>(std::round(resolved.x));
        drawY = static_cast<int>(std::round(resolved.y));
    }

    float currentX = (float)drawX;
    float currentY = (float)drawY;
    const float lineHeight = (float)fontSize * scaleAmt.y * 1.4f;

    for (char rawChar : text) {
        if (rawChar == '\n') {
            currentX = (float)drawX;
            currentY += lineHeight;
            continue;
        }

        unsigned char c = static_cast<unsigned char>(rawChar);
        if (c >= sharedFontData->characters.size()) continue;
        if (!sharedFontData->characterLoaded[static_cast<size_t>(c)]) continue;

        const Character ch = sharedFontData->characters[static_cast<size_t>(c)];

        float xpos = currentX + ch.Bearing.x * scaleAmt.x;
        float ypos = currentY + ch.Bearing.y * scaleAmt.y + (float)fontSize;

        float w = ch.Size.x * scaleAmt.x;
        float h = ch.Size.y * scaleAmt.y;

        float vertices[6][4] = {
            { xpos,     ypos,       0.0f, 0.0f }, // TL
            { xpos,     ypos + h,   0.0f, 1.0f }, // BL
            { xpos + w, ypos + h,   1.0f, 1.0f }, // BR

            { xpos + w, ypos + h,   1.0f, 1.0f }, // BR
            { xpos + w, ypos,       1.0f, 0.0f }, // TR
            { xpos,     ypos,       0.0f, 0.0f }  // TL
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        currentX += (ch.Advance * scaleAmt.x);
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    shader.setInt("isText", 0);
}
#pragma endregion
