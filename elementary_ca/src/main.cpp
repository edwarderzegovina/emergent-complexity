#include "raylib.h"

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#define RULE 30
#define FRAMES 30

namespace {
constexpr int kScreenWidth = 1400;
constexpr int kScreenHeight = 900;
constexpr int kUiHeight = 80;
constexpr int kCellSize = 10;
constexpr float kStepIntervalSeconds = 0.1f;

std::vector<uint8_t> ComputeNextRow(const std::vector<uint8_t> &prev, int rule) {
    std::vector<uint8_t> next(prev.size(), 0);
    if (prev.empty()) {
        return next;
    }
    const int last = static_cast<int>(prev.size()) - 1;
    for (int x = 0; x < static_cast<int>(prev.size()); ++x) {
        int left = (x == 0) ? prev[last] : prev[x - 1];
        int center = prev[x];
        int right = (x == last) ? prev[0] : prev[x + 1];
        int index = (left << 2) | (center << 1) | right; // 0b000..0b111
        next[x] = static_cast<uint8_t>((rule >> index) & 1);
    }
    return next;
}

std::vector<uint8_t> MakeInitialRow(int cols) {
    std::vector<uint8_t> row(cols, 0);
    if (cols > 0) {
        row[cols / 2] = 1;
    }
    return row;
}

std::vector<uint8_t> MakeRandomRow(int cols) {
    std::vector<uint8_t> row(cols, 0);
    for (int x = 0; x < cols; ++x) {
        row[x] = static_cast<uint8_t>(GetRandomValue(0, 1));
    }
    return row;
}
} // namespace

int main() {
    InitWindow(kScreenWidth, kScreenHeight, "Elementary Cellular Automata");
    SetTargetFPS(FRAMES);

    const int cols = kScreenWidth / kCellSize;
    int rows = (kScreenHeight - kUiHeight) / kCellSize;
    if (rows < 1) {
        rows = 1;
    }

    int rule = RULE;
    bool paused = false;
    float accumulator = 0.0f;
    std::deque<std::vector<uint8_t>> history;
    Rectangle randomButton{static_cast<float>(kScreenWidth - 300), 20.0f, 120.0f, 32.0f};

    auto reset = [&]() {
        history.clear();
        history.push_back(MakeInitialRow(cols));
        accumulator = 0.0f;
    };

    auto randomize = [&]() {
        history.clear();
        history.push_back(MakeRandomRow(cols));
        accumulator = 0.0f;
    };

    auto step = [&]() {
        if (history.empty()) {
            return;
        }
        history.push_back(ComputeNextRow(history.back(), rule));
        if (static_cast<int>(history.size()) > rows) {
            history.pop_front();
        }
    };

    reset();

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) {
            paused = !paused;
        }
        if (IsKeyPressed(KEY_R)) {
            reset();
        }
        if (IsKeyPressed(KEY_S)) {
            step();
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            rule = (rule + 1) & 255;
            reset();
        }
        if (IsKeyPressed(KEY_LEFT)) {
            rule = (rule + 255) & 255;
            reset();
        }
        if (IsKeyPressed(KEY_N)) {
            randomize();
        }
        Vector2 mouse = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(mouse, randomButton)) {
            randomize();
        }

        float dt = GetFrameTime();
        if (!paused) {
            accumulator += dt;
            while (accumulator >= kStepIntervalSeconds) {
                step();
                accumulator -= kStepIntervalSeconds;
            }
        }

        BeginDrawing();
        ClearBackground(Color{10, 10, 14, 255});

        DrawText(TextFormat("Rule: %d", rule), 20, 20, 20, RAYWHITE);
        DrawText(
            "Left/Right: rule  Space: pause  S: step  R: reset  N: random",
            20,
            45,
            18,
            GRAY);
        bool hovered = CheckCollisionPointRec(mouse, randomButton);
        Color buttonColor = hovered ? Color{60, 60, 80, 255} : Color{40, 40, 60, 255};
        DrawRectangleRec(randomButton, buttonColor);
        DrawRectangleLinesEx(randomButton, 2.0f, Color{120, 120, 140, 255});
        DrawText("Random", static_cast<int>(randomButton.x + 18), static_cast<int>(randomButton.y + 8), 18, RAYWHITE);
        if (paused) {
            DrawText("Paused", kScreenWidth - 120, 20, 20, ORANGE);
        }

        int rowIndex = 0;
        for (const auto &row : history) {
            int y = kUiHeight + rowIndex * kCellSize;
            for (int x = 0; x < cols; ++x) {
                if (row[x]) {
                    DrawRectangle(x * kCellSize, y, kCellSize, kCellSize, RAYWHITE);
                }
            }
            ++rowIndex;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
