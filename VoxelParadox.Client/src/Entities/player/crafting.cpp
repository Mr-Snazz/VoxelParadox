// Arquivo: VoxelParadox.Client/src/Entities/player/crafting.cpp
// Papel: implementa "crafting" dentro do subsistema "player" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma region Includes
#include "crafting.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>

#include <nlohmann/json.hpp>

#include "client_assets.hpp"
#include "path/app_paths.hpp"
#pragma endregion

#pragma region CraftingLocalHelpers
namespace {

using json = nlohmann::json;
using Crafting::TriangleRecipeDefinition;
using Crafting::TriangleSlot;

// Funcao: executa 'readTextFile' na resolucao de receitas de crafting.
// Detalhe: usa 'path' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'std::string' com o texto pronto para exibicao, lookup ou serializacao.
std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return {};
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Funcao: executa 'categoryFromPath' na resolucao de receitas de crafting.
// Detalhe: usa 'path' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'std::string' com o texto pronto para exibicao, lookup ou serializacao.
std::string categoryFromPath(const std::filesystem::path& path) {
    std::string stem = normalizeItemId(path.stem().string());
    constexpr const char* prefix = "recipe_";
    if (stem.rfind(prefix, 0) == 0) {
        stem.erase(0, std::char_traits<char>::length(prefix));
    }
    return stem;
}

// Funcao: interpreta 'parseRecipeDefinition' na resolucao de receitas de crafting.
// Detalhe: usa 'object', 'category', 'outRecipe' para converter a entrada textual para a representacao interna correspondente.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool parseRecipeDefinition(const json& object, const std::string& category,
                           TriangleRecipeDefinition& outRecipe) {
    if (!object.is_object()) {
        return false;
    }

    const std::string recipeId = object.value("recipe_id", std::string());
    const std::string outputId = object.value("output", std::string());
    const int outputCount = object.value("output_count", 1);
    const json ingredientsObject = object.value("ingredients", json::object());

    if (recipeId.empty() || outputId.empty() || !ingredientsObject.is_object()) {
        return false;
    }

    if (outputCount <= 0) {
        return false;
    }

    std::array<InventoryItem, static_cast<size_t>(TriangleSlot::COUNT)> ingredients{};
    auto parseIngredient = [&](TriangleSlot slot) {
        InventoryItem parsedItem{};
        const auto found = ingredientsObject.find(Crafting::triangleSlotName(slot));
        if (found == ingredientsObject.end() || !found->is_string()) {
            ingredients[static_cast<size_t>(slot)] = {};
            return true;
        }
        const std::string ingredientId = found->get<std::string>();
        if (!tryParseInventoryItem(ingredientId, parsedItem)) {
            return false;
        }
        ingredients[static_cast<size_t>(slot)] = parsedItem;
        return true;
    };

    if (!parseIngredient(TriangleSlot::TOP) ||
        !parseIngredient(TriangleSlot::LEFT) ||
        !parseIngredient(TriangleSlot::RIGHT)) {
        return false;
    }

    const bool hasAnyIngredient =
        std::any_of(ingredients.begin(), ingredients.end(),
                    [](const InventoryItem& item) { return !item.empty(); });
    if (!hasAnyIngredient) {
        return false;
    }

    InventoryItem outputItem{};
    if (!tryParseInventoryItem(outputId, outputItem) || outputItem.empty()) {
        return false;
    }

    outRecipe.id = normalizeItemId(recipeId);
    outRecipe.category = category;
    outRecipe.ingredients = ingredients;
    outRecipe.result = outputItem;
    outRecipe.resultCount = outputCount;
    return true;
}

