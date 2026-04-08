// Arquivo: VoxelParadox.Client/src/Renderer/render/hud/hud_portal_info.cpp
// Papel: implementa "hud portal info" dentro do subsistema "render hud" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma region Includes
#include "hud_portal_info.hpp"

#include "engine/engine.hpp"
#include "engine/input.hpp"
#include "player/player.hpp"
#include "world/world_stack.hpp"
#include "hud.hpp"
#include "hud_text.hpp"

#include <algorithm>
#include <cmath>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iomanip>
#include <sstream>
#pragma endregion

#pragma region HudPortalInfoImplementation
namespace {

constexpr int kScreenPadding = 4;
constexpr int kAnchorOffset = 12;
constexpr int kInputGap = 8;
constexpr int kInputPadding = 8;
constexpr std::size_t kMaxUniverseNameLength = 24;
constexpr double kKeyRepeatDelay = 0.35;
constexpr double kKeyRepeatInterval = 0.045;
constexpr float kCaretWidth = 2.0f;

// Funcao: executa 'trimSpaces' no painel de informacoes do portal.
// Detalhe: usa 'value' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'std::string' com o texto pronto para exibicao, lookup ou serializacao.
std::string trimSpaces(const std::string& value) {
    const std::size_t start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};

    const std::size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

// Funcao: executa 'formatSeed' no painel de informacoes do portal.
// Detalhe: usa 'seed' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'std::string' com o texto pronto para exibicao, lookup ou serializacao.
std::string formatSeed(std::uint32_t seed) {
    std::ostringstream out;
    out << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << seed;
    return out.str();
}

// Funcao: executa 'displayNameOrFallback' no painel de informacoes do portal.
// Detalhe: usa 'universeName' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'std::string' com o texto pronto para exibicao, lookup ou serializacao.
std::string displayNameOrFallback(const std::string& universeName) {
    return universeName.empty() ? "Unnamed" : universeName;
}

// PORTAL INFO LABEL :)
// Funcao: monta 'makePortalInfoLabel' no painel de informacoes do portal.
// Detalhe: usa 'universeName', 'childSeed', 'childBiome' para derivar e compor um valor pronto para a proxima etapa do pipeline.
// Retorno: devolve 'std::string' com o texto pronto para exibicao, lookup ou serializacao.
std::string makePortalInfoLabel(const std::string& universeName,
                                std::uint32_t childSeed,
                                const BiomeSelection& childBiome) {
    return "Name: " + displayNameOrFallback(universeName) + "\nSeed: " +
           formatSeed(childSeed) + "\nBiome: " + childBiome.displayName +
           "\nPress \"Y\" to rename this universe";
}

// Funcao: monta 'makeInputLabel' no painel de informacoes do portal.
// Detalhe: usa 'currentName' para derivar e compor um valor pronto para a proxima etapa do pipeline.
// Retorno: devolve 'std::string' com o texto pronto para exibicao, lookup ou serializacao.
std::string makeInputLabel(const std::string& currentName) {
    return currentName.empty() ? "Type a universe name" : currentName;
}

// Funcao: executa 'ctrlDown' no painel de informacoes do portal.
// Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool ctrlDown() {
    return Input::keyDown(GLFW_KEY_LEFT_CONTROL) || Input::keyDown(GLFW_KEY_RIGHT_CONTROL);
}

// Funcao: renderiza 'drawRect' no painel de informacoes do portal.
// Detalhe: usa 'shader', 'pos', 'size', 'color' para desenhar a saida visual correspondente usando o estado atual.
void drawRect(Shader& shader, const glm::ivec2& pos, const glm::vec2& size,
              const glm::vec4& color) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3((float)pos.x, (float)pos.y, 0.0f));
    model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));

    shader.setMat4("model", model);
    shader.setInt("isText", 0);
    shader.setVec4("color", color);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

} // namespace

