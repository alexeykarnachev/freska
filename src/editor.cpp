#include "editor.hpp"

#include "GLFW/glfw3.h"
#include "graph.hpp"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_node_editor.h"
#include "raylib/raylib.h"
#include <stdexcept>

namespace ed = ax::NodeEditor;

Editor::Editor() {
    GLFWwindow *window = (GLFWwindow *)GetWindowHandle();
    if (!window) {
        throw std::runtime_error("Failed to create the Editor, GLFW window is not "
                                 "initialized. Did you call InitWindow beforehand?");
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    glfwGetWindowUserPointer(window);
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460 core");
    ImGui::StyleColorsDark();

    ed::Config config;
    config.SettingsFile = "freska.json";
    this->context = ed::CreateEditor(&config);
}

Editor::~Editor() {
    ed::DestroyEditor(this->context);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Editor::update_and_draw(Graph &graph) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ed::SetCurrentEditor(this->context);
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

        int node_id = -1;
        if (ImGui::MenuItem("Video Source")) {
            node_id = graph.create_node(Node(
                "Video Source",
                {
                    Pin(PinType::FLOAT, PinKind::INPUT, "Float"),
                    Pin(PinType::TEXTURE, PinKind::INPUT, "Texture"),
                    Pin(PinType::TEXTURE, PinKind::OUTPUT, "Texture"),
                }
            ));
        }
        if (ImGui::MenuItem("Color Correction")) {
            node_id = graph.create_node(Node(
                "Color Correction",
                {
                    Pin(PinType::FLOAT, PinKind::INPUT, "Float"),
                    Pin(PinType::INTEGER, PinKind::INPUT, "Integer"),
                    Pin(PinType::TEXTURE, PinKind::OUTPUT, "Texture"),
                }
            ));
        }
        if (ImGui::MenuItem("Color Quantization")) {
        }
        if (ImGui::MenuItem("Color Outline")) {
        }
        if (ImGui::MenuItem("Camera Effects")) {
        }

        if (node_id != -1) {
            ed::SetNodePosition(node_id, mouse_position);
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
        ed::PinId start_pin_id = 0, end_pin_id = 0;
        if (ed::QueryNewLink(&start_pin_id, &end_pin_id)) {
            Link link(start_pin_id.Get(), end_pin_id.Get());
            bool can_create = graph.can_create_link(link);
            if (!can_create) {
                ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
            } else if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f)) {
                graph.create_link(link);
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
                graph.delete_node(deleted_node_id.Get());
            }
        }

        ed::LinkId deleted_link_id = 0;
        while (ed::QueryDeletedLink(&deleted_link_id)) {
            if (ed::AcceptDeletedItem()) {
                graph.delete_link(deleted_link_id.Get());
            }
        }
    }
    ed::EndDelete();

    // ---------------------------------------------------------------
    // draw nodes
    for (auto &[_, node] : graph.get_nodes()) {
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
    for (auto &[_, link] : graph.get_links()) {
        ed::Link(link.id, link.start_pin_id, link.end_pin_id);
    }

    // ---------------------------------------------------------------
    // finalize drawing
    ed::End();
    ed::SetCurrentEditor(nullptr);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
