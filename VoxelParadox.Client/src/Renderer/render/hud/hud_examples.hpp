// Arquivo: VoxelParadox.Client/src/Renderer/render/hud/hud_examples.hpp
// Papel: declara "hud examples" dentro do subsistema "render hud" do projeto VoxelParadox.Client.
// Fluxo: concentra tipos, dados e rotinas usados por este ponto do runtime de forma documentada e consistente.
// Dependencias principais: os headers, tipos STL e bibliotecas externas incluidos logo abaixo.

#pragma once

#pragma region HudExamplesApi
/**
 * HUD System - Usage Examples
 *
 * This file contains practical examples of how to use the HUD system
 * in various scenarios.
 */

#include "hud.hpp"
#include "hud_text.hpp"
#include "hud_image.hpp"
#include <glm/glm.hpp>

// ============================================================================
// EXAMPLE 1: Basic HUD Setup
// ============================================================================
// Funcao: executa 'Example_BasicSetup' nos exemplos de integracao do HUD.
// Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
void Example_BasicSetup() {
    // Initialize HUD system
    if (!HUD::init()) {
        printf("HUD initialization failed\n");
        return;
    }

    // Set default font (optional but recommended)
    HUD::setDefaultFont("assets/RobotoMono-Regular.ttf");

    // Create simple text element
    hudText* helloText = new hudText(
        "Hello World",              // Text content
        100, 100,                   // Position (x, y)
        glm::vec2(1.0f, 1.0f), // Scale
        24                          // Font size
    );

    // Add to HUD for rendering
    HUD::add(helloText);

    // In render loop:
    // HUD::render(screenWidth, screenHeight);

    // Cleanup when done:
    // HUD::clear();
}

// ============================================================================
// EXAMPLE 2: Game HUD with Stats
// ============================================================================
class GameHUD {
private:
    hudText* textDepth = nullptr;
    hudText* textPosition = nullptr;
    hudText* textFPS = nullptr;
    hudImage* imageCrosshair = nullptr;

public:
    // Funcao: inicializa 'init' nos exemplos de integracao do HUD.
    // Detalhe: centraliza a logica necessaria para preparar os recursos e o estado inicial antes do uso.
    void init() {
        HUD::init();
        HUD::setDefaultFont("assets/RobotoMono-Regular.ttf");

        // Top-left stats
        textDepth = new hudText(
            "Depth: 0",
            10, 10,
            glm::vec2(1.2f, 1.2f),
            18
        );

        textPosition = new hudText(
            "Pos: 0, 0, 0",
            10, 35,
            glm::vec2(1.0f, 1.0f),
            16
        );

        textFPS = new hudText(
            "FPS: 60",
            10, 55,
            glm::vec2(1.0f, 1.0f),
            16
        );

        // Center crosshair
        imageCrosshair = new hudImage(
            "assets/hud/crosshair.png",
            640 - 16,  // Center X (1280/2 - 32/2)
            360 - 16,  // Center Y (720/2 - 32/2)
            glm::vec3(32.0f, 32.0f, 1.0f),
            ImageType::STRETCH
        );

        HUD::add(textDepth);
        HUD::add(textPosition);
        HUD::add(textFPS);
        HUD::add(imageCrosshair);
    }

    // Funcao: atualiza 'updateStats' nos exemplos de integracao do HUD.
    // Detalhe: usa 'depth', 'position', 'fps' para sincronizar o estado derivado com o frame atual.
    void updateStats(int depth, const glm::vec3& position, float fps) {
        // Note: Current implementation requires recreating elements for dynamic text
        // Future enhancement: Add setText() method for dynamic updates

        char depthStr[64];
        snprintf(depthStr, sizeof(depthStr), "Depth: %d", depth);

        char posStr[128];
        snprintf(posStr, sizeof(posStr), "Pos: %.1f, %.1f, %.1f",
                 position.x, position.y, position.z);

        char fpsStr[64];
        snprintf(fpsStr, sizeof(fpsStr), "FPS: %.1f", fps);

        // Would need to delete old and create new elements
        // Or implement setText() for in-place updates
    }

    // Funcao: libera 'cleanup' nos exemplos de integracao do HUD.
    // Detalhe: centraliza a logica necessaria para encerrar a etapa e descartar recursos associados.
    void cleanup() {
        HUD::clear();
    }
};