// Funcao: carrega 'loadTriangleRecipesFromJson' na resolucao de receitas de crafting.
// Detalhe: centraliza a logica necessaria para ler dados externos e adapta-los ao formato interno usado pelo jogo.
// Retorno: devolve 'std::vector<TriangleRecipeDefinition>' com a colecao ou o resultado agregado montado por esta etapa.
std::vector<TriangleRecipeDefinition> loadTriangleRecipesFromJson() {
    std::vector<TriangleRecipeDefinition> recipes;
    const std::filesystem::path recipesDirectory =
        AppPaths::resolve(ClientAssets::kRecipesDirectory);
    std::error_code errorCode;
    if (!std::filesystem::exists(recipesDirectory, errorCode) ||
        !std::filesystem::is_directory(recipesDirectory, errorCode)) {
        return recipes;
    }

    std::vector<std::filesystem::path> jsonFiles;
    for (const auto& entry : std::filesystem::directory_iterator(recipesDirectory, errorCode)) {
        if (errorCode || !entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() == ".json") {
            jsonFiles.push_back(entry.path());
        }
    }

    std::sort(jsonFiles.begin(), jsonFiles.end());

    for (const std::filesystem::path& path : jsonFiles) {
        const std::string content = readTextFile(path);
        if (content.empty()) {
            continue;
        }

        json root = json::parse(content, nullptr, false);
        if (root.is_discarded() || !root.is_array()) {
            continue;
        }

        const std::string category = categoryFromPath(path);
        for (const json& object : root) {
            TriangleRecipeDefinition recipe;
            if (parseRecipeDefinition(object, category, recipe)) {
                recipes.push_back(std::move(recipe));
            }
        }
    }

    return recipes;
}

} // namespace

#pragma region CraftingRecipes
namespace Crafting {

// Funcao: executa 'triangleSlotName' na resolucao de receitas de crafting.
// Detalhe: usa 'slot' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'const char*' com o texto pronto para exibicao, lookup ou serializacao.
const char* triangleSlotName(TriangleSlot slot) {
    switch (slot) {
    case TriangleSlot::TOP:
        return "top";
    case TriangleSlot::LEFT:
        return "left";
    case TriangleSlot::RIGHT:
        return "right";
    case TriangleSlot::COUNT:
    default:
        return "unknown";
    }
}

// Funcao: retorna 'getTriangleRecipes' na resolucao de receitas de crafting.
// Detalhe: centraliza a logica necessaria para expor um dado derivado ou um acesso controlado ao estado interno.
// Retorno: devolve 'const std::vector<TriangleRecipeDefinition>&' com a colecao ou o resultado agregado montado por esta etapa.
const std::vector<TriangleRecipeDefinition>& getTriangleRecipes() {
    static const std::vector<TriangleRecipeDefinition> recipes =
        loadTriangleRecipesFromJson();
    return recipes;
}

// Funcao: resolve 'resolveTriangleRecipe' na resolucao de receitas de crafting.
// Detalhe: usa 'inputs' para traduzir o estado atual para uma resposta concreta usada pelo restante do sistema.
// Retorno: devolve 'TriangleRecipeMatch' com o resultado composto por esta chamada.
TriangleRecipeMatch resolveTriangleRecipe(
    const std::array<InputSlot, static_cast<size_t>(TriangleSlot::COUNT)>& inputs) {
    const std::vector<TriangleRecipeDefinition>& recipes = getTriangleRecipes();
    for (const TriangleRecipeDefinition& recipe : recipes) {
        bool matches = true;
        int craftCount = std::numeric_limits<int>::max();
        bool usedAtLeastOneIngredient = false;

        for (std::size_t index = 0; index < inputs.size(); index++) {
            const InventoryItem& expectedItem = recipe.ingredients[index];
            const InputSlot& input = inputs[index];

            if (expectedItem.empty()) {
                if (!input.empty()) {
                    matches = false;
                    break;
                }
                continue;
            }

            if (input.item != expectedItem || input.count <= 0) {
                matches = false;
                break;
            }

            usedAtLeastOneIngredient = true;
            craftCount = std::min(craftCount, input.count);
        }

        if (matches && usedAtLeastOneIngredient && craftCount > 0) {
            return {&recipe, craftCount};
        }
    }

    return {};
}

} // namespace Crafting
#pragma endregion
