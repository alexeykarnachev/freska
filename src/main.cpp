#include "app.hpp"
#include "raylib/raylib.h"
#include "raylib/rlgl.h"

int main() {

    App editor;

    while (!WindowShouldClose()) {
        editor.update_and_draw();
    }
}
