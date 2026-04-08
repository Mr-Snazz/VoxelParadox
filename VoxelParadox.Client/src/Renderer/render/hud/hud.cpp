// Arquivo: VoxelParadox.Client/src/Renderer/render/hud/hud.cpp
// Papel: implementa "hud" dentro do subsistema "render hud" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma region Includes
#include "hud.hpp"
#include "hud_text.hpp"
#include "hud_watch_text.hpp"
#include "client_assets.hpp"
#include <algorithm>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <iostream>
#pragma endregion

#pragma region HudLocalHelpers
namespace {

// HUD owns the 2D draw list shared by chat, hotbar, pause menu, and settings.
// If a UI element renders on screen, it eventually comes through this file.

} // namespace
#pragma endregion

#pragma region HudGlobalState
std::vector<std::unique_ptr<HUDElement>> HUD::elements;
std::unordered_map<std::string, HUDGroup> HUD::groups;
Shader HUD::hudShader;
std::string HUD::defaultFontPath = "";
unsigned int HUD::quadVAO = 0;
unsigned int HUD::quadVBO = 0;
unsigned int HUD::whiteTextureID = 0;
int HUD::lastViewportWidth = 0;
int HUD::lastViewportHeight = 0;
bool HUD::viewportTracked = false;
bool HUD::globallyVisible = true;

// Funcao: define 'setVisible' no sistema principal de HUD.
// Detalhe: usa 'visible' para aplicar ao componente o valor ou configuracao recebida.
#pragma endregion

#pragma region HudVisibilityAndSetup
void HUD::setVisible(bool visible) {
    globallyVisible = visible;
}

// Funcao: alterna 'toggleVisible' no sistema principal de HUD.
// Detalhe: centraliza a logica necessaria para trocar o estado controlado por esta rotina.
void HUD::toggleVisible() {
    globallyVisible = !globallyVisible;
}

// Funcao: verifica 'isVisible' no sistema principal de HUD.
// Detalhe: centraliza a logica necessaria para avaliar a condicao consultada com base no estado atual.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool HUD::isVisible() {
    return globallyVisible;
}

