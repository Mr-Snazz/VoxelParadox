// Arquivo: VoxelParadox.Client/src/UI/ui/biome_teleport_window.cpp
// Papel: implementa "biome teleport window" dentro do subsistema "ui" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma region Includes
#include "biome_teleport_window.hpp"

#include <algorithm>

#include <imgui.h>
#pragma endregion

#pragma region BiomeTeleportWindowImplementation
namespace BiomeTeleportWindow {
namespace {

std::vector<BiomeSelection> availableBiomes;
BiomeSelection currentBiome;
BiomeSelection selectedBiome;
bool selectionInitialized = false;
bool teleportRequested = false;
bool updateLocationRequested = false;

} // namespace

// Funcao: define 'setAvailableBiomes' na janela de teleporte de biomes.
// Detalhe: usa 'biomes' para aplicar ao componente o valor ou configuracao recebida.
void setAvailableBiomes(const std::vector<BiomeSelection>& biomes) {
  availableBiomes = biomes;
  if (availableBiomes.empty()) {
    selectionInitialized = false;
    return;
  }

  const auto selectedIt =
      std::find(availableBiomes.begin(), availableBiomes.end(), selectedBiome);
  if (!selectionInitialized || selectedIt == availableBiomes.end()) {
    selectedBiome = availableBiomes.front();
    selectionInitialized = true;
  }
}

// Funcao: define 'setSelectedBiome' na janela de teleporte de biomes.
// Detalhe: usa 'biome' para aplicar ao componente o valor ou configuracao recebida.
void setSelectedBiome(const BiomeSelection& biome) {
  selectedBiome = biome;
  selectionInitialized = true;
}

// Funcao: executa 'syncCurrentBiome' na janela de teleporte de biomes.
// Detalhe: usa 'biome' para encapsular esta etapa especifica do subsistema.
void syncCurrentBiome(const BiomeSelection& biome) {
  currentBiome = biome;
  if (!selectionInitialized) {
    selectedBiome = biome;
    selectionInitialized = true;
  }
}

// Funcao: renderiza 'draw' na janela de teleporte de biomes.
// Detalhe: centraliza a logica necessaria para desenhar a saida visual correspondente usando o estado atual.
void draw() {
  ImGui::SetNextWindowPos(ImVec2(24.0f, 180.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(320.0f, 140.0f), ImGuiCond_FirstUseEver);

  if (!ImGui::Begin("Biome Teleport")) {
    ImGui::End();
    return;
  }

  ImGui::Text("Current: %s", currentBiome.displayName.c_str());

  if (ImGui::BeginCombo("Biome", selectedBiome.displayName.c_str())) {
    for (const BiomeSelection& biome : availableBiomes) {
      const bool selected = biome == selectedBiome;
      if (ImGui::Selectable(biome.displayName.c_str(), selected)) {
        selectedBiome = biome;
        selectionInitialized = true;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  if (ImGui::Button("Teleport")) {
    teleportRequested = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Update Location")) {
    updateLocationRequested = true;
  }

  ImGui::End();
}

// Funcao: libera 'shutdown' na janela de teleporte de biomes.
// Detalhe: centraliza a logica necessaria para encerrar a etapa e descartar recursos associados.
void shutdown() {
  availableBiomes.clear();
  currentBiome = {};
  selectedBiome = {};
  selectionInitialized = false;
  teleportRequested = false;
  updateLocationRequested = false;
}

// Funcao: consome 'consumeTeleportRequest' na janela de teleporte de biomes.
// Detalhe: usa 'outBiome' para retirar um pedido ou evento pendente para evitar reprocessamento.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool consumeTeleportRequest(BiomeSelection& outBiome) {
  if (!teleportRequested) {
    return false;
  }

  teleportRequested = false;
  outBiome = selectedBiome;
  return true;
}

// Funcao: consome 'consumeUpdateLocationRequest' na janela de teleporte de biomes.
// Detalhe: centraliza a logica necessaria para retirar um pedido ou evento pendente para evitar reprocessamento.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool consumeUpdateLocationRequest() {
  if (!updateLocationRequested) {
    return false;
  }

  updateLocationRequested = false;
  return true;
}

} // namespace BiomeTeleportWindow
#pragma endregion
