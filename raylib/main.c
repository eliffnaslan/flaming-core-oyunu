#include "raylib.h"

int main() {
    InitWindow(400, 300, "Raylib Test");
    
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Raylib çalışıyor!", 100, 150, 20, DARKGRAY);
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}