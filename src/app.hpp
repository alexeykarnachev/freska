#pragma once
#include "graph.hpp"
#include "imgui/imgui_node_editor.h"

namespace ed = ax::NodeEditor;

class App {
private:
    ed::EditorContext *context;
    Graph graph;

public:
    App();
    ~App();

    void update_and_draw();
};