// Funcao: prepara 'setupQuad' no sistema principal de HUD.
// Detalhe: centraliza a logica necessaria para configurar dados auxiliares ou buffers usados nas proximas chamadas.
void HUD::setupQuad() {
    if (quadVAO != 0) return;
    float quadVertices[] = {
        // pos // tex
        0.0f, 0.0f, 0.0f, 0.0f, // TL
        0.0f, 1.0f, 0.0f, 1.0f, // BL
        1.0f, 1.0f, 1.0f, 1.0f, // BR

        1.0f, 1.0f, 1.0f, 1.0f, // BR
        1.0f, 0.0f, 1.0f, 0.0f, // TR
        0.0f, 0.0f, 0.0f, 0.0f  // TL
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Funcao: inicializa 'init' no sistema principal de HUD.
// Detalhe: centraliza a logica necessaria para preparar os recursos e o estado inicial antes do uso.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool HUD::init() {
    setupQuad();
    if (whiteTextureID == 0) {
        unsigned char white[] = {255, 255, 255, 255};
        glGenTextures(1, &whiteTextureID);
        glBindTexture(GL_TEXTURE_2D, whiteTextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
  return hudShader.compileFromFiles(ClientAssets::kHudVertexShader,
                                    ClientAssets::kHudFragmentShader);
}

// Funcao: cria 'createGroup' no sistema principal de HUD.
// Detalhe: usa 'name', 'baseLayer', 'enabled', 'category' para registrar ou anexar o recurso solicitado no fluxo atual.
#pragma endregion

#pragma region HudGrouping
void HUD::createGroup(const std::string& name, int baseLayer, bool enabled,
                      HUDGroupCategory category) {
    if (name.empty()) return;

    auto [it, inserted] = groups.emplace(name, HUDGroup{});
    if (!inserted) {
        return;
    }

    HUDGroup& group = it->second;
    group.name = name;
    group.baseLayer = baseLayer;
    group.enabled = enabled;
    group.category = category;
}

// Funcao: define 'setGroupEnabled' no sistema principal de HUD.
// Detalhe: usa 'name', 'enabled' para aplicar ao componente o valor ou configuracao recebida.
void HUD::setGroupEnabled(const std::string& name, bool enabled) {
    if (name.empty()) return;
    createGroup(name);
    groups[name].enabled = enabled;
}

// Funcao: verifica 'isGroupEnabled' no sistema principal de HUD.
// Detalhe: usa 'name' para avaliar a condicao consultada com base no estado atual.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool HUD::isGroupEnabled(const std::string& name) {
    if (name.empty()) return true;
    auto it = groups.find(name);
    return it == groups.end() ? true : it->second.enabled;
}

// Funcao: define 'setGroupBaseLayer' no sistema principal de HUD.
// Detalhe: usa 'name', 'baseLayer' para aplicar ao componente o valor ou configuracao recebida.
void HUD::setGroupBaseLayer(const std::string& name, int baseLayer) {
    if (name.empty()) return;
    createGroup(name);
    groups[name].baseLayer = baseLayer;
}

// Funcao: retorna 'getGroupBaseLayer' no sistema principal de HUD.
// Detalhe: usa 'name' para expor um dado derivado ou um acesso controlado ao estado interno.
// Retorno: devolve 'int' com o valor numerico calculado para a proxima decisao do pipeline.
int HUD::getGroupBaseLayer(const std::string& name) {
    if (name.empty()) return 0;
    auto it = groups.find(name);
    return it == groups.end() ? 0 : it->second.baseLayer;
}

// Funcao: define 'setGroupCategory' no sistema principal de HUD.
// Detalhe: usa 'name', 'category' para aplicar ao componente o valor ou configuracao recebida.
void HUD::setGroupCategory(const std::string& name, HUDGroupCategory category) {
    if (name.empty()) return;
    createGroup(name);
    groups[name].category = category;
}

// Funcao: retorna 'getGroupCategory' no sistema principal de HUD.
// Detalhe: usa 'name' para expor um dado derivado ou um acesso controlado ao estado interno.
// Retorno: devolve 'HUDGroupCategory' com o resultado composto por esta chamada.
HUDGroupCategory HUD::getGroupCategory(const std::string& name) {
    if (name.empty()) return HUDGroupCategory::GAMEPLAY;
    auto it = groups.find(name);
    return it == groups.end() ? HUDGroupCategory::GAMEPLAY : it->second.category;
}

// Funcao: remove 'removeGroupMembership' no sistema principal de HUD.
// Detalhe: usa 'element' para descartar o registro ou recurso correspondente.
void HUD::removeGroupMembership(HUDElement* element) {
    if (!element) return;

    for (auto& [name, group] : groups) {
        auto it = std::remove(group.elements.begin(), group.elements.end(), element);
        if (it != group.elements.end()) {
            group.elements.erase(it, group.elements.end());
        }
    }
}

// Funcao: executa 'registerGroupMembership' no sistema principal de HUD.
// Detalhe: usa 'element' para encapsular esta etapa especifica do subsistema.
void HUD::registerGroupMembership(HUDElement* element) {
    if (!element || !element->hasGroup()) return;

    createGroup(element->getGroupName());
    HUDGroup& group = groups[element->getGroupName()];
    const bool alreadyRegistered =
        std::find(group.elements.begin(), group.elements.end(), element) != group.elements.end();
    if (!alreadyRegistered) {
        group.elements.push_back(element);
    }
}

// Funcao: executa 'assignToGroup' no sistema principal de HUD.
// Detalhe: usa 'element', 'groupName' para encapsular esta etapa especifica do subsistema.
void HUD::assignToGroup(HUDElement* element, const std::string& groupName) {
    if (!element) return;

    removeGroupMembership(element);
    element->setGroupName(groupName);
    registerGroupMembership(element);
}

// Funcao: verifica 'isElementEnabled' no sistema principal de HUD.
// Detalhe: usa 'element' para avaliar a condicao consultada com base no estado atual.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool HUD::isElementEnabled(const HUDElement* element) {
    if (!element) return false;
    return !element->hasGroup() || isGroupEnabled(element->getGroupName());
}

// Funcao: verifica 'shouldRenderElement' no sistema principal de HUD.
// Detalhe: usa 'element' para avaliar a condicao consultada com base no estado atual.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool HUD::shouldRenderElement(const HUDElement* element) {
    if (!element) return false;
    if (globallyVisible) return true;
    return element->hasGroup() &&
           getGroupCategory(element->getGroupName()) == HUDGroupCategory::MENU;
}

// Funcao: retorna 'getEffectiveLayer' no sistema principal de HUD.
// Detalhe: usa 'element' para expor um dado derivado ou um acesso controlado ao estado interno.
// Retorno: devolve 'int' com o valor numerico calculado para a proxima decisao do pipeline.
int HUD::getEffectiveLayer(const HUDElement* element) {
    if (!element) return 0;
    if (!element->hasGroup()) return element->getLayer();
    return getGroupBaseLayer(element->getGroupName()) + element->getLayer();
}