// ============================================================================
// EXAMPLE 3: Menu System with Text
// ============================================================================
class MenuHUD {
private:
    std::vector<hudText*> menuItems;

public:
    // Funcao: inicializa 'init' nos exemplos de integracao do HUD.
    // Detalhe: centraliza a logica necessaria para preparar os recursos e o estado inicial antes do uso.
    void init() {
        HUD::init();
        HUD::setDefaultFont("assets/RobotoMono-Regular.ttf");

        // Title
        hudText* title = new hudText(
            "MAIN MENU",
            640 - 120, 100,  // Approximate center
            glm::vec2(2.0f, 2.0f),
            32
        );
        HUD::add(title);

        // Menu items
        const char* items[] = { "New Game", "Continue", "Settings", "Exit" };
        int startY = 250;
        int itemHeight = 50;

        for (int i = 0; i < 4; i++) {
            hudText* item = new hudText(
                items[i],
                640 - 80,
                startY + (i * itemHeight),
                glm::vec2(1.5f, 1.5f),
                24
            );
            HUD::add(item);
            menuItems.push_back(item);
        }
    }

    // Funcao: libera 'cleanup' nos exemplos de integracao do HUD.
    // Detalhe: centraliza a logica necessaria para encerrar a etapa e descartar recursos associados.
    void cleanup() {
        menuItems.clear();
        HUD::clear();
    }
};

// ============================================================================
// EXAMPLE 4: Minimap with Image
// ============================================================================
// Funcao: executa 'Example_Minimap' nos exemplos de integracao do HUD.
// Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
void Example_Minimap() {
    HUD::init();
    HUD::setDefaultFont("assets/RobotoMono-Regular.ttf");

    // Minimap background image (top-right corner)
    hudImage* minimap = new hudImage(
        "assets/minimap.png",
        1280 - 200,  // Right side
        10,          // Top
        glm::vec3(190.0f, 190.0f, 1.0f),  // 190x190 pixels
        ImageType::STRETCH
    );

    // Minimap label
    hudText* minimapLabel = new hudText(
        "MINIMAP",
        1280 - 190, 200,  // Below minimap
        glm::vec2(1.0f, 1.0f),
        16
    );

    HUD::add(minimap);
    HUD::add(minimapLabel);
}

// ============================================================================
// EXAMPLE 5: Health/Status Bar Text
// ============================================================================
// Funcao: executa 'Example_StatusDisplay' nos exemplos de integracao do HUD.
// Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
void Example_StatusDisplay() {
    HUD::init();
    HUD::setDefaultFont("assets/RobotoMono-Regular.ttf");

    // Health status
    hudText* healthLabel = new hudText(
        "HEALTH",
        10, 600,
        glm::vec2(1.0f, 1.0f),
        14
    );

    hudText* healthValue = new hudText(
        "100/100",
        100, 600,
        glm::vec2(1.0f, 1.0f),
        14
    );

    // Energy status
    hudText* energyLabel = new hudText(
        "ENERGY",
        10, 625,
        glm::vec2(1.0f, 1.0f),
        14
    );

    hudText* energyValue = new hudText(
        "75/100",
        100, 625,
        glm::vec2(1.0f, 1.0f),
        14
    );

    HUD::add(healthLabel);
    HUD::add(healthValue);
    HUD::add(energyLabel);
    HUD::add(energyValue);
}

// ============================================================================
// EXAMPLE 6: Custom Font Text
// ============================================================================
// Funcao: executa 'Example_CustomFonts' nos exemplos de integracao do HUD.
// Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
void Example_CustomFonts() {
    HUD::init();

    // Title with Arial (serif-like)
    hudText* titleArial = new hudText(
        "Game Title",
        640 - 100,
        50,
        glm::vec2(3.0f, 3.0f),
        40,
        "C:\\Windows\\Fonts\\Arial.ttf"  // Custom font path
    );

    // Body text with Courier New (monospace)
    hudText* bodyText = new hudText(
        "This is monospace text",
        100,
        150,
        glm::vec2(1.0f, 1.0f),
        16,
        "C:\\Windows\\Fonts\\cour.ttf"  // Courier New
    );

    HUD::add(titleArial);
    HUD::add(bodyText);
}

