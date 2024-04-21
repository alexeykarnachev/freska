#include "GLFW/glfw3.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_node_editor.h"
#include "raylib/raylib.h"
#include "raylib/rlgl.h"
#include <algorithm>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <tuple>
#include <vector>

namespace ed = ax::NodeEditor;

int get_next_id() {
    static int id = 1;
    return id++;
}

enum class PinType {
    FLOAT,
    TEXTURE,
};

enum class PinKind {
    INPUT,
    OUTPUT,
};

struct Node;
struct Link;

struct Pin {
    ed::PinId id;
    std::string name;
    PinType type;
    PinKind kind;

    Pin(ed::PinId id, std::string name, PinType type, PinKind kind)
        : id(id)
        , name(name)
        , type(type)
        , kind(kind) {}
};

struct Node {
    ed::NodeId id;
    std::string name;

    std::vector<Pin> pins;

    Node(ed::NodeId id, std::string name)
        : id(id)
        , name(name) {}
};

struct Link {
    ed::LinkId id;

    ed::PinId start_pin_id;
    ed::PinId end_pin_id;

    Link(ed::LinkId id, ed::PinId start_pin_id, ed::PinId end_pin_id)
        : id(id)
        , start_pin_id(start_pin_id)
        , end_pin_id(end_pin_id) {}
};

std::vector<Node> NODES;
std::vector<Link> LINKS;

Node *get_node(ed::NodeId node_id) {
    for (auto &node : NODES) {
        if (node.id == node_id) return &node;
    }

    throw std::runtime_error("Failed to get node");
}

std::tuple<Pin *, Node *> get_pin_and_node(ed::PinId pin_id) {
    for (auto &node : NODES) {
        for (auto &pin : node.pins) {
            if (pin.id == pin_id) return {&pin, &node};
        }
    }

    throw std::runtime_error("Failed to get pin and node");
}

Link *get_link(ed::LinkId link_id) {
    for (auto &link : LINKS) {
        if (link.id == link_id) return &link;
    }

    throw std::runtime_error("Failed to get link");
}

Link *try_get_link(ed::PinId pin_id) {
    for (auto &link : LINKS) {
        if (link.start_pin_id == pin_id || link.end_pin_id == pin_id) {
            return &link;
        }
    }

    return nullptr;
}

void delete_link(ed::LinkId link_id) {
    auto deleted_link = get_link(link_id);
    auto idx = std::find_if(LINKS.begin(), LINKS.end(), [deleted_link](auto &link) {
        return deleted_link->id == link.id;
    });
    LINKS.erase(idx);
}

void delete_node(ed::NodeId node_id) {
    auto deleted_node = get_node(node_id);

    static std::vector<Link> alive_links;
    alive_links.clear();

    for (auto &link : LINKS) {
        Node *node0, *node1;
        std::tie(std::ignore, node0) = get_pin_and_node(link.start_pin_id);
        std::tie(std::ignore, node1) = get_pin_and_node(link.end_pin_id);
        if (deleted_node != node0 && deleted_node != node1) {
            alive_links.push_back(link);
        }
    }

    LINKS.clear();
    LINKS.insert(LINKS.end(), alive_links.begin(), alive_links.end());

    auto idx = std::find_if(NODES.begin(), NODES.end(), [deleted_node](auto &node) {
        return deleted_node->id == node.id;
    });
    NODES.erase(idx);
}

Node *create_video_source_node() {
    NODES.emplace_back(get_next_id(), "Video Source");
    NODES.back().pins.emplace_back(
        Pin(get_next_id(), "Texture", PinType::TEXTURE, PinKind::OUTPUT)
    );
    return &NODES.back();
}

Node *create_color_correction_node() {
    NODES.emplace_back(get_next_id(), "Color Correction");
    NODES.back().pins.emplace_back(
        Pin(get_next_id(), "Texture", PinType::TEXTURE, PinKind::INPUT)
    );
    NODES.back().pins.emplace_back(
        Pin(get_next_id(), "Brightness", PinType::FLOAT, PinKind::INPUT)
    );
    NODES.back().pins.emplace_back(
        Pin(get_next_id(), "Saturation", PinType::FLOAT, PinKind::INPUT)
    );

    NODES.back().pins.emplace_back(
        Pin(get_next_id(), "Texture", PinType::TEXTURE, PinKind::OUTPUT)
    );
    return &NODES.back();
}