// Funcao: executa 'hudPortalInfo' no painel de informacoes do portal.
// Detalhe: usa 'player', 'worldStack', 'fontSize', 'fontPath' para encapsular esta etapa especifica do subsistema.
hudPortalInfo::hudPortalInfo(Player* player, WorldStack* worldStack, int fontSize,
                             const std::string& fontPath)
    : player(player), worldStack(worldStack), fontSize(fontSize), fontPath(fontPath) {
    text = new hudText("", 0, 0, glm::vec2(1.0f), fontSize, fontPath);
    text->setColor(glm::vec3(1.0f));

    inputText = new hudText("", 0, 0, glm::vec2(1.0f), fontSize, fontPath);
    inputText->setColor(glm::vec3(1.0f, 0.95f, 0.6f));
}

// Funcao: libera '~hudPortalInfo' no painel de informacoes do portal.
// Detalhe: centraliza a logica necessaria para encerrar a etapa e descartar recursos associados.
// Retorno: devolve 'hudPortalInfo::' com o resultado composto por esta chamada.
hudPortalInfo::~hudPortalInfo() {
    delete text;
    text = nullptr;

    delete inputText;
    inputText = nullptr;
}

// Funcao: verifica 'isEditing' no painel de informacoes do portal.
// Detalhe: centraliza a logica necessaria para avaliar a condicao consultada com base no estado atual.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool hudPortalInfo::isEditing() const { return editing; }

// Funcao: verifica 'cancelEditing' no painel de informacoes do portal.
// Detalhe: centraliza a logica necessaria para avaliar a condicao consultada com base no estado atual.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool hudPortalInfo::cancelEditing() {
    if (!editing) return false;
    endEditing(false);
    return true;
}

// Funcao: retorna 'getPortalTarget' no painel de informacoes do portal.
// Detalhe: usa 'block', 'childSeed', 'childBiome' para expor um dado derivado ou um acesso controlado ao estado interno.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool hudPortalInfo::getPortalTarget(glm::ivec3& block, std::uint32_t& childSeed,
                                    BiomeSelection& childBiome) const {
    if (!worldStack) return false;

    FractalWorld* world = worldStack->currentWorld();
    if (!world) return false;
    if (world->getBlock(block) != BlockType::PORTAL) return false;

    auto it = world->portalBlocks.find(block);
    if (it == world->portalBlocks.end()) return false;

    childSeed = it->second;
    childBiome = worldStack->getResolvedPortalBiomeSelection(*world, block, childSeed);
    return true;
}

// Funcao: inicia 'beginEditing' no painel de informacoes do portal.
// Detalhe: usa 'block', 'childSeed', 'childBiome' para marcar o inicio de uma transicao ou fluxo controlado.
void hudPortalInfo::beginEditing(const glm::ivec3& block, std::uint32_t childSeed,
                                 const BiomeSelection& childBiome) {
    editing = true;
    editingBlock = block;
    editingSeed = childSeed;
    editingBiome = childBiome;
    editingName = worldStack ? worldStack->getUniverseName(childSeed, childBiome)
                             : std::string();
    caretIndex = editingName.size();
    clearSelection();
    backspaceRepeatAt = 0.0;
    deleteRepeatAt = 0.0;
    leftRepeatAt = 0.0;
    rightRepeatAt = 0.0;
    drawCaret = true;
    drawSelection = false;
    showPlaceholder = editingName.empty();

    Input::enableTextInput(true);
    Input::setCursorVisible(true);
}

// Funcao: finaliza 'endEditing' no painel de informacoes do portal.
// Detalhe: usa 'commit' para concluir a etapa em andamento e consolidar o estado resultante.
void hudPortalInfo::endEditing(bool commit) {
    if (commit && worldStack) {
        worldStack->setUniverseName(editingSeed, editingBiome, trimSpaces(editingName));
    }

    editing = false;
    editingName.clear();
    caretIndex = 0;
    clearSelection();
    backspaceRepeatAt = 0.0;
    deleteRepeatAt = 0.0;
    leftRepeatAt = 0.0;
    rightRepeatAt = 0.0;
    drawCaret = false;
    drawSelection = false;
    showPlaceholder = false;
    Input::enableTextInput(false);
    Input::setCursorVisible(false);
}