// Funcao: atualiza 'update' no sistema principal de HUD.
// Detalhe: usa 'screenWidth', 'screenHeight' para sincronizar o estado derivado com o frame atual.
#pragma endregion

#pragma region HudFrameLoop
void HUD::update(int screenWidth, int screenHeight) {
    if (elements.empty()) return;

    if (!viewportTracked) {
        viewportTracked = true;
        lastViewportWidth = screenWidth;
        lastViewportHeight = screenHeight;
        for (auto& el : elements) {
            el->onViewportChanged(screenWidth, screenHeight, screenWidth, screenHeight);
        }
    } else if (screenWidth != lastViewportWidth || screenHeight != lastViewportHeight) {
        const int oldWidth = lastViewportWidth;
        const int oldHeight = lastViewportHeight;
        lastViewportWidth = screenWidth;
        lastViewportHeight = screenHeight;
        for (auto& el : elements) {
            el->onViewportChanged(screenWidth, screenHeight, oldWidth, oldHeight);
        }
    }

    for (auto& el : elements) {
        if (!isElementEnabled(el.get()) || !shouldRenderElement(el.get())) continue;
        el->update(screenWidth, screenHeight);
    }
}

// Funcao: renderiza 'render' no sistema principal de HUD.
// Detalhe: usa 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
void HUD::render(int screenWidth, int screenHeight) {
    if (elements.empty()) return;

    std::vector<HUDElement*> activeElements;
    activeElements.reserve(elements.size());
    for (auto& el : elements) {
        if (!isElementEnabled(el.get()) || !shouldRenderElement(el.get())) continue;
        activeElements.push_back(el.get());
    }
    if (activeElements.empty()) return;

    std::stable_sort(activeElements.begin(), activeElements.end(),
                     [](const HUDElement* a, const HUDElement* b) {
                         return HUD::getEffectiveLayer(a) < HUD::getEffectiveLayer(b);
                     });

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);

    for (HUDElement* el : activeElements) {
        hudShader.use();
        hudShader.setMat4("projection", projection);
        el->draw(hudShader, screenWidth, screenHeight);
    }

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

// Funcao: libera 'cleanup' no sistema principal de HUD.
// Detalhe: centraliza a logica necessaria para encerrar a etapa e descartar recursos associados.
void HUD::cleanup() {
    elements.clear();
    groups.clear();
    hudText::cleanupSharedFontCache();
    viewportTracked = false;
    globallyVisible = true;
    lastViewportWidth = 0;
    lastViewportHeight = 0;
    if (quadVAO != 0) {
        glDeleteVertexArrays(1, &quadVAO);
        quadVAO = 0;
    }
    if (quadVBO != 0) {
        glDeleteBuffers(1, &quadVBO);
        quadVBO = 0;
    }
    if (whiteTextureID != 0) {
        glDeleteTextures(1, &whiteTextureID);
        whiteTextureID = 0;
    }
    hudShader.release();
}

// Funcao: cria 'add' no sistema principal de HUD.
// Detalhe: usa 'element' para registrar ou anexar o recurso solicitado no fluxo atual.
#pragma endregion

#pragma region HudElementFactories
void HUD::add(HUDElement* element) {
    if (!element) return;

    if (viewportTracked) {
        element->onViewportChanged(lastViewportWidth, lastViewportHeight,
                                   lastViewportWidth, lastViewportHeight);
    }
    registerGroupMembership(element);
    elements.push_back(std::unique_ptr<HUDElement>(element));
}

// Funcao: cria 'add' no sistema principal de HUD.
// Detalhe: usa 'element', 'groupName' para registrar ou anexar o recurso solicitado no fluxo atual.
void HUD::add(HUDElement* element, const std::string& groupName) {
    if (!element) return;
    element->setGroupName(groupName);
    add(element);
}

// Funcao: cria 'addText' no sistema principal de HUD.
// Detalhe: usa 'text', 'x', 'y', 'scale', 'fontSize', 'fontPath' para registrar ou anexar o recurso solicitado no fluxo atual.
// Retorno: devolve 'hudText*' para dar acesso direto ao objeto resolvido por esta rotina.
hudText* HUD::addText(const std::string& text, int x, int y, glm::vec2 scale,
                      int fontSize, const std::string& fontPath) {
    auto* el = new hudText(text, x, y, scale, fontSize, fontPath);
    add(el);
    return el;
}

