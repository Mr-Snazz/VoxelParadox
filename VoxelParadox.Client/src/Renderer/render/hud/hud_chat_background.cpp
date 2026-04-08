// Arquivo: VoxelParadox.Client/src/Renderer/render/hud/hud_chat_background.cpp
// Papel: implementa o fundo dedicado do chat dentro do subsistema de HUD.

#include "hud_chat_background.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "hud.hpp"
#include "runtime/game_chat.hpp"

namespace {

constexpr int kVisibleHistoryLines = 6;
constexpr float kChatBackgroundLeftMargin = 12.0f;
constexpr float kChatBackgroundWidth = 540.0f;
constexpr float kChatHistoryBackgroundBottomMargin = 52.0f;
constexpr float kChatHistoryBackgroundHeight = 160.0f;
constexpr float kChatInputBackgroundBottomMargin = 14.0f;
constexpr float kChatInputBackgroundHeight = 38.0f;

glm::ivec4 makeBottomLeftRect(int screenHeight, float leftMargin, float bottomMargin,
                              float width, float height) {
    return glm::ivec4(static_cast<int>(leftMargin),
                      static_cast<int>(screenHeight - bottomMargin - height),
                      static_cast<int>(width),
                      static_cast<int>(height));
}

} // namespace

hudChatBackground::hudChatBackground(const GameChat* chat) : chat(chat) {}

void hudChatBackground::drawRect(Shader& shader, const glm::ivec4& rect,
                                 const glm::vec4& color) const {
    if (rect.z <= 0 || rect.w <= 0) {
        return;
    }

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(static_cast<float>(rect.x),
                                            static_cast<float>(rect.y), 0.0f));
    model = glm::scale(model, glm::vec3(static_cast<float>(rect.z),
                                        static_cast<float>(rect.w), 1.0f));

    shader.setMat4("model", model);
    shader.setInt("isText", 0);
    shader.setVec4("color", color);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, HUD::getWhiteTexture());
    HUD::bindQuad();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    HUD::unbindQuad();
}

bool hudChatBackground::shouldDrawHistoryBackground() const {
    if (!chat) {
        return false;
    }

    for (int lineIndex = 0; lineIndex < kVisibleHistoryLines; ++lineIndex) {
        if (!chat->historyLineText(lineIndex).empty()) {
            return true;
        }
    }

    return false;
}

void hudChatBackground::draw(Shader& shader, int /*screenWidth*/, int screenHeight) {
    if (!chat) {
        return;
    }

    if (shouldDrawHistoryBackground()) {
        drawRect(shader,
                 makeBottomLeftRect(screenHeight, kChatBackgroundLeftMargin,
                                    kChatHistoryBackgroundBottomMargin,
                                    kChatBackgroundWidth,
                                    kChatHistoryBackgroundHeight),
                 glm::vec4(1.0f, 1.0f, 1.0f, 0.82f));
    }

    if (chat->isOpen()) {
        drawRect(shader,
                 makeBottomLeftRect(screenHeight, kChatBackgroundLeftMargin,
                                    kChatInputBackgroundBottomMargin,
                                    kChatBackgroundWidth,
                                    kChatInputBackgroundHeight),
                 glm::vec4(1.0f, 1.0f, 1.0f, 0.88f));
    }
}
