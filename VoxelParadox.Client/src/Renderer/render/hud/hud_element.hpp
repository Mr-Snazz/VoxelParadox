// Arquivo: VoxelParadox.Client/src/Renderer/render/hud/hud_element.hpp
// Papel: declara "hud element" dentro do subsistema "render hud" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma once

#pragma region Includes
#include <string>
#include <glm/glm.hpp>
#pragma endregion

#pragma region HudElementApi
enum class ImageType {
    STRETCH, // Stretch width and height independently based on scale.x and scale.y
    FILL     // Keep aspect ratio, scale by taking maximum of scale.x and scale.y
};

enum class HUDAnchor {
    TOP_LEFT,
    TOP_CENTER,
    TOP_RIGHT,
    CENTER_LEFT,
    CENTER,
    CENTER_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_CENTER,
    BOTTOM_RIGHT,
};

struct HUDLayout {
    HUDAnchor anchor = HUDAnchor::TOP_LEFT;
    HUDAnchor pivot = HUDAnchor::TOP_LEFT;
    glm::vec2 offset{0.0f};
};

// Funcao: executa 'hudAnchorFactor' neste modulo do projeto VoxelParadox.Client.
// Detalhe: usa 'anchor' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'glm::vec2' com o resultado composto por esta chamada.
inline glm::vec2 hudAnchorFactor(HUDAnchor anchor) {
    switch (anchor) {
    case HUDAnchor::TOP_LEFT: return glm::vec2(0.0f, 0.0f);
    case HUDAnchor::TOP_CENTER: return glm::vec2(0.5f, 0.0f);
    case HUDAnchor::TOP_RIGHT: return glm::vec2(1.0f, 0.0f);
    case HUDAnchor::CENTER_LEFT: return glm::vec2(0.0f, 0.5f);
    case HUDAnchor::CENTER: return glm::vec2(0.5f, 0.5f);
    case HUDAnchor::CENTER_RIGHT: return glm::vec2(1.0f, 0.5f);
    case HUDAnchor::BOTTOM_LEFT: return glm::vec2(0.0f, 1.0f);
    case HUDAnchor::BOTTOM_CENTER: return glm::vec2(0.5f, 1.0f);
    case HUDAnchor::BOTTOM_RIGHT: return glm::vec2(1.0f, 1.0f);
    }
    return glm::vec2(0.0f);
}

// Funcao: resolve 'resolveHUDPosition' neste modulo do projeto VoxelParadox.Client.
// Detalhe: usa 'layout', 'screenWidth', 'screenHeight', 'size' para traduzir o estado atual para uma resposta concreta usada pelo restante do sistema.
// Retorno: devolve 'glm::vec2' com o resultado composto por esta chamada.
inline glm::vec2 resolveHUDPosition(const HUDLayout& layout, int screenWidth,
                                    int screenHeight, const glm::vec2& size) {
    const glm::vec2 anchorPoint =
        glm::vec2(screenWidth, screenHeight) * hudAnchorFactor(layout.anchor);
    const glm::vec2 pivotOffset = size * hudAnchorFactor(layout.pivot);
    return anchorPoint + layout.offset - pivotOffset;
}

// Funcao: monta 'makeHUDLayout' neste modulo do projeto VoxelParadox.Client.
// Detalhe: usa 'anchor', 'offset' para derivar e compor um valor pronto para a proxima etapa do pipeline.
// Retorno: devolve 'HUDLayout' com o resultado composto por esta chamada.
inline HUDLayout makeHUDLayout(HUDAnchor anchor, const glm::vec2& offset = glm::vec2(0.0f)) {
    HUDLayout layout;
    layout.anchor = anchor;
    layout.pivot = anchor;
    layout.offset = offset;
    return layout;
}

// Funcao: monta 'makeHUDLayout' neste modulo do projeto VoxelParadox.Client.
// Detalhe: usa 'anchor', 'pivot', 'offset' para derivar e compor um valor pronto para a proxima etapa do pipeline.
// Retorno: devolve 'HUDLayout' com o resultado composto por esta chamada.
inline HUDLayout makeHUDLayout(HUDAnchor anchor, HUDAnchor pivot, const glm::vec2& offset) {
    HUDLayout layout;
    layout.anchor = anchor;
    layout.pivot = pivot;
    layout.offset = offset;
    return layout;
}

class HUDElement {
public:
    // Funcao: libera '~HUDElement' neste modulo do projeto VoxelParadox.Client.
    // Detalhe: centraliza a logica necessaria para encerrar a etapa e descartar recursos associados.
    virtual ~HUDElement() = default;

    // Funcao: define 'setLayer' neste modulo do projeto VoxelParadox.Client.
    // Detalhe: usa 'newLayer' para aplicar ao componente o valor ou configuracao recebida.
    void setLayer(int newLayer) { layer = newLayer; }
    // Funcao: retorna 'getLayer' neste modulo do projeto VoxelParadox.Client.
    // Detalhe: centraliza a logica necessaria para expor um dado derivado ou um acesso controlado ao estado interno.
    // Retorno: devolve 'int' com o valor numerico calculado para a proxima decisao do pipeline.
    int getLayer() const { return layer; }

    // Funcao: define 'setGroupName' neste modulo do projeto VoxelParadox.Client.
    // Detalhe: usa 'newGroupName' para aplicar ao componente o valor ou configuracao recebida.
    void setGroupName(const std::string& newGroupName) { groupName = newGroupName; }
    // Funcao: retorna 'getGroupName' neste modulo do projeto VoxelParadox.Client.
    // Detalhe: centraliza a logica necessaria para expor um dado derivado ou um acesso controlado ao estado interno.
    // Retorno: devolve 'const std::string&' com o texto pronto para exibicao, lookup ou serializacao.
    const std::string& getGroupName() const { return groupName; }
    // Funcao: verifica 'hasGroup' neste modulo do projeto VoxelParadox.Client.
    // Detalhe: centraliza a logica necessaria para avaliar a condicao consultada com base no estado atual.
    // Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
    bool hasGroup() const { return !groupName.empty(); }

    // Abstract method to render the UI element. Left as pure virtual to force implementation.
    // Funcao: renderiza 'draw' neste modulo do projeto VoxelParadox.Client.
    // Detalhe: usa 'shader', 'screenWidth', 'screenHeight' para desenhar a saida visual correspondente usando o estado atual.
    virtual void draw(class Shader& shader, int screenWidth, int screenHeight) = 0;

    // Optional per-frame update hook (input, animation, layout). Default is no-op.
    // Funcao: atualiza 'update' neste modulo do projeto VoxelParadox.Client.
    // Detalhe: usa 'int', 'int' para sincronizar o estado derivado com o frame atual.
    virtual void update(int /*screenWidth*/, int /*screenHeight*/) {}

    // Optional hook fired whenever the viewport dimensions change.
    // Funcao: executa 'onViewportChanged' neste modulo do projeto VoxelParadox.Client.
    // Detalhe: usa 'int', 'int', 'int', 'int' para encapsular esta etapa especifica do subsistema.
    virtual void onViewportChanged(int /*newWidth*/, int /*newHeight*/,
                                   int /*oldWidth*/, int /*oldHeight*/) {}

private:
    int layer = 0;
    std::string groupName;
};
#pragma endregion
