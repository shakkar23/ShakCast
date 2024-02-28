#include <array>
#include <iostream>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

int get_spacing(int font_size) {
    int defaultFontSize = 10;  // Default Font chars height in pixel
    if (font_size < defaultFontSize) font_size = defaultFontSize;
    return font_size / defaultFontSize;
}

int fit_text(const char *text, int max_width, int max_height) {
    int font_size = 1;
    int spacing = get_spacing(font_size);
    Vector2 box = MeasureTextEx(GetFontDefault(), text, font_size, spacing);
    while (box.x < max_width && box.y < max_height) {
        font_size++;
        spacing = get_spacing(font_size);
        box = MeasureTextEx(GetFontDefault(), text, font_size, spacing);
    }
    return font_size - 1;
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main() {
    constexpr bool is_android = false;
    // Initialization
    //---------------------------------------------------------------------------------------
    int screenWidth = is_android ? 0 : 450;
    int screenHeight = is_android ? 0 : 700;

    // SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(screenWidth, screenHeight, "thing");

    // General variables
    // GuiSetStyle(DEFAULT, TEXT_SIZE);
    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())  // Detect window close button or ESC key
    {
        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();

        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();
        ClearBackground(RAYWHITE);
        int bar_height = screenHeight / 20;
        DrawRectangle(0, 0, screenWidth, bar_height, MAROON);

        const char *text = "Title";
        int title_font_size = fit_text(text, screenWidth - 20, bar_height);
        int spacing = get_spacing(title_font_size);

        Vector2 text_size = MeasureTextEx(GetFontDefault(), text, title_font_size, spacing);

        DrawText(text,
                 (screenWidth - (int)text_size.x) / 2,
                 (bar_height - text_size.y) / 2,
                 title_font_size, RAYWHITE);

        // draw 5x5 grid of tiles that you can put a single letter in
        const float tile_size = screenWidth * 0.9 / 5;
        const float padding = (screenWidth - tile_size * 5) / 6;
        {
            for (int i = 0; i < 5; i++) {
                for (int j = 0; j < 5; j++) {
                    DrawRectangle(i * tile_size + padding * (i + 1), j * tile_size + bar_height + padding * (j + 1), tile_size, tile_size, DARKGRAY);
                }
            }
        }

        static std::array<char, 25 * 2> tiles = {'a', '\0', 'b', '\0', 'c', '\0', 'd', '\0', 'e', '\0',
                                                 'f', '\0', 'g', '\0', 'h', '\0', 'i', '\0', 'j', '\0',
                                                 'k', '\0', 'l', '\0', 'm', '\0', 'n', '\0', 'o', '\0',
                                                 'p', '\0', 'q', '\0', 'r', '\0', 's', '\0', 't', '\0',
                                                 'u', '\0', 'v', '\0', 'w', '\0', 'x', '\0', 'y', '\0'};

        static std::array<bool, 25> selected = {false};

        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 5; j++) {
                char &c = tiles[j * 10 + i * 2];
                if (GuiTextBox((Rectangle){i * tile_size + (i + 1) * padding,
                                           j * tile_size + bar_height + padding * (j + 1), tile_size, tile_size / 2},
                               &c, 2, selected[j * 5 + i])) {
                    selected.fill(false);
                    selected[j * 5 + i] = true;
                }
            }
        }
        // find if we click off the tiles
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();
            bool clicked_outside = true;
            for (int i = 0; i < 5; i++) {
                for (int j = 0; j < 5; j++) {
                    if (CheckCollisionPointRec(mouse, (Rectangle){i * tile_size + (i + 1) * padding,
                                                                  j * tile_size + bar_height + padding * (j + 1), tile_size, tile_size})) {
                        clicked_outside = false;
                    }
                }
            }
            if (clicked_outside) {
                selected.fill(false);
            }
        }
        EndDrawing();
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}