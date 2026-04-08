# Client Reading Guide

Este guia existe para te ajudar a navegar o `VoxelParadox.Client` sem precisar entender o projeto inteiro de uma vez.

## Mapa de pastas

- `src/Core` = bootstrap, loop principal, configuracao compartilhada e suporte.
- `src/Systems` = integracoes de audio e ferramentas de debug.
- `src/Entities` = player, inimigos e catalogos de itens.
- `src/Renderer` = renderer principal, assets de render e previews.
- `src/UI` = camadas de ImGui e janelas auxiliares.
- `src/World` = biomas, geracao e pilha de mundos.

## Se voce vem de Unity

- `runtime_app.cpp` = bootstrap principal em alto nivel.
- `runtime_app_bootstrap.cpp` = validacao e preparacao antes do jogo comecar.
- `runtime_app_loop.cpp` = loop principal por frame.
- `runtime_ui.cpp` = ponto de entrada da UI e atalhos globais.
- `player.cpp` / `player_*.cpp` = `CharacterController` + camera + gameplay do player.
- `biome_preset_defaults.cpp` = ids, defaults e factories do gerador.
- `biome_preset.cpp` = serializacao JSON e load/save de biome presets.
- `world_stack.cpp` / `world_stack_persistence.cpp` = session manager + save/load.
- `renderer.cpp` = coordenador do frame 3D.
- `renderer_items.cpp` = previews de HUD, item na mao, poeira e dropped items.
- `renderer_portals.cpp` = stencil, preview de portal e debug de camera.
- `game_settings.*` = configuracao persistida do jogo.

## Ordem recomendada de leitura

1. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Core\runtime\runtime_app.cpp`
   Aqui o jogo comeca. Agora esse arquivo ficou curto e so orquestra as etapas principais.

2. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Core\runtime\runtime_app_bootstrap.cpp`
   Validacao de presets, loading de settings, logs de bootstrap e preparacao do mundo raiz.

3. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Core\runtime\runtime_app_loop.cpp`
   Loop por frame: input, gameplay, audio, render e rebuild de HUD.

4. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Core\runtime\runtime_ui.cpp`
   Ponto de entrada pequeno do sistema de UI.

5. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Core\runtime\runtime_ui_hud.cpp`
   Montagem de menus e HUD.

6. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Core\runtime\runtime_ui_common.cpp`
   Helpers compartilhados da UI.

7. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Entities\player\player.hpp`
   Leia o estado do player antes dos `.cpp`.

8. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Entities\player\player.cpp`
   Orquestracao principal do frame do player.

9. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Entities\player\player_movement.cpp`
   Movimento, camera e colisao.

10. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Entities\player\player_interaction.cpp`
   Raycast, quebrar bloco, colocar bloco e drop.

11. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Entities\player\player_portal.cpp`
   Preview de nested world e transicoes de portal.

12. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\World\world\world_stack.hpp`
    Header principal do sistema de mundos.

13. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\World\world\world_stack.cpp`
    Nucleo do `WorldStack`: ciclo de vida, mundo ativo e traversal.

14. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\World\world\world_stack_persistence.cpp`
    Save/load, cache e resolucao de mundos/portais.

15. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\World\world\biome_preset.hpp`
    Tipos do preset de biome. Leia antes dos `.cpp` do gerador/preset.

16. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\World\world\biome_preset_defaults.cpp`
    IDs, display names, defaults legados e factories dos modulos.

17. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\World\world\biome_preset.cpp`
    Conversao para JSON e leitura/escrita de arquivos de preset/modulo.

18. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Renderer\render\renderer.hpp`
    Header principal do renderer. Leia antes dos `.cpp`.

19. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Renderer\render\renderer.cpp`
    Setup/cleanup, frame principal e transicoes globais.

20. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Renderer\render\renderer_items.cpp`
    Previews 3D de HUD, item equipado, poeira e dropped items.

21. `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Renderer\render\renderer_portals.cpp`
    Portal preview, stencil mask e debug de frustum.

## Arquivos centrais para reduzir hardcode

- `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Core\config\client_defaults.hpp`
  Defaults globais do client.

- `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Core\config\client_assets.hpp`
  Paths centrais de assets e diretorios.

- `C:\Users\nikkkkoooooo\DEV\cpp\VoxelParadox\VoxelParadox.Client\src\Core\runtime\runtime_hud_ids.hpp`
  IDs e layers de HUD concentrados em um lugar.

## Onde procurar cada tipo de problema

- Bug de movimento/input: `Entities/player/*`, `Core/runtime/runtime_app.cpp`, `Core/runtime/runtime_app_loop.cpp`, `Core/runtime/game_chat.cpp`, `Core/runtime/runtime_ui.cpp`
- Bug de inventario/hotbar/crafting: `Entities/player/hotbar.*`, `Entities/player/crafting.cpp`, `Renderer/render/hud/hud_inventory_menu.cpp`
- Bug de menu/settings: `Core/runtime/runtime_ui.cpp`, `Core/runtime/game_settings.*`
- Bug de mundo/save/portal: `World/world/world_stack.*`, `World/world/biome_preset.*`
- Bug visual/render geral: `Renderer/render/renderer.cpp`, `Renderer/render/renderer_items.cpp`, `Renderer/render/renderer_portals.cpp`, `Renderer/render/hud/*`

## Proxima simplificacao recomendada

Os proximos pontos com mais densidade ainda sao:

- `renderer.cpp`, porque ainda carrega shaders inline muito grandes.
- `biome_preset.hpp`, porque ainda concentra muita configuracao de geracao em structs grandes.
- `game_settings.cpp`, porque ainda mistura validacao, persistencia e discovery de opcoes.