// Funcao: verifica 'hasSelection' no painel de informacoes do portal.
// Detalhe: centraliza a logica necessaria para avaliar a condicao consultada com base no estado atual.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool hudPortalInfo::hasSelection() const {
    return selectionStart != selectionEnd;
}

// Funcao: limpa 'clearSelection' no painel de informacoes do portal.
// Detalhe: centraliza a logica necessaria para resetar o estado temporario mantido por este componente.
void hudPortalInfo::clearSelection() {
    selectionStart = caretIndex;
    selectionEnd = caretIndex;
}

// Funcao: executa 'selectAll' no painel de informacoes do portal.
// Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
void hudPortalInfo::selectAll() {
    selectionStart = 0;
    selectionEnd = editingName.size();
    caretIndex = selectionEnd;
}

// Funcao: remove 'deleteSelection' no painel de informacoes do portal.
// Detalhe: centraliza a logica necessaria para descartar o registro ou recurso correspondente.
void hudPortalInfo::deleteSelection() {
    if (!hasSelection()) return;

    const std::size_t first = std::min(selectionStart, selectionEnd);
    const std::size_t last = std::max(selectionStart, selectionEnd);
    editingName.erase(first, last - first);
    caretIndex = first;
    clearSelection();
}

// Funcao: edita 'insertText' no painel de informacoes do portal.
// Detalhe: usa 'typed' para alterar o conteudo controlado por esta rotina preservando o estado restante.
void hudPortalInfo::insertText(const std::string& typed) {
    if (typed.empty()) return;

    if (hasSelection()) {
        deleteSelection();
    }

    const std::size_t available = kMaxUniverseNameLength > editingName.size()
                                      ? (kMaxUniverseNameLength - editingName.size())
                                      : 0;
    if (available == 0) return;

    const std::string clipped = typed.substr(0, available);
    editingName.insert(caretIndex, clipped);
    caretIndex += clipped.size();
    clearSelection();
}

// Funcao: move 'moveCaretLeft' no painel de informacoes do portal.
// Detalhe: centraliza a logica necessaria para reorganizar o cursor ou deslocamento relacionado a esta rotina.
void hudPortalInfo::moveCaretLeft() {
    if (hasSelection()) {
        caretIndex = std::min(selectionStart, selectionEnd);
        clearSelection();
        return;
    }
    if (caretIndex == 0) return;
    caretIndex--;
    clearSelection();
}

// Funcao: move 'moveCaretRight' no painel de informacoes do portal.
// Detalhe: centraliza a logica necessaria para reorganizar o cursor ou deslocamento relacionado a esta rotina.
void hudPortalInfo::moveCaretRight() {
    if (hasSelection()) {
        caretIndex = std::max(selectionStart, selectionEnd);
        clearSelection();
        return;
    }
    if (caretIndex >= editingName.size()) return;
    caretIndex++;
    clearSelection();
}

// Funcao: edita 'eraseBackward' no painel de informacoes do portal.
// Detalhe: centraliza a logica necessaria para alterar o conteudo controlado por esta rotina preservando o estado restante.
void hudPortalInfo::eraseBackward() {
    if (hasSelection()) {
        deleteSelection();
        return;
    }
    if (caretIndex == 0) return;
    editingName.erase(caretIndex - 1, 1);
    caretIndex--;
    clearSelection();
}

// Funcao: edita 'eraseForward' no painel de informacoes do portal.
// Detalhe: centraliza a logica necessaria para alterar o conteudo controlado por esta rotina preservando o estado restante.
void hudPortalInfo::eraseForward() {
    if (hasSelection()) {
        deleteSelection();
        return;
    }
    if (caretIndex >= editingName.size()) return;
    editingName.erase(caretIndex, 1);
    clearSelection();
}

