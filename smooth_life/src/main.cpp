#include "SmoothLifeEngine.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include <raylib.h>

namespace {
constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 800;
constexpr int kGridWidth = 256;
constexpr int kGridHeight = 256;
constexpr int kBrushMin = 1;
constexpr int kBrushMax = 40;

Color heatmap(float value) {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    const unsigned char r = static_cast<unsigned char>(50.0f + 205.0f * clamped);
    const unsigned char g = static_cast<unsigned char>(20.0f + 235.0f * std::sqrt(clamped));
    const unsigned char b = static_cast<unsigned char>(140.0f * (1.0f - clamped) + 15.0f);
    return {r, g, b, 255};
}
}  // namespace

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(kWindowWidth, kWindowHeight, "SmoothLife - raylib");
    SetTargetFPS(60);

    SmoothLifeSettings settings{};
    settings.width = kGridWidth;
    settings.height = kGridHeight;

    SmoothLifeEngine engine(settings);

    Image image = GenImageColor(kGridWidth, kGridHeight, BLACK);
    auto* pixels = static_cast<Color*>(image.data);
    Texture2D texture = LoadTextureFromImage(image);

    bool paused = false;
    int brushRadius = 4;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) {
            paused = !paused;
        }
        if (IsKeyPressed(KEY_R)) {
            engine.randomize();
        }
        if (IsKeyPressed(KEY_C)) {
            engine.clear();
        }
        if (IsKeyPressed(KEY_UP)) {
            engine.setTimeStep(engine.timeStep() + 0.01f);
        }
        if (IsKeyPressed(KEY_DOWN)) {
            engine.setTimeStep(engine.timeStep() - 0.01f);
        }
        if (IsKeyPressed(KEY_LEFT_BRACKET)) {
            engine.setRadii(engine.innerRadius() - 0.5f, engine.outerRadius() - 0.5f);
        }
        if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
            engine.setRadii(engine.innerRadius() + 0.5f, engine.outerRadius() + 0.5f);
        }

        brushRadius += GetMouseWheelMove();
        brushRadius = std::clamp(brushRadius, kBrushMin, kBrushMax);

        const int windowWidth = GetScreenWidth();
        const int windowHeight = GetScreenHeight();
        const float scaleX = static_cast<float>(windowWidth) / static_cast<float>(kGridWidth);
        const float scaleY = static_cast<float>(windowHeight) / static_cast<float>(kGridHeight);
        const float drawScale = std::min(scaleX, scaleY);

        const float drawWidth = kGridWidth * drawScale;
        const float drawHeight = kGridHeight * drawScale;
        const float drawX = (windowWidth - drawWidth) * 0.5f;
        const float drawY = (windowHeight - drawHeight) * 0.5f;
        Rectangle dest = {drawX, drawY, drawWidth, drawHeight};

        const Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, dest)) {
            const float normalizedX = (mouse.x - drawX) / drawWidth;
            const float normalizedY = (mouse.y - drawY) / drawHeight;
            const int gridX = static_cast<int>(normalizedX * static_cast<float>(kGridWidth));
            const int gridY = static_cast<int>(normalizedY * static_cast<float>(kGridHeight));

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                engine.splat(gridX, gridY, brushRadius, 1.0f);
            }
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                engine.splat(gridX, gridY, brushRadius, 0.0f);
            }
        }

        if (!paused) {
            engine.update();
        }

        const std::vector<float>& cells = engine.cells();
        for (int y = 0; y < kGridHeight; ++y) {
            for (int x = 0; x < kGridWidth; ++x) {
                const int index = y * kGridWidth + x;
                pixels[index] = heatmap(cells[index]);
            }
        }
        UpdateTexture(texture, pixels);

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(texture, {0, 0, static_cast<float>(kGridWidth), static_cast<float>(kGridHeight)}, dest, {0, 0}, 0.0f,
                       WHITE);

        const int panelX = 12;
        int panelY = 12;
        DrawRectangle(panelX - 8, panelY - 8, 450, 146, Fade(BLACK, 0.55f));
        DrawText("SmoothLife", panelX, panelY, 24, RAYWHITE);
        panelY += 30;

        DrawText(paused ? "[SPACE] Resume" : "[SPACE] Pause", panelX, panelY, 18, RAYWHITE);
        panelY += 22;
        DrawText("[R] Randomize  [C] Clear", panelX, panelY, 18, RAYWHITE);
        panelY += 22;
        DrawText("Mouse Left: add mass   Mouse Right: erase", panelX, panelY, 18, RAYWHITE);
        panelY += 22;
        DrawText("Mouse Wheel: brush size   [UP/DOWN]: dt", panelX, panelY, 18, RAYWHITE);
        panelY += 22;
        DrawText("[ / ]: change neighborhood radii", panelX, panelY, 18, RAYWHITE);

        char status[256] = {};
        std::snprintf(status, sizeof(status), "dt %.3f  inner %.1f  outer %.1f  brush %d", engine.timeStep(),
                      engine.innerRadius(), engine.outerRadius(), brushRadius);
        DrawText(status, panelX, panelY + 24, 18, YELLOW);

        EndDrawing();
    }

    UnloadTexture(texture);
    UnloadImage(image);
    CloseWindow();
    return 0;
}
