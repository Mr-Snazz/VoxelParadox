// Arquivo: VoxelParadox.Client/src/Renderer/render/hud/hud.hpp
// Papel: declara "hud" dentro do subsistema "render hud" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma once

#pragma region Includes

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <glm/glm.hpp>
#include "engine/shader.hpp"
#include "hud_element.hpp"
#pragma endregion

#pragma region HudForwardDeclarations
class hudText;
class hudWatchText;
#pragma endregion

#pragma region HudTypes
enum class HUDGroupCategory {
    GAMEPLAY = 0,
    MENU,
};

struct HUDGroup {
    std::string name;
    bool enabled = true;
    int baseLayer = 0;
    HUDGroupCategory category = HUDGroupCategory::GAMEPLAY;
    std::vector<HUDElement*> elements;
};

class HUD {
public:
#pragma region HudLifecycleAndVisibilityApi
    // Funcao: inicializa 'init' no sistema principal de HUD.
    // Detalhe: centraliza a logica necessaria para preparar os recursos e o estado inicial antes do uso.
    // Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
    static bool init();
    // Funcao: atualiza 'update' no sistema principal de HUD.
    // Detalhe: usa 'screenWidth', 'screenHeight' para sincronizar o estado derivado com o frame atual.
    static void update(int screenWidth, int screenHeight);
    // Funcao: renderiza 'render' no sistema principal de HUD.
    // Detalhe: usa 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
    static void render(int screenWidth, int screenHeight);
    // Funcao: libera 'cleanup' no sistema principal de HUD.
    // Detalhe: centraliza a logica necessaria para encerrar a etapa e descartar recursos associados.
    static void cleanup();
    // Funcao: define 'setVisible' no sistema principal de HUD.
    // Detalhe: usa 'visible' para aplicar ao componente o valor ou configuracao recebida.
    static void setVisible(bool visible);
    // Funcao: alterna 'toggleVisible' no sistema principal de HUD.
    // Detalhe: centraliza a logica necessaria para trocar o estado controlado por esta rotina.
    static void toggleVisible();
    // Funcao: verifica 'isVisible' no sistema principal de HUD.
    // Detalhe: centraliza a logica necessaria para avaliar a condicao consultada com base no estado atual.
    // Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
    static bool isVisible();
#pragma endregion

#pragma region HudGroupingApi
    // Funcao: cria 'createGroup' no sistema principal de HUD.
    // Detalhe: usa 'name', 'baseLayer', 'enabled', 'category' para registrar ou anexar o recurso solicitado no fluxo atual.
    static void createGroup(const std::string& name, int baseLayer = 0, bool enabled = true,
                            HUDGroupCategory category = HUDGroupCategory::GAMEPLAY);
    // Funcao: define 'setGroupEnabled' no sistema principal de HUD.
    // Detalhe: usa 'name', 'enabled' para aplicar ao componente o valor ou configuracao recebida.
    static void setGroupEnabled(const std::string& name, bool enabled);
    // Funcao: verifica 'isGroupEnabled' no sistema principal de HUD.
    // Detalhe: usa 'name' para avaliar a condicao consultada com base no estado atual.
    // Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
    static bool isGroupEnabled(const std::string& name);
    // Funcao: define 'setGroupBaseLayer' no sistema principal de HUD.
    // Detalhe: usa 'name', 'baseLayer' para aplicar ao componente o valor ou configuracao recebida.
    static void setGroupBaseLayer(const std::string& name, int baseLayer);
    // Funcao: retorna 'getGroupBaseLayer' no sistema principal de HUD.
    // Detalhe: usa 'name' para expor um dado derivado ou um acesso controlado ao estado interno.
    // Retorno: devolve 'int' com o valor numerico calculado para a proxima decisao do pipeline.
    static int getGroupBaseLayer(const std::string& name);
    // Funcao: define 'setGroupCategory' no sistema principal de HUD.
    // Detalhe: usa 'name', 'category' para aplicar ao componente o valor ou configuracao recebida.
    static void setGroupCategory(const std::string& name, HUDGroupCategory category);
    // Funcao: retorna 'getGroupCategory' no sistema principal de HUD.
    // Detalhe: usa 'name' para expor um dado derivado ou um acesso controlado ao estado interno.
    // Retorno: devolve 'HUDGroupCategory' com o resultado composto por esta chamada.
    static HUDGroupCategory getGroupCategory(const std::string& name);
    // Funcao: executa 'assignToGroup' no sistema principal de HUD.
    // Detalhe: usa 'element', 'groupName' para encapsular esta etapa especifica do subsistema.
    static void assignToGroup(HUDElement* element, const std::string& groupName);
#pragma endregion

#pragma region HudElementRegistryApi
    // Funcao: cria 'add' no sistema principal de HUD.
    // Detalhe: usa 'element' para registrar ou anexar o recurso solicitado no fluxo atual.
    static void add(HUDElement* element);
    // Funcao: cria 'add' no sistema principal de HUD.
    // Detalhe: usa 'element', 'groupName' para registrar ou anexar o recurso solicitado no fluxo atual.
    static void add(HUDElement* element, const std::string& groupName);
    // Funcao: limpa 'clear' no sistema principal de HUD.
    // Detalhe: centraliza a logica necessaria para resetar o estado temporario mantido por este componente.
    static void clear();
#pragma endregion

#pragma region HudFactoryApi
    // Funcao: cria 'addText' no sistema principal de HUD.
    // Detalhe: usa 'text', 'x', 'y', 'scale', 'fontSize', 'fontPath' para registrar ou anexar o recurso solicitado no fluxo atual.
    // Retorno: devolve 'hudText*' para dar acesso direto ao objeto resolvido por esta rotina.
    static hudText* addText(const std::string& text, int x, int y, glm::vec2 scale,
                            int fontSize, const std::string& fontPath = "");
    // Funcao: cria 'addText' no sistema principal de HUD.
    // Detalhe: usa 'text', 'layout', 'scale', 'fontSize', 'fontPath' para registrar ou anexar o recurso solicitado no fluxo atual.
    // Retorno: devolve 'hudText*' para dar acesso direto ao objeto resolvido por esta rotina.
    static hudText* addText(const std::string& text, const HUDLayout& layout,
                            glm::vec2 scale, int fontSize,
                            const std::string& fontPath = "");

