// Arquivo: VoxelParadox.Client/src/Renderer/render/hud/hud_chat_background.hpp
// Papel: declara o fundo dedicado do chat dentro do subsistema de HUD.

#pragma once

#include "hud_element.hpp"

class GameChat;

class hudChatBackground : public HUDElement {
public:
    explicit hudChatBackground(const GameChat* chat);

    void draw(class Shader& shader, int screenWidth, int screenHeight) override;

private:
    const GameChat* chat = nullptr;

    void drawRect(class Shader& shader, const glm::ivec4& rect,
                  const glm::vec4& color) const;
    bool shouldDrawHistoryBackground() const;
};

