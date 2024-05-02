#include "app.hpp"

#include "GLFW/glfw3.h"
#include "graph.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_node_editor.h"
#include "raylib/raylib.h"
#include "raylib/rlgl.h"

App::App() {
    InitWindow(1600, 1100, "Freska");
    SetTargetFPS(60);
    rlDisableBackfaceCulling();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    GLFWwindow *window = (GLFWwindow *)GetWindowHandle();
    glfwGetWindowUserPointer(window);
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460 core");

    ed::Config config;
    config.SettingsFile = "freska.json";
    this->context = ed::CreateEditor(&config);

    this->graph = Graph();
}

App::~App() {
    ed::DestroyEditor(this->context);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    CloseWindow();
}

void App::update_and_draw() {
    BeginDrawing();
    ClearBackground(BLANK);

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
        for (auto [name, node] : graph.node_templates) {
            if (ImGui::MenuItem(name.c_str())) {
                node_id = graph.create_node(node);
                break;
            }
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
    for (auto &[_, node] : graph.nodes) {
        ed::BeginNode(node.id);
        ImGui::TextUnformatted(node.name.c_str());

        // input pins
        ImGui::BeginGroup();
        for (auto &pin : node.pins) {
            if (pin.kind != PinKind::INPUT) continue;
            ed::BeginPin(pin.id, ed::PinKind::Input);
            ImGui::TextUnformatted(pin.name.c_str());
            ed::EndPin();
        }
        ImGui::EndGroup();

        // local pins
        ImGui::SameLine();
        ImGui::BeginGroup();
        for (auto &pin : node.pins) {
            if (pin.kind != PinKind::MANUAL) continue;

            auto name = pin.name.c_str();

            ImGui::PushID(pin.id);
            ImGui::SetNextItemWidth(150.0);
            switch (pin.type) {
                case PinType::FLOAT:
                    ImGui::SliderFloat(
                        name, &pin._float.val, pin._float.min, pin._float.max
                    );
                    break;
                case PinType::INT:
                    ImGui::SliderInt(name, &pin._int.val, pin._int.min, pin._int.max);
                    break;
                case PinType::COLOR:
                    ImGui::ColorPicker3(name, reinterpret_cast<float *>(&pin._color));
                    break;
                case PinType::TEXTURE: ImGui::TextUnformatted(name); break;
            }
            ImGui::PopID();
        }
        ImGui::EndGroup();

        // output pins
        ImGui::SameLine();
        ImGui::BeginGroup();
        for (auto &pin : node.pins) {
            if (pin.kind != PinKind::OUTPUT) continue;
            ed::BeginPin(pin.id, ed::PinKind::Output);
            ImGui::TextUnformatted(pin.name.c_str());
            if (pin.type == PinType::TEXTURE) {
                int id = pin._texture.id;
                float aspect = (float)pin._texture.width / pin._texture.height;
                float width = 200.0;
                float height = width / aspect;
                ImGui::Image((ImTextureID)(long)id, {width, height});
            }
            ed::EndPin();
        }
        ImGui::EndGroup();

        ed::EndNode();
    }

    // ---------------------------------------------------------------
    // draw links
    for (auto &[_, link] : graph.links) {
        ed::Link(link.id, link.start_pin_id, link.end_pin_id);
    }

    // ---------------------------------------------------------------
    // update graph
    graph.update();

    // ---------------------------------------------------------------
    // finalize drawing
    ed::End();
    ed::SetCurrentEditor(nullptr);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    DrawFPS(0, 0);
    EndDrawing();
}
