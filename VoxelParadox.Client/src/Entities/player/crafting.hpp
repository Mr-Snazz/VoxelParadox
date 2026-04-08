// Arquivo: VoxelParadox.Client/src/Entities/player/crafting.hpp
// Papel: declara "crafting" dentro do subsistema "player" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma once

#pragma region Includes

#include <array>
#include <string>
#include <vector>

#include "items/item_catalog.hpp"
#pragma endregion

#pragma region CraftingTypes
struct CraftingRecipeResult {
    InventoryItem item{};
    int count = 0;

    // Funcao: executa 'empty' na resolucao de receitas de crafting.
    // Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
    // Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
    bool empty() const {
        return item.empty() || count <= 0;
    }
};

namespace Crafting {

enum class TriangleSlot : int {
    TOP = 0,
    LEFT = 1,
    RIGHT = 2,
    COUNT = 3,
};

struct InputSlot {
    InventoryItem item{};
    int count = 0;

    // Funcao: executa 'empty' na resolucao de receitas de crafting.
    // Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
    // Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
    bool empty() const {
        return item.empty() || count <= 0;
    }
};

struct TriangleRecipeDefinition {
    std::string id;
    std::string category;
    std::array<InventoryItem, static_cast<size_t>(TriangleSlot::COUNT)> ingredients{};
    InventoryItem result{};
    int resultCount = 0;
};

struct TriangleRecipeMatch {
    const TriangleRecipeDefinition* definition = nullptr;
    int craftCount = 0;

    // Funcao: executa 'valid' na resolucao de receitas de crafting.
    // Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
    // Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
    bool valid() const {
        return definition != nullptr && craftCount > 0;
    }

    // Funcao: executa 'singleResult' na resolucao de receitas de crafting.
    // Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
    // Retorno: devolve 'CraftingRecipeResult' com o resultado composto por esta chamada.
    CraftingRecipeResult singleResult() const {
        if (!valid()) {
            return {};
        }
        return {definition->result, definition->resultCount};
    }

    // Funcao: executa 'bulkResult' na resolucao de receitas de crafting.
    // Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
    // Retorno: devolve 'CraftingRecipeResult' com o resultado composto por esta chamada.
    CraftingRecipeResult bulkResult() const {
        if (!valid()) {
            return {};
        }
        return {definition->result, definition->resultCount * craftCount};
    }
};
#pragma endregion

#pragma region CraftingApi
// Funcao: executa 'triangleSlotName' na resolucao de receitas de crafting.
// Detalhe: usa 'slot' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'const char*' com o texto pronto para exibicao, lookup ou serializacao.
const char* triangleSlotName(TriangleSlot slot);
// Funcao: retorna 'getTriangleRecipes' na resolucao de receitas de crafting.
// Detalhe: centraliza a logica necessaria para expor um dado derivado ou um acesso controlado ao estado interno.
// Retorno: devolve 'const std::vector<TriangleRecipeDefinition>&' com a colecao ou o resultado agregado montado por esta etapa.
const std::vector<TriangleRecipeDefinition>& getTriangleRecipes();
// Funcao: resolve 'resolveTriangleRecipe' na resolucao de receitas de crafting.
// Detalhe: usa 'inputs' para traduzir o estado atual para uma resposta concreta usada pelo restante do sistema.
// Retorno: devolve 'TriangleRecipeMatch' com o resultado composto por esta chamada.
TriangleRecipeMatch resolveTriangleRecipe(
    const std::array<InputSlot, static_cast<size_t>(TriangleSlot::COUNT)>& inputs);

} // namespace Crafting
#pragma endregion