// Funcao: consome 'consumeHeldKey' no painel de informacoes do portal.
// Detalhe: usa 'key', 'now', 'nextRepeatTime' para retirar um pedido ou evento pendente para evitar reprocessamento.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool hudPortalInfo::consumeHeldKey(int key, double now, double& nextRepeatTime) {
    if (Input::keyPressed(key)) {
        nextRepeatTime = now + kKeyRepeatDelay;
        return true;
    }
    if (!Input::keyDown(key)) {
        nextRepeatTime = 0.0;
        return false;
    }
    if (nextRepeatTime > 0.0 && now >= nextRepeatTime) {
        nextRepeatTime = now + kKeyRepeatInterval;
        return true;
    }
    return false;
}

// Funcao: atualiza 'update' no painel de informacoes do portal.
// Detalhe: usa 'screenWidth', 'screenHeight' para sincronizar o estado derivado com o frame atual.
void hudPortalInfo::update(int screenWidth, int screenHeight) {
    visible = false;
    inputPanelSize = glm::vec2(0.0f);
    drawCaret = false;
    drawSelection = false;
    caretPixelOffset = 0.0f;
    selectionPixelStart = 0.0f;
    selectionPixelEnd = 0.0f;

    if (!player || !worldStack || !text || !inputText) return;

    glm::ivec3 block(0);
    std::uint32_t childSeed = 0;
    BiomeSelection childBiome{};

    if (editing) {
        block = editingBlock;
        if (!getPortalTarget(block, childSeed, childBiome)) {
            endEditing(false);
            return;
        }
        editingSeed = childSeed;
        editingBiome = childBiome;
    } else {
        if (!player->hasTarget) return;
        block = player->targetBlock;
        if (!getPortalTarget(block, childSeed, childBiome)) return;

        if (Input::keyPressed(GLFW_KEY_Y)) {
            beginEditing(block, childSeed, childBiome);
        }
    }

    if (editing) {
        const double now = ENGINE::GETTIME();
        if (ctrlDown() && Input::keyPressed(GLFW_KEY_A)) {
            selectAll();
        }

        if (consumeHeldKey(GLFW_KEY_LEFT, now, leftRepeatAt)) {
            moveCaretLeft();
        }

        if (consumeHeldKey(GLFW_KEY_RIGHT, now, rightRepeatAt)) {
            moveCaretRight();
        }

        if (consumeHeldKey(GLFW_KEY_BACKSPACE, now, backspaceRepeatAt)) {
            eraseBackward();
        }

        if (consumeHeldKey(GLFW_KEY_DELETE, now, deleteRepeatAt)) {
            eraseForward();
        }

        insertText(Input::consumeTypedChars());

        if (Input::keyPressed(GLFW_KEY_ENTER) || Input::keyPressed(GLFW_KEY_KP_ENTER)) {
            endEditing(true);
        } else if (Input::keyPressed(GLFW_KEY_ESCAPE)) {
            endEditing(false);
        }
    }

    const std::string storedName = worldStack->getUniverseName(childSeed, childBiome);
    const std::string previewName = editing ? trimSpaces(editingName) : storedName;
    text->setText(makePortalInfoLabel(previewName, childSeed, childBiome));

    const glm::vec3 worldPos = glm::vec3(block) + glm::vec3(0.5f);
    glm::vec2 screenPos(0.0f);
    float ndcDepth = 0.0f;
    if (!player->camera.WorldToScreenPoint(worldPos, screenWidth, screenHeight, screenPos,
                                           &ndcDepth))
        return;
    if (ndcDepth < -1.0f || ndcDepth > 1.0f) return;

    const glm::vec2 infoSize = text->measure();
    glm::vec2 maxSize = infoSize;
    glm::vec2 inputInnerSize(0.0f);

    if (editing) {
        inputText->setText(makeInputLabel(editingName));
        showPlaceholder = editingName.empty();
        inputText->setColor(showPlaceholder ? glm::vec3(0.7f, 0.7f, 0.7f)
                                            : glm::vec3(1.0f, 0.95f, 0.6f));
        inputInnerSize = inputText->measure();
        inputPanelSize = inputInnerSize + glm::vec2((float)kInputPadding * 2.0f);
        maxSize.x = std::max(maxSize.x, inputPanelSize.x);
    }

    int x = (int)std::round(screenPos.x) + kAnchorOffset;
    int y = (int)std::round(screenPos.y) + kAnchorOffset;

    const int maxX = std::max(kScreenPadding, screenWidth - (int)std::ceil(maxSize.x) - kScreenPadding);
    x = std::clamp(x, kScreenPadding, maxX);

    int minY = kScreenPadding;
    if (editing) {
        minY += (int)std::ceil(inputPanelSize.y) + kInputGap;
    }
    const int maxY = std::max(minY, screenHeight - (int)std::ceil(infoSize.y) - kScreenPadding);
    y = std::clamp(y, minY, maxY);

    text->setPosition(x, y);

    if (editing) {
        inputPanelPos = glm::ivec2(x, y - kInputGap - (int)std::ceil(inputPanelSize.y));
        inputText->setPosition(inputPanelPos.x + kInputPadding, inputPanelPos.y + kInputPadding);

        if (!showPlaceholder) {
            const std::string caretPrefix = editingName.substr(0, caretIndex);
            caretPixelOffset = inputText->measureText(caretPrefix).x;
            drawCaret = std::fmod(ENGINE::GETTIME(), 1.0) < 0.5;

            if (hasSelection()) {
                const std::size_t first = std::min(selectionStart, selectionEnd);
                const std::size_t last = std::max(selectionStart, selectionEnd);
                selectionPixelStart = inputText->measureText(editingName.substr(0, first)).x;
                selectionPixelEnd = inputText->measureText(editingName.substr(0, last)).x;
                drawSelection = selectionPixelEnd > selectionPixelStart;
            }
        } else {
            drawCaret = std::fmod(ENGINE::GETTIME(), 1.0) < 0.5;
        }
    }

    visible = true;
}

