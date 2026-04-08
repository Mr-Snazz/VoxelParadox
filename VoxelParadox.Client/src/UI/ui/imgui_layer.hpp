// Arquivo: VoxelParadox.Client/src/UI/ui/imgui_layer.hpp
// Papel: declara "imgui layer" dentro do subsistema "ui" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma once

#pragma region ImguiLayerApi
struct GLFWwindow;

namespace ImGuiLayer {

// Funcao: inicializa 'initialize' na camada ImGui do runtime.
// Detalhe: usa 'window' para preparar os recursos e o estado inicial antes do uso.
// Retorno: devolve 'bool' para indicar sucesso, presenca, validacao ou qualquer outra condicao relevante produzida pela chamada.
bool initialize(GLFWwindow* window);
// Funcao: inicia 'beginFrame' na camada ImGui do runtime.
// Detalhe: centraliza a logica necessaria para marcar o inicio de uma transicao ou fluxo controlado.
void beginFrame();
// Funcao: renderiza 'render' na camada ImGui do runtime.
// Detalhe: centraliza a logica necessaria para desenhar a saida visual correspondente usando o estado atual.
void render();
// Funcao: libera 'shutdown' na camada ImGui do runtime.
// Detalhe: centraliza a logica necessaria para encerrar a etapa e descartar recursos associados.
void shutdown();

} // namespace ImGuiLayer
#pragma endregion