// Funcao: cria 'addText' no sistema principal de HUD.
// Detalhe: usa 'text', 'layout', 'scale', 'fontSize', 'fontPath' para registrar ou anexar o recurso solicitado no fluxo atual.
// Retorno: devolve 'hudText*' para dar acesso direto ao objeto resolvido por esta rotina.
hudText* HUD::addText(const std::string& text, const HUDLayout& layout, glm::vec2 scale,
                      int fontSize, const std::string& fontPath) {
    auto* el = new hudText(text, layout, scale, fontSize, fontPath);
    add(el);
    return el;
}

// Funcao: executa 'watchText' no sistema principal de HUD.
// Detalhe: usa 'binder', 'x', 'y', 'scale', 'fontSize', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'hudWatchText*' para dar acesso direto ao objeto resolvido por esta rotina.
hudWatchText* HUD::watchText(const std::function<void(std::string&)>& binder,
                             int x, int y, glm::vec2 scale, int fontSize,
                             double updateIntervalSeconds,
                             const std::string& fontPath) {
    auto* el = new hudWatchText(binder, x, y, scale, fontSize, updateIntervalSeconds, fontPath);
    add(el);
    return el;
}

// Funcao: executa 'watchText' no sistema principal de HUD.
// Detalhe: usa 'binder', 'layout', 'scale', 'fontSize', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'hudWatchText*' para dar acesso direto ao objeto resolvido por esta rotina.
hudWatchText* HUD::watchText(const std::function<void(std::string&)>& binder,
                             const HUDLayout& layout, glm::vec2 scale, int fontSize,
                             double updateIntervalSeconds,
                             const std::string& fontPath) {
    auto* el =
        new hudWatchText(binder, layout, scale, fontSize, updateIntervalSeconds, fontPath);
    add(el);
    return el;
}

// Funcao: executa 'watchFloat' no sistema principal de HUD.
// Detalhe: usa 'label', 'getter', 'x', 'y', 'scale', 'fontSize', 'precision', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'hudWatchText*' para dar acesso direto ao objeto resolvido por esta rotina.
hudWatchText* HUD::watchFloat(const std::string& label,
                              const std::function<float()>& getter,
                              int x, int y, glm::vec2 scale, int fontSize,
                              int precision, double updateIntervalSeconds,
                              const std::string& fontPath) {
    return watchText(
        [label, getter, precision](std::string& out) {
            char buf[96];
            const float v = getter ? getter() : 0.0f;
            int p = precision;
            if (p < 0) p = 0;
            if (p > 6) p = 6;
            std::snprintf(buf, sizeof(buf), "%s: %.*f", label.c_str(), p, v);
            out = buf;
        },
        x, y, scale, fontSize, updateIntervalSeconds, fontPath);
}

// Funcao: executa 'watchFloat' no sistema principal de HUD.
// Detalhe: usa 'label', 'getter', 'layout', 'scale', 'fontSize', 'precision', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'hudWatchText*' para dar acesso direto ao objeto resolvido por esta rotina.
hudWatchText* HUD::watchFloat(const std::string& label,
                              const std::function<float()>& getter,
                              const HUDLayout& layout, glm::vec2 scale, int fontSize,
                              int precision, double updateIntervalSeconds,
                              const std::string& fontPath) {
    return watchText(
        [label, getter, precision](std::string& out) {
            char buf[96];
            const float v = getter ? getter() : 0.0f;
            int p = precision;
            if (p < 0) p = 0;
            if (p > 6) p = 6;
            std::snprintf(buf, sizeof(buf), "%s: %.*f", label.c_str(), p, v);
            out = buf;
        },
        layout, scale, fontSize, updateIntervalSeconds, fontPath);
}

// Funcao: executa 'watchInt' no sistema principal de HUD.
// Detalhe: usa 'label', 'getter', 'x', 'y', 'scale', 'fontSize', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'hudWatchText*' para dar acesso direto ao objeto resolvido por esta rotina.
hudWatchText* HUD::watchInt(const std::string& label,
                            const std::function<int()>& getter,
                            int x, int y, glm::vec2 scale, int fontSize,
                            double updateIntervalSeconds,
                            const std::string& fontPath) {
    return watchText(
        [label, getter](std::string& out) {
            char buf[96];
            const int v = getter ? getter() : 0;
            std::snprintf(buf, sizeof(buf), "%s: %d", label.c_str(), v);
            out = buf;
        },
        x, y, scale, fontSize, updateIntervalSeconds, fontPath);
}