// ============================================================================
// EXAMPLE 7: Scaling Variations
// ============================================================================
// Funcao: executa 'Example_ScalingTypes' nos exemplos de integracao do HUD.
// Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
void Example_ScalingTypes() {
    HUD::init();

    // Image with STRETCH (exact dimensions)
    hudImage* stretchImg = new hudImage(
        "assets/logo.png",
        50, 50,
        glm::vec3(256.0f, 128.0f, 1.0f),  // 256x128 exact
        ImageType::STRETCH
    );

    // Image with FILL (maintain aspect ratio)
    hudImage* fillImg = new hudImage(
        "assets/logo.png",
        50, 200,
        glm::vec3(256.0f, 256.0f, 1.0f),  // 256x256 max bounds
        ImageType::FILL                     // Will maintain original ratio
    );

    HUD::add(stretchImg);
    HUD::add(fillImg);
}

// ============================================================================
// EXAMPLE 8: Text at Different Sizes
// ============================================================================
// Funcao: executa 'Example_TextSizes' nos exemplos de integracao do HUD.
// Detalhe: centraliza a logica necessaria para encapsular esta etapa especifica do subsistema.
void Example_TextSizes() {
    HUD::init();
    HUD::setDefaultFont("assets/RobotoMono-Regular.ttf");

    // Tiny text
    hudText* tinyText = new hudText("8px font", 10, 10, glm::vec2(0.5f, 0.5f), 8);

    // Small text
    hudText* smallText = new hudText("12px font", 10, 30, glm::vec2(0.8f, 0.8f), 12);

    // Normal text
    hudText* normalText = new hudText("16px font", 10, 50, glm::vec2(1.0f, 1.0f), 16);

    // Large text
    hudText* largeText = new hudText("24px font", 10, 70, glm::vec2(1.5f, 1.5f), 24);

    // Huge text
    hudText* hugeText = new hudText("48px font", 10, 100, glm::vec2(2.0f, 2.0f), 48);

    HUD::add(tinyText);
    HUD::add(smallText);
    HUD::add(normalText);
    HUD::add(largeText);
    HUD::add(hugeText);
}

// ============================================================================
// EXAMPLE 9: Screen-Space Positioning
// ============================================================================
// Funcao: executa 'Example_ScreenPositioning' nos exemplos de integracao do HUD.
// Detalhe: usa 'screenWidth', 'screenHeight' para encapsular esta etapa especifica do subsistema.
void Example_ScreenPositioning(int screenWidth, int screenHeight) {
    HUD::init();
    HUD::setDefaultFont("assets/RobotoMono-Regular.ttf");

    // Top-left
    hudText* topLeft = new hudText(
        "Top-Left",
        10, 10,
        glm::vec2(1.0f, 1.0f),
        16
    );

    // Top-right
    hudText* topRight = new hudText(
        "Top-Right",
        screenWidth - 120, 10,
        glm::vec2(1.0f, 1.0f),
        16
    );

    // Bottom-left
    hudText* bottomLeft = new hudText(
        "Bottom-Left",
        10, screenHeight - 30,
        glm::vec2(1.0f, 1.0f),
        16
    );

    // Bottom-right
    hudText* bottomRight = new hudText(
        "Bottom-Right",
        screenWidth - 140, screenHeight - 30,
        glm::vec2(1.0f, 1.0f),
        16
    );

    // Center
    hudText* center = new hudText(
        "Center",
        screenWidth / 2 - 40, screenHeight / 2,
        glm::vec2(2.0f, 2.0f),
        24
    );

    HUD::add(topLeft);
    HUD::add(topRight);
    HUD::add(bottomLeft);
    HUD::add(bottomRight);
    HUD::add(center);
}

// ============================================================================
// EXAMPLE 10: Integrated into Main Render Loop
// ============================================================================
// Funcao: executa 'Example_RenderLoopIntegration' nos exemplos de integracao do HUD.
// Detalhe: usa 'window' para encapsular esta etapa especifica do subsistema.
void Example_RenderLoopIntegration(GLFWwindow* window) {
    HUD::init();
    HUD::setDefaultFont("assets/RobotoMono-Regular.ttf");

    // Create HUD elements
    auto* fpsText = new hudText("FPS: 0", 10, 10, glm::vec2(1.0f, 1.0f), 16);
    auto* crosshair = new hudImage(
        "assets/hud/crosshair.png",
        640 - 16, 360 - 16,
        glm::vec3(32.0f, 32.0f, 1.0f),
        ImageType::STRETCH
    );

    HUD::add(fpsText);
    HUD::add(crosshair);

    // Render loop
    float lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float dt = currentTime - lastTime;
        lastTime = currentTime;

        float fps = 1.0f / dt;

        // ... Render world ...

        // Render HUD (last, after world)
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        HUD::render(width, height);

        glfwSwapBuffers(window);
    }

    HUD::clear();
}
#pragma endregion
