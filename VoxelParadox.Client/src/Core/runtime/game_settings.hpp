// Arquivo: VoxelParadox.Client/src/Core/runtime/game_settings.hpp
// Papel: declara "game settings" dentro do subsistema "runtime" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma once

#pragma region Includes

#include <filesystem>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

#include "client_defaults.hpp"
#include "audio/audio_types.hpp"
#include "engine/engine.hpp"
#include "world/world_stack.hpp"
#pragma endregion

#pragma region GameSettingsApi
struct GameSettings {
  std::string fontFile = ClientDefaults::kDefaultFontFile;
  glm::ivec2 resolution{ClientDefaults::kDefaultWindowedResolution};
  WorldStack::RenderDistancePreset renderDistance =
      WorldStack::RenderDistancePreset::NORMAL;
  float mouseSensitivity = 0.002f;
  float renderScale = ClientDefaults::kDefaultRenderScale;
  ENGINE::VIEWPORTMODE windowMode = ENGINE::VIEWPORTMODE::BORDERLESS;
  bool vSyncEnabled = false;
  bool showFpsCounterOnly = false;
  ENGINE::AUDIO::AudioSettings audioSettings{};

  // Funcao: executa 'fontAssetPath' na configuracao persistente do runtime.
  // Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
  // Retorno: devolve 'std::string' com o texto pronto para exibicao, lookup ou serializacao.
  std::string fontAssetPath() const;
  // Funcao: executa 'fontDisplayName' na configuracao persistente do runtime.
  // Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
  // Retorno: devolve 'std::string' com o texto pronto para exibicao, lookup ou serializacao.
  std::string fontDisplayName() const;

  // Funcao: normaliza 'sanitize' na configuracao persistente do runtime.
  // Detalhe: usa 'availableFonts', 'availableResolutions' para limpar a entrada para reduzir inconsistencias antes do uso.
  void sanitize(const std::vector<std::string>& availableFonts,
                const std::vector<glm::ivec2>& availableResolutions = {});
  // Funcao: salva 'save' na configuracao persistente do runtime.
  // Detalhe: usa 'outError' para persistir os dados recebidos no formato esperado pelo projeto.
  // Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
  bool save(std::string* outError = nullptr) const;

  // Funcao: carrega 'load' na configuracao persistente do runtime.
  // Detalhe: usa 'outError' para ler dados externos e adapta-los ao formato interno usado pelo jogo.
  // Retorno: devolve 'GameSettings' com o resultado composto por esta chamada.
  static GameSettings load(std::string* outError = nullptr);
  // Funcao: executa 'availableFonts' na configuracao persistente do runtime.
  // Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
  // Retorno: devolve 'std::vector<std::string>' com o texto pronto para exibicao, lookup ou serializacao.
  static std::vector<std::string> availableFonts();
  // Funcao: executa 'availableDisplayResolutions' na configuracao persistente do runtime.
  // Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
  // Retorno: devolve 'std::vector<glm::ivec2>' com a colecao ou o resultado agregado montado por esta etapa.
  static std::vector<glm::ivec2> availableDisplayResolutions();
  // Funcao: define 'settingsDirectory' na configuracao persistente do runtime.
  // Detalhe: centraliza a logica necessaria para aplicar ao componente o valor ou configuracao recebida.
  // Retorno: devolve 'std::filesystem::path' com o resultado composto por esta chamada.
  static std::filesystem::path settingsDirectory();
  // Funcao: define 'settingsFilePath' na configuracao persistente do runtime.
  // Detalhe: centraliza a logica necessaria para aplicar ao componente o valor ou configuracao recebida.
  // Retorno: devolve 'std::filesystem::path' com o resultado composto por esta chamada.
  static std::filesystem::path settingsFilePath();
};
#pragma endregion