    // Funcao: executa 'watchText' no sistema principal de HUD.
    // Detalhe: usa 'binder', 'x', 'y', 'scale', 'fontSize', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
    // Retorno: devolve 'hudWatchText*' para dar acesso direto ao objeto resolvido por esta rotina.
    static hudWatchText* watchText(const std::function<void(std::string&)>& binder,
                                   int x, int y, glm::vec2 scale, int fontSize,
                                   double updateIntervalSeconds = 0.0,
                                   const std::string& fontPath = "");
    // Funcao: executa 'watchText' no sistema principal de HUD.
    // Detalhe: usa 'binder', 'layout', 'scale', 'fontSize', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
    // Retorno: devolve 'hudWatchText*' para dar acesso direto ao objeto resolvido por esta rotina.
    static hudWatchText* watchText(const std::function<void(std::string&)>& binder,
                                   const HUDLayout& layout, glm::vec2 scale, int fontSize,
                                   double updateIntervalSeconds = 0.0,
                                   const std::string& fontPath = "");

    // Funcao: executa 'watchFloat' no sistema principal de HUD.
    // Detalhe: usa 'label', 'getter', 'x', 'y', 'scale', 'fontSize', 'precision', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
    // Retorno: devolve 'hudWatchText*' para dar acesso direto ao objeto resolvido por esta rotina.
    static hudWatchText* watchFloat(const std::string& label,
                                    const std::function<float()>& getter,
                                    int x, int y, glm::vec2 scale, int fontSize,
                                    int precision = 1,
                                    double updateIntervalSeconds = 0.0,
                                    const std::string& fontPath = "");
    // Funcao: executa 'watchFloat' no sistema principal de HUD.
    // Detalhe: usa 'label', 'getter', 'layout', 'scale', 'fontSize', 'precision', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
    // Retorno: devolve 'hudWatchText*' para dar acesso direto ao objeto resolvido por esta rotina.
    static hudWatchText* watchFloat(const std::string& label,
                                    const std::function<float()>& getter,
                                    const HUDLayout& layout, glm::vec2 scale, int fontSize,
                                    int precision = 1,
                                    double updateIntervalSeconds = 0.0,
                                    const std::string& fontPath = "");

    // Funcao: executa 'watchInt' no sistema principal de HUD.
    // Detalhe: usa 'label', 'getter', 'x', 'y', 'scale', 'fontSize', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
    // Retorno: devolve 'hudWatchText*' para dar acesso direto ao objeto resolvido por esta rotina.
    static hudWatchText* watchInt(const std::string& label,
                                  const std::function<int()>& getter,
                                  int x, int y, glm::vec2 scale, int fontSize,
                                  double updateIntervalSeconds = 0.0,
                                  const std::string& fontPath = "");
    // Funcao: executa 'watchInt' no sistema principal de HUD.
    // Detalhe: usa 'label', 'getter', 'layout', 'scale', 'fontSize', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
    // Retorno: devolve 'hudWatchText*' para dar acesso direto ao objeto resolvido por esta rotina.
    static hudWatchText* watchInt(const std::string& label,
                                  const std::function<int()>& getter,
                                  const HUDLayout& layout, glm::vec2 scale, int fontSize,
                                  double updateIntervalSeconds = 0.0,
                                  const std::string& fontPath = "");