int main() {
    InitWindow(1600, 1100, "Freska");
    SetTargetFPS(60);
    rlDisableBackfaceCulling();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    GLFWwindow *window = (GLFWwindow *)GetWindowHandle();
    glfwGetWindowUserPointer(window);
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460 core");
    ImGui::StyleColorsDark();

    ed::Config config;
    config.SettingsFile = "freska.json";
    ax::NodeEditor::EditorContext *context = ed::CreateEditor(&config);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLANK);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ed::SetCurrentEditor(context);
        ed::Begin("Freska", ImVec2(0.0, 0.0f));

        auto mouse_position = ImGui::GetMousePos();

        // ---------------------------------------------------------------
        // handle background context menu
        ed::Suspend();
        if (ed::ShowBackgroundContextMenu()) {
            ImGui::OpenPopup("Create New Node");
        }
        ed::Resume();

        ed::Suspend();
        if (ImGui::BeginPopup("Create New Node")) {

            Node *node = nullptr;
            if (ImGui::MenuItem("Video Source")) {
                node = create_video_source_node();
            }
            if (ImGui::MenuItem("Color Correction")) {
                node = create_color_correction_node();
            }
            if (ImGui::MenuItem("Color Quantization")) {
            }
            if (ImGui::MenuItem("Color Outline")) {
            }
            if (ImGui::MenuItem("Camera Effects")) {
            }

            if (node) {
                ed::SetNodePosition(node->id, mouse_position);
            }

            ImGui::EndPopup();
        }
        ed::Resume();

        // ---------------------------------------------------------------
        // handle node context menu
        ed::Suspend();
        static ed::NodeId context_node_id = 0;
        if (ed::ShowNodeContextMenu(&context_node_id)) {
            ImGui::OpenPopup("Node Context Menu");
        }
        ed::Resume();

        ed::Suspend();
        if (ImGui::BeginPopup("Node Context Menu")) {
            if (ImGui::MenuItem("Delete")) {
                ed::DeleteNode(context_node_id);
            }
            ImGui::EndPopup();
        }
        ed::Resume();

        // ---------------------------------------------------------------
        // handle link context menu
        ed::Suspend();
        static ed::LinkId context_link_id = 0;
        if (ed::ShowLinkContextMenu(&context_link_id)) {
            ImGui::OpenPopup("Link Context Menu");
        }
        ed::Resume();

        ed::Suspend();
        if (ImGui::BeginPopup("Link Context Menu")) {
            if (ImGui::MenuItem("Delete")) {
                ed::DeleteLink(context_link_id);
            }
            ImGui::EndPopup();
        }
        ed::Resume();

        // ---------------------------------------------------------------
        // handle links creation
        if (ed::BeginCreate()) {
            ed::PinId pin0_id = 0, pin1_id = 0;
            if (ed::QueryNewLink(&pin0_id, &pin1_id)) {
                Pin *pin0, *pin1;
                Node *node0, *node1;
                std::tie(pin0, node0) = get_pin_and_node(pin0_id);
                std::tie(pin1, node1) = get_pin_and_node(pin1_id);
                Link *link0 = try_get_link(pin0_id);
                Link *link1 = try_get_link(pin1_id);

                if (pin0->kind == pin1->kind) {
                    ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                } else if (pin0->type != pin1->type) {
                    ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                } else if (node0 == node1) {
                    ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                } else if (link0 || link1) {
                    ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                } else if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f)) {
                    if (pin0->kind == PinKind::INPUT) {
                        std::swap(pin0, pin1);
                        std::swap(pin0_id, pin1_id);
                    }
                    LINKS.emplace_back(get_next_id(), pin0_id, pin1_id);
                }
            }
        }
        ed::EndCreate();

        // ---------------------------------------------------------------
        // handle delete events
        if (ed::BeginDelete()) {
            ed::NodeId deleted_node_id = 0;
            while (ed::QueryDeletedNode(&deleted_node_id)) {
                if (ed::AcceptDeletedItem()) {
                    delete_node(deleted_node_id);
                }
            }

            ed::LinkId deleted_link_id = 0;
            while (ed::QueryDeletedLink(&deleted_link_id)) {
                if (ed::AcceptDeletedItem()) {
                    delete_link(deleted_link_id);
                }
            }
        }
        ed::EndDelete();

        // ---------------------------------------------------------------
        // draw nodes
        for (auto &node : NODES) {
            ed::BeginNode(node.id);
            ImGui::TextUnformatted(node.name.c_str());

            ImGui::BeginGroup();
            for (auto &pin : node.pins) {
                if (pin.kind != PinKind::INPUT) continue;
                ed::BeginPin(pin.id, ed::PinKind::Input);
                ImGui::TextUnformatted(pin.name.c_str());
                ed::EndPin();
            }
            ImGui::EndGroup();

            ImGui::SameLine();
            ImGui::BeginGroup();
            for (auto &pin : node.pins) {
                if (pin.kind != PinKind::OUTPUT) continue;
                ed::BeginPin(pin.id, ed::PinKind::Output);
                ImGui::TextUnformatted(pin.name.c_str());
                ed::EndPin();
            }
            ImGui::EndGroup();

            ed::EndNode();
        }

        // ---------------------------------------------------------------
        // draw links
        for (auto &link : LINKS) {
            ed::Link(link.id, link.start_pin_id, link.end_pin_id);
        }

        // ---------------------------------------------------------------
        // finalize drawing
        ed::End();
        ed::SetCurrentEditor(nullptr);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        EndDrawing();
    }

    ed::DestroyEditor(context);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    CloseWindow();
}