// Funcao: executa 'watchInt' no sistema principal de HUD.
// Detalhe: usa 'label', 'getter', 'layout', 'scale', 'fontSize', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'hudWatchText*' para dar acesso direto ao objeto resolvido por esta rotina.
hudWatchText* HUD::watchInt(const std::string& label,
                            const std::function<int()>& getter,
                            const HUDLayout& layout, glm::vec2 scale, int fontSize,
                            double updateIntervalSeconds,
                            const std::string& fontPath) {
    return watchText(
        [label, getter](std::string& out) {
            char buf[96];
            const int v = getter ? getter() : 0;
            std::snprintf(buf, sizeof(buf), "%s: %d", label.c_str(), v);
            out = buf;
        },
        layout, scale, fontSize, updateIntervalSeconds, fontPath);
}

// Funcao: executa 'watchVec3' no sistema principal de HUD.
// Detalhe: usa 'label', 'getter', 'x', 'y', 'scale', 'fontSize', 'precision', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'hudWatchText*' para dar acesso direto ao objeto resolvido por esta rotina.
hudWatchText* HUD::watchVec3(const std::string& label,
                             const std::function<glm::vec3()>& getter,
                             int x, int y, glm::vec2 scale, int fontSize,
                             int precision, double updateIntervalSeconds,
                             const std::string& fontPath) {
    return watchText(
        [label, getter, precision](std::string& out) {
            char buf[128];
            const glm::vec3 v = getter ? getter() : glm::vec3(0.0f);
            int p = precision;
            if (p < 0) p = 0;
            if (p > 6) p = 6;
            std::snprintf(buf, sizeof(buf), "%s: %.*f, %.*f, %.*f", label.c_str(),
                          p, v.x, p, v.y, p, v.z);
            out = buf;
        },
        x, y, scale, fontSize, updateIntervalSeconds, fontPath);
}

// Funcao: executa 'watchVec3' no sistema principal de HUD.
// Detalhe: usa 'label', 'getter', 'layout', 'scale', 'fontSize', 'precision', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'hudWatchText*' para dar acesso direto ao objeto resolvido por esta rotina.
hudWatchText* HUD::watchVec3(const std::string& label,
                             const std::function<glm::vec3()>& getter,
                             const HUDLayout& layout, glm::vec2 scale, int fontSize,
                             int precision, double updateIntervalSeconds,
                             const std::string& fontPath) {
    return watchText(
        [label, getter, precision](std::string& out) {
            char buf[128];
            const glm::vec3 v = getter ? getter() : glm::vec3(0.0f);
            int p = precision;
            if (p < 0) p = 0;
            if (p > 6) p = 6;
            std::snprintf(buf, sizeof(buf), "%s: %.*f, %.*f, %.*f", label.c_str(),
                          p, v.x, p, v.y, p, v.z);
            out = buf;
        },
        layout, scale, fontSize, updateIntervalSeconds, fontPath);
}

// Funcao: limpa 'clear' no sistema principal de HUD.
// Detalhe: centraliza a logica necessaria para resetar o estado temporario mantido por este componente.
#pragma endregion

#pragma region HudUtilities
void HUD::clear() {
    elements.clear();
    groups.clear();
    hudText::cleanupSharedFontCache();
    viewportTracked = false;
    lastViewportWidth = 0;
    lastViewportHeight = 0;
}

// Funcao: define 'setDefaultFont' no sistema principal de HUD.
// Detalhe: usa 'fontPath' para aplicar ao componente o valor ou configuracao recebida.
void HUD::setDefaultFont(const std::string& fontPath) {
    defaultFontPath = fontPath;
}

// Funcao: retorna 'getDefaultFont' no sistema principal de HUD.
// Detalhe: centraliza a logica necessaria para expor um dado derivado ou um acesso controlado ao estado interno.
// Retorno: devolve 'const std::string&' com o texto pronto para exibicao, lookup ou serializacao.
const std::string& HUD::getDefaultFont() {
    return defaultFontPath;
}

// Funcao: ajusta 'bindQuad' no sistema principal de HUD.
// Detalhe: centraliza a logica necessaria para alterar a vinculacao atual de recursos graficos ou estado compartilhado.
void HUD::bindQuad() {
    glBindVertexArray(quadVAO);
}

// Funcao: ajusta 'unbindQuad' no sistema principal de HUD.
// Detalhe: centraliza a logica necessaria para alterar a vinculacao atual de recursos graficos ou estado compartilhado.
void HUD::unbindQuad() {
    glBindVertexArray(0);
}

// Funcao: retorna 'getWhiteTexture' no sistema principal de HUD.
// Detalhe: centraliza a logica necessaria para expor um dado derivado ou um acesso controlado ao estado interno.
// Retorno: devolve 'unsigned int' com o resultado composto por esta chamada.
unsigned int HUD::getWhiteTexture() {
    return whiteTextureID;
}
#pragma endregion
