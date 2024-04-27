#pragma once
#include "graph.hpp"
#include "imgui/imgui_node_editor.h"

namespace ed = ax::NodeEditor;

class Editor {
private:
    ed::EditorContext *context;

public:
    Editor();
    ~Editor();

    void update_and_draw(Graph &graph);
};