    // Funcao: executa 'watchVec3' no sistema principal de HUD.
    // Detalhe: usa 'label', 'getter', 'x', 'y', 'scale', 'fontSize', 'precision', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
    // Retorno: devolve 'hudWatchText*' para dar acesso direto ao objeto resolvido por esta rotina.
    static hudWatchText* watchVec3(const std::string& label,
                                   const std::function<glm::vec3()>& getter,
                                   int x, int y, glm::vec2 scale, int fontSize,
                                   int precision = 1,
                                    double updateIntervalSeconds = 0.0,
                                   const std::string& fontPath = "");
    // Funcao: executa 'watchVec3' no sistema principal de HUD.
    // Detalhe: usa 'label', 'getter', 'layout', 'scale', 'fontSize', 'precision', 'updateIntervalSeconds', 'fontPath' para encapsular esta etapa especifica do subsistema.
    // Retorno: devolve 'hudWatchText*' para dar acesso direto ao objeto resolvido por esta rotina.
    static hudWatchText* watchVec3(const std::string& label,
                                   const std::function<glm::vec3()>& getter,
                                   const HUDLayout& layout, glm::vec2 scale, int fontSize,
                                   int precision = 1,
                                   double updateIntervalSeconds = 0.0,
                                   const std::string& fontPath = "");

    // Funcao: define 'setDefaultFont' no sistema principal de HUD.
    // Detalhe: usa 'fontPath' para aplicar ao componente o valor ou configuracao recebida.
    static void setDefaultFont(const std::string& fontPath);
    // Funcao: retorna 'getDefaultFont' no sistema principal de HUD.
    // Detalhe: centraliza a logica necessaria para expor um dado derivado ou um acesso controlado ao estado interno.
    // Retorno: devolve 'const std::string&' com o texto pronto para exibicao, lookup ou serializacao.
    static const std::string& getDefaultFont();
#pragma endregion

#pragma region HudRenderUtilityApi
    // Funcao: prepara 'setupQuad' no sistema principal de HUD.
    // Detalhe: centraliza a logica necessaria para configurar dados auxiliares ou buffers usados nas proximas chamadas.
    static void setupQuad();
    // Funcao: ajusta 'bindQuad' no sistema principal de HUD.
    // Detalhe: centraliza a logica necessaria para alterar a vinculacao atual de recursos graficos ou estado compartilhado.
    static void bindQuad();
    // Funcao: ajusta 'unbindQuad' no sistema principal de HUD.
    // Detalhe: centraliza a logica necessaria para alterar a vinculacao atual de recursos graficos ou estado compartilhado.
    static void unbindQuad();
    // Funcao: retorna 'getWhiteTexture' no sistema principal de HUD.
    // Detalhe: centraliza a logica necessaria para expor um dado derivado ou um acesso controlado ao estado interno.
    // Retorno: devolve 'unsigned int' com o resultado composto por esta chamada.
    static unsigned int getWhiteTexture();
#pragma endregion

private:
#pragma region HudPrivateState
    static std::vector<std::unique_ptr<HUDElement>> elements;
    static std::unordered_map<std::string, HUDGroup> groups;
    static Shader hudShader;
    static std::string defaultFontPath;
    static unsigned int quadVAO;
    static unsigned int quadVBO;
    static unsigned int whiteTextureID;
    static int lastViewportWidth;
    static int lastViewportHeight;
    static bool viewportTracked;
    static bool globallyVisible;
#pragma endregion

#pragma region HudPrivateHelpers
    // Funcao: executa 'registerGroupMembership' no sistema principal de HUD.
    // Detalhe: usa 'element' para encapsular esta etapa especifica do subsistema.
    static void registerGroupMembership(HUDElement* element);
    // Funcao: remove 'removeGroupMembership' no sistema principal de HUD.
    // Detalhe: usa 'element' para descartar o registro ou recurso correspondente.
    static void removeGroupMembership(HUDElement* element);
    // Funcao: verifica 'isElementEnabled' no sistema principal de HUD.
    // Detalhe: usa 'element' para avaliar a condicao consultada com base no estado atual.
    // Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
    static bool isElementEnabled(const HUDElement* element);
    // Funcao: verifica 'shouldRenderElement' no sistema principal de HUD.
    // Detalhe: usa 'element' para avaliar a condicao consultada com base no estado atual.
    // Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
    static bool shouldRenderElement(const HUDElement* element);
    // Funcao: retorna 'getEffectiveLayer' no sistema principal de HUD.
    // Detalhe: usa 'element' para expor um dado derivado ou um acesso controlado ao estado interno.
    // Retorno: devolve 'int' com o valor numerico calculado para a proxima decisao do pipeline.
    static int getEffectiveLayer(const HUDElement* element);
#pragma endregion
};
#pragma endregion
