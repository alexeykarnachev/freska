#include "app.hpp"
#include "raylib/raylib.h"
#include "raylib/rlgl.h"

int main() {

    App app;

    while (!WindowShouldClose()) {
        app.update_and_draw();
    }
}
