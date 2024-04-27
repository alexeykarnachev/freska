#include "editor.hpp"
#include "graph.hpp"
#include "raylib/raylib.h"
#include "raylib/rlgl.h"

int main() {
    InitWindow(1600, 1100, "Freska");
    SetTargetFPS(60);
    rlDisableBackfaceCulling();

    Graph graph;
    Editor editor;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLANK);

        editor.update_and_draw(graph);

        EndDrawing();
    }

    CloseWindow();
}
