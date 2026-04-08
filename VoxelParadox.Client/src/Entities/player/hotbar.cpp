// Arquivo: VoxelParadox.Client/src/Entities/player/hotbar.cpp
// Papel: implementa "hotbar" dentro do subsistema "player" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma region Includes
#include "hotbar.hpp"
#pragma endregion

#pragma region HotbarLifecycle
void PlayerHotbar::clear() {
    storageSlots.fill(Slot{});
    craftSlots.fill(Slot{});
    heldSlot = Slot{};
    selectedIndex = 0;
    inventoryOpen = false;
}

// Funcao: tenta 'tryCloseInventory' na hotbar e no inventario do jogador.
// Detalhe: centraliza a logica necessaria para executar a operacao sem assumir que as pre-condicoes estao garantidas.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool PlayerHotbar::tryCloseInventory() {
    if (!heldSlot.empty() && tryStoreHeldSlotIntoStorage()) {
        heldSlot = Slot{};
    }
    if (!heldSlot.empty()) {
        return false;
    }
    inventoryOpen = false;
    return true;
}

// Funcao: retorna 'getCraftingResult' na hotbar e no inventario do jogador.
// Detalhe: centraliza a logica necessaria para expor um dado derivado ou um acesso controlado ao estado interno.
// Retorno: devolve 'CraftingRecipeResult' com o resultado composto por esta chamada.
#pragma endregion

#pragma region HotbarCraftingAndCapacity
CraftingRecipeResult PlayerHotbar::getCraftingResult() const {
    return getResolvedCraftingRecipe().singleResult();
}

// Funcao: retorna 'getCraftingBulkResult' na hotbar e no inventario do jogador.
// Detalhe: centraliza a logica necessaria para expor um dado derivado ou um acesso controlado ao estado interno.
// Retorno: devolve 'CraftingRecipeResult' com o resultado composto por esta chamada.
CraftingRecipeResult PlayerHotbar::getCraftingBulkResult() const {
    const Crafting::TriangleRecipeMatch recipe = getResolvedCraftingRecipe();
    if (!recipe.valid()) {
        return {};
    }
    return recipe.bulkResult();
}

// Funcao: verifica 'canAccept' na hotbar e no inventario do jogador.
// Detalhe: usa 'type', 'amount' para avaliar a condicao consultada com base no estado atual.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool PlayerHotbar::canAccept(const InventoryItem& item, int amount) const {
    if (!canStore(item) || amount <= 0) {
        return false;
    }
    return availableCapacityFor(item) >= amount;
}

// Funcao: cria 'addItem' na hotbar e no inventario do jogador.
// Detalhe: usa 'type', 'amount' para registrar ou anexar o recurso solicitado no fluxo atual.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool PlayerHotbar::addItem(const InventoryItem& item, int amount) {
    if (!canAccept(item, amount)) {
        return false;
    }

    int remaining = amount;
    const int stackLimit = getInventoryItemStackLimit(item);

    for (Slot& slot : storageSlots) {
        if (remaining <= 0) {
            break;
        }
        if (slot.empty() || slot.item != item || slot.count >= stackLimit) {
            continue;
        }

        const int addAmount = std::min(stackLimit - slot.count, remaining);
        slot.count += addAmount;
        remaining -= addAmount;
    }

    for (Slot& slot : storageSlots) {
        if (remaining <= 0) {
            break;
        }
        if (!slot.empty()) {
            continue;
        }

        slot.item = item;
        const int addAmount = std::min(stackLimit, remaining);
        slot.count = addAmount;
        remaining -= addAmount;
    }

    return remaining == 0;
}

// Funcao: consome 'consumeSelected' na hotbar e no inventario do jogador.
// Detalhe: usa 'amount' para retirar um pedido ou evento pendente para evitar reprocessamento.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool PlayerHotbar::consumeSelected(int amount) {
    if (amount <= 0) {
        return true;
    }

    Slot& slot = storageSlots[static_cast<size_t>(selectedIndex)];
    if (slot.empty() || slot.count < amount) {
        return false;
    }

    slot.count -= amount;
    sanitize(slot);
    return true;
}

// Funcao: processa 'clickStorageSlot' na hotbar e no inventario do jogador.
// Detalhe: usa 'index', 'clickType' para interpretar a interacao recebida e executar as mudancas necessarias.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
#pragma endregion

#pragma region HotbarInputHandling
bool PlayerHotbar::clickStorageSlot(int index, ClickType clickType) {
    if (index < 0 || index >= TOTAL_STORAGE_SLOTS) {
        return false;
    }

    Slot& slot = storageSlots[static_cast<size_t>(index)];
    return clickType == ClickType::LEFT
               ? handleLeftClick(slot, getSlotInteractionCapacity(slot))
               : handleRightClick(slot, getSlotInteractionCapacity(slot));
}

// Funcao: processa 'clickCraftSlot' na hotbar e no inventario do jogador.
// Detalhe: usa 'index', 'clickType' para interpretar a interacao recebida e executar as mudancas necessarias.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool PlayerHotbar::clickCraftSlot(int index, ClickType clickType) {
    if (index < 0 || index >= CRAFT_SLOT_COUNT) {
        return false;
    }

    Slot& slot = craftSlots[static_cast<size_t>(index)];
    return clickType == ClickType::LEFT
               ? handleLeftClick(slot, getSlotInteractionCapacity(slot))
               : handleRightClick(slot, getSlotInteractionCapacity(slot));
}

// Funcao: processa 'clickCraftResult' na hotbar e no inventario do jogador.
// Detalhe: usa 'ClickType', 'craftAll' para interpretar a interacao recebida e executar as mudancas necessarias.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool PlayerHotbar::clickCraftResult(ClickType /*clickType*/, bool craftAll) {
    const Crafting::TriangleRecipeMatch recipe = getResolvedCraftingRecipe();
    if (!recipe.valid()) {
        return false;
    }

    const CraftingRecipeResult previewResult =
        craftAll ? recipe.bulkResult() : recipe.singleResult();
    if (previewResult.empty()) {
        return false;
    }

    const int craftIterations = craftAll ? recipe.craftCount : 1;
    if (craftIterations <= 0) {
        return false;
    }

    const CraftingRecipeResult result =
        craftIterations == 1 ? recipe.singleResult() : recipe.bulkResult();
    if (result.empty()) {
        return false;
    }

    if (craftAll) {
        if (!addItem(result.item, result.count)) {
            return false;
        }
        consumeCraftIngredients(craftIterations);
        return true;
    }

    if (heldSlot.empty()) {
        heldSlot.item = result.item;
        heldSlot.count = result.count;
        consumeCraftIngredients(craftIterations);
        return true;
    }

    if (heldSlot.item != result.item ||
        heldSlot.count + result.count > getInventoryItemStackLimit(result.item)) {
        return false;
    }

    heldSlot.count += result.count;
    consumeCraftIngredients(craftIterations);
    return true;
}

// Funcao: tenta 'tryStoreHeldSlotIntoStorage' na hotbar e no inventario do jogador.
// Detalhe: centraliza a logica necessaria para executar a operacao sem assumir que as pre-condicoes estao garantidas.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool PlayerHotbar::tryStoreHeldSlotIntoStorage() {
    sanitize(heldSlot);
    if (heldSlot.empty()) {
        return true;
    }

    return addItem(heldSlot.item, heldSlot.count);
}

// Funcao: retorna 'getResolvedCraftingRecipe' na hotbar e no inventario do jogador.
// Detalhe: centraliza a logica necessaria para expor um dado derivado ou um acesso controlado ao estado interno.
// Retorno: devolve 'Crafting::TriangleRecipeMatch' com o resultado composto por esta chamada.
Crafting::TriangleRecipeMatch PlayerHotbar::getResolvedCraftingRecipe() const {
    std::array<Crafting::InputSlot, CRAFT_SLOT_COUNT> inputs{};
    for (int index = 0; index < CRAFT_SLOT_COUNT; index++) {
        const Slot& slot = craftSlots[static_cast<size_t>(index)];
        inputs[static_cast<size_t>(index)].item = slot.empty() ? InventoryItem{} : slot.item;
        inputs[static_cast<size_t>(index)].count = slot.empty() ? 0 : slot.count;
    }
    return Crafting::resolveTriangleRecipe(inputs);
}

// Funcao: executa 'availableCapacityFor' na hotbar e no inventario do jogador.
// Detalhe: usa 'type' para encapsular esta etapa especifica do subsistema.
// Retorno: devolve 'int' com o valor numerico calculado para a proxima decisao do pipeline.
int PlayerHotbar::availableCapacityFor(const InventoryItem& item) const {
    int capacity = 0;
    const int stackLimit = getInventoryItemStackLimit(item);
    for (const Slot& slot : storageSlots) {
        if (slot.empty()) {
            capacity += stackLimit;
        } else if (slot.item == item) {
            capacity += stackLimit - slot.count;
        }
    }
    return capacity;
}

// Funcao: processa 'handleLeftClick' na hotbar e no inventario do jogador.
// Detalhe: usa 'slot', 'capacity' para interpretar a interacao recebida e executar as mudancas necessarias.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
#pragma endregion

#pragma region HotbarInternalHelpers
bool PlayerHotbar::handleLeftClick(Slot& slot, int capacity) {
    sanitize(slot);
    sanitize(heldSlot);

    if (slot.empty()) {
        capacity = getSlotInteractionCapacity(slot, &heldSlot);
    } else {
        capacity = getSlotInteractionCapacity(slot);
    }

    if (heldSlot.empty()) {
        if (slot.empty()) {
            return false;
        }

        heldSlot = slot;
        slot = Slot{};
        return true;
    }

    if (slot.empty()) {
        const int placeAmount = std::min(capacity, heldSlot.count);
        if (placeAmount <= 0) {
            return false;
        }

        slot.item = heldSlot.item;
        slot.count = placeAmount;
        heldSlot.count -= placeAmount;
        sanitize(heldSlot);
        return true;
    }

    if (slot.item == heldSlot.item) {
        if (slot.count >= capacity) {
            return false;
        }

        const int moveAmount = std::min(capacity - slot.count, heldSlot.count);
        if (moveAmount <= 0) {
            return false;
        }

        slot.count += moveAmount;
        heldSlot.count -= moveAmount;
        sanitize(heldSlot);
        return true;
    }

    if (!canSwapIntoLimitedSlot(heldSlot, capacity)) {
        return false;
    }

    std::swap(slot, heldSlot);
    sanitize(slot);
    sanitize(heldSlot);
    return true;
}

// Funcao: processa 'handleRightClick' na hotbar e no inventario do jogador.
// Detalhe: usa 'slot', 'capacity' para interpretar a interacao recebida e executar as mudancas necessarias.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool PlayerHotbar::handleRightClick(Slot& slot, int capacity) {
    sanitize(slot);
    sanitize(heldSlot);

    if (slot.empty()) {
        capacity = getSlotInteractionCapacity(slot, &heldSlot);
    } else {
        capacity = getSlotInteractionCapacity(slot);
    }

    if (heldSlot.empty()) {
        if (slot.empty()) {
            return false;
        }

        const int takeAmount = (slot.count + 1) / 2;
        heldSlot.item = slot.item;
        heldSlot.count = takeAmount;
        slot.count -= takeAmount;
        sanitize(slot);
        return true;
    }

    if (slot.empty()) {
        slot.item = heldSlot.item;
        slot.count = 1;
        heldSlot.count -= 1;
        sanitize(heldSlot);
        return true;
    }

    if (slot.item != heldSlot.item || slot.count >= capacity) {
        return false;
    }

    slot.count += 1;
    heldSlot.count -= 1;
    sanitize(heldSlot);
    return true;
}

// Funcao: consome 'consumeCraftIngredients' na hotbar e no inventario do jogador.
// Detalhe: usa 'amountPerSlot' para retirar um pedido ou evento pendente para evitar reprocessamento.
void PlayerHotbar::consumeCraftIngredients(int amountPerSlot) {
    if (amountPerSlot <= 0) {
        return;
    }

    for (Slot& slot : craftSlots) {
        if (!slot.empty()) {
            slot.count -= amountPerSlot;
            sanitize(slot);
        }
    }
}

PlayerHotbar::PersistentState PlayerHotbar::capturePersistentState() const {
    PersistentState state;
    state.storageSlots = storageSlots;
    state.craftSlots = craftSlots;
    state.heldSlot = heldSlot;
    state.selectedIndex = selectedIndex;
    state.inventoryOpen = inventoryOpen;
    return state;
}

void PlayerHotbar::applyPersistentState(const PersistentState& state) {
    storageSlots = state.storageSlots;
    craftSlots = state.craftSlots;
    heldSlot = state.heldSlot;
    selectedIndex = std::clamp(state.selectedIndex, 0, HOTBAR_SLOT_COUNT - 1);
    inventoryOpen = state.inventoryOpen;

    for (Slot& slot : storageSlots) {
        sanitize(slot);
    }
    for (Slot& slot : craftSlots) {
        sanitize(slot);
    }
    sanitize(heldSlot);
}
#pragma endregion