// Funcao: renderiza 'draw' no painel de informacoes do portal.
// Detalhe: usa 'shader', 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
void hudPortalInfo::draw(Shader& shader, int screenWidth, int screenHeight) {
    if (!visible || !text) return;

    if (editing && inputText) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, HUD::getWhiteTexture());
        HUD::bindQuad();

        drawRect(shader, inputPanelPos - glm::ivec2(1), inputPanelSize + glm::vec2(2.0f),
                 glm::vec4(0.95f, 0.85f, 0.3f, 0.95f));
        drawRect(shader, inputPanelPos, inputPanelSize, glm::vec4(0.05f, 0.05f, 0.05f, 0.92f));
        if (drawSelection) {
            drawRect(shader,
                     glm::ivec2(inputText->getX() + (int)std::round(selectionPixelStart),
                                inputText->getY()),
                     glm::vec2(selectionPixelEnd - selectionPixelStart, inputText->measure().y),
                     glm::vec4(0.3f, 0.45f, 0.9f, 0.65f));
        }

        HUD::unbindQuad();
        glBindTexture(GL_TEXTURE_2D, 0);

        inputText->draw(shader, screenWidth, screenHeight);

        if (drawCaret) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, HUD::getWhiteTexture());
            HUD::bindQuad();
            drawRect(shader,
                     glm::ivec2(inputText->getX() + (int)std::round(caretPixelOffset),
                                inputText->getY()),
                     glm::vec2(kCaretWidth, inputText->measure().y),
                     glm::vec4(1.0f, 0.95f, 0.6f, 0.95f));
            HUD::unbindQuad();
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    text->draw(shader, screenWidth, screenHeight);
}
#pragma endregion
