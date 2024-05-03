#include "graph.hpp"

#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "raylib/raylib.h"
#include "raylib/rlgl.h"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <vector>

// -----------------------------------------------------------------------
// shader utils
std::string load_shader_src(const std::string &file_name) {
    const std::string version_src = "#version 460 core";
    std::ifstream common_file("shaders/common.glsl");
    std::ifstream shader_file("shaders/" + file_name);

    std::stringstream common_stream, shader_stream;
    common_stream << common_file.rdbuf();
    shader_stream << shader_file.rdbuf();

    std::string common_src = common_stream.str();
    std::string shader_src = shader_stream.str();

    std::string full_src = version_src + "\n" + common_src + "\n" + shader_src;

    return full_src;
}

Shader load_shader(const std::string &vs_file_name, const std::string &fs_file_name) {
    std::string vs, fs;

    vs = load_shader_src(vs_file_name);
    fs = load_shader_src(fs_file_name);
    Shader shader = LoadShaderFromMemory(vs.c_str(), fs.c_str());
    return shader;
}

// -----------------------------------------------------------------------
// video source node
class VideoSourceContext : public NodeContext {
private:
    cv::VideoCapture capture;
    std::thread capture_thread;
    std::atomic<bool> stop;
    std::mutex mutex;
    cv::Mat frame;
    Texture texture;

    static void capture_frames(
        cv::VideoCapture &capture,
        cv::Mat &out_frame,
        std::atomic<bool> &stop,
        std::mutex &mutex
    ) {
        cv::Mat bgr;
        while (!stop) {
            capture >> bgr;
            // TODO: validate properly that frame is not empty

            std::lock_guard<std::mutex> lock(mutex);
            cv::cvtColor(bgr, out_frame, cv::COLOR_BGR2RGB);
        }
    }

public:
    VideoSourceContext()
        : capture(0)
        , stop(false) {
        if (!capture.isOpened()) {
            throw std::runtime_error("Failed to open video capture\n");
        }

        int frame_width = capture.get(cv::CAP_PROP_FRAME_WIDTH);
        int frame_height = capture.get(cv::CAP_PROP_FRAME_HEIGHT);

        texture = {
            .id = rlLoadTexture(
                0, frame_width, frame_height, RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8, 1
            ),
            .width = frame_width,
            .height = frame_height,
            .mipmaps = 1,
            .format = RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8};

        capture_thread = std::thread(
            capture_frames,
            std::ref(capture),
            std::ref(frame),
            std::ref(stop),
            std::ref(mutex)
        );
    }

    ~VideoSourceContext() override {
        stop = true;
        capture_thread.join();
        capture.release();
        UnloadTexture(texture);
    }

    Texture get_texture() {
        std::lock_guard<std::mutex> lock(mutex);
        // TODO: don't need to update texture on each get_texture() call
        // introduce need_update flag and update texture only when capture
        // is provided a new frame
        UpdateTexture(texture, frame.data);
        return texture;
    }

    void update(std::shared_ptr<Node> node) override {
        // TODO: put pins in some kind of map and access them by name,
        // not by index
        node->pins[0]._texture = get_texture();
    }
};

// -----------------------------------------------------------------------
// color correction node
class FrameProcessingContext : public NodeContext {
private:
    Shader shader;

    void set_shader_values(std::vector<Pin> &pins) {
        for (auto &pin : pins) {
            if (pin.kind == PinKind::OUTPUT) continue;
            int loc = GetShaderLocation(shader, pin.name.c_str());

            switch (pin.type) {
                case PinType::INT:
                    SetShaderValue(shader, loc, &pin._int.val, SHADER_UNIFORM_INT);
                    break;
                case PinType::FLOAT:
                    SetShaderValue(shader, loc, &pin._float.val, SHADER_UNIFORM_FLOAT);
                    break;
                case PinType::COLOR:
                    SetShaderValue(shader, loc, &pin._color, SHADER_UNIFORM_VEC3);
                    break;
                case PinType::TEXTURE:
                    SetShaderValueTexture(shader, loc, pin._texture);
                    break;
            }
        }
    }

public:
    RenderTexture render_texture;

    FrameProcessingContext(std::string fs_file_name) {
        shader = load_shader("screen_rect.vert", fs_file_name);
        render_texture.id = 0;
    }

    ~FrameProcessingContext() {
        UnloadShader(shader);
        UnloadRenderTexture(render_texture);
    }

    void draw(std::vector<Pin> &pins) {
        // TODO: put pins in some kind of map and access them by name,
        // not by index
        Texture frame = pins[0]._texture;
        if (render_texture.id == 0 && IsTextureReady(frame)) {
            render_texture = LoadRenderTexture(frame.width, frame.height);
        }

        if (render_texture.id == 0 || !IsTextureReady(frame)) {
            return;
        }

        BeginTextureMode(render_texture);
        BeginShaderMode(shader);
        set_shader_values(pins);
        DrawRectangle(0, 0, 1, 1, BLANK);
        EndShaderMode();
        EndTextureMode();
    }

    void update(std::shared_ptr<Node> node) override {
        draw(node->pins);
        node->pins.back()._texture = render_texture.texture;
    }
};

// -----------------------------------------------------------------------
// graph
int get_next_id() {
    static int id = 1;
    return id++;
}

Pin::Pin() = default;
Pin Pin::create_int(PinKind kind, std::string name, int val, int min, int max) {
    Pin pin;
    pin.type = PinType::INT;
    pin.kind = kind;
    pin.name = name;
    pin._int.val = val;
    pin._int.min = min;
    pin._int.max = max;
    return pin;
}

Pin Pin::create_float(PinKind kind, std::string name, float val, float min, float max) {
    Pin pin;
    pin.type = PinType::FLOAT;
    pin.kind = kind;
    pin.name = name;
    pin._float.val = val;
    pin._float.min = min;
    pin._float.max = max;
    return pin;
}

Pin Pin::create_texture(PinKind kind, std::string name) {
    Pin pin;
    pin.type = PinType::TEXTURE;
    pin._texture.id = 0;
    pin.kind = kind;
    pin.name = name;
    return pin;
}

Pin Pin::create_color(PinKind kind, std::string name, Vector3 val) {
    Pin pin;
    pin.type = PinType::COLOR;
    pin.kind = kind;
    pin.name = name;
    pin._color = val;
    return pin;
}

Node::Node() = default;
Node::Node(std::string name, std::vector<Pin> pins, NodeContext *context)
    : name(name)
    , pins(pins)
    , context(context) {}
Node::~Node() {
    delete (NodeContext *)context;
}

Link::Link() = default;
Link::Link(int start_pin_id, int end_pin_id)
    : start_pin_id(start_pin_id)
    , end_pin_id(end_pin_id) {}

void Graph::update() {
    // TODO: this is incorrect! Nodes must be sorted topologically
    for (auto [name, node] : this->nodes) {
        node->context->update(node);
    }

    for (auto &[_, link] : this->links) {
        Pin *start_pin = this->pins[link.start_pin_id];
        Pin *end_pin = this->pins[link.end_pin_id];
        switch (start_pin->type) {
            case PinType::INT:
                end_pin->_int.val = std::clamp(
                    start_pin->_int.val, end_pin->_int.min, end_pin->_int.max
                );
                break;
            case PinType::FLOAT:
                end_pin->_float.val = std::clamp(
                    start_pin->_float.val, end_pin->_float.min, end_pin->_float.max
                );
                break;
            case PinType::COLOR: end_pin->_color = start_pin->_color; break;
            case PinType::TEXTURE: end_pin->_texture = start_pin->_texture; break;
        }
    }
}

std::shared_ptr<Node> create_video_source_node() {
    auto name = "Video Source";
    auto context = new VideoSourceContext();
    auto pins = {
        Pin::create_texture(PinKind::OUTPUT, "frame"),
    };
    std::shared_ptr<Node> node(new Node(name, pins, context));
    return node;
}

std::shared_ptr<Node> create_color_correction_node() {
    auto name = "Color Correction";
    auto context = new FrameProcessingContext("color_correction.frag");
    auto pins = {
        Pin::create_texture(PinKind::INPUT, "frame"),
        Pin::create_color(PinKind::MANUAL, "white_balance", {1.0, 1.0, 1.0}),
        Pin::create_float(PinKind::MANUAL, "exposure", -1.0, -1.0, 10.0),
        Pin::create_float(PinKind::MANUAL, "temperature", 1.0, 0.0, 2.0),
        Pin::create_float(PinKind::MANUAL, "contrast", 1.0, 0.0, 3.0),
        Pin::create_float(PinKind::MANUAL, "brightness", 0.0, -1.0, 1.0),
        Pin::create_float(PinKind::MANUAL, "saturation", 1.0, 0.0, 5.0),
        Pin::create_float(PinKind::MANUAL, "gamma", 1.0, 0.0, 4.0),
        Pin::create_texture(PinKind::OUTPUT, "frame"),
    };
    std::shared_ptr<Node> node(new Node(name, pins, context));
    return node;
}

std::shared_ptr<Node> create_color_quantization_node() {
    auto name = "Color Quantization";
    auto context = new FrameProcessingContext("color_quantization.frag");
    auto pins = {
        Pin::create_texture(PinKind::INPUT, "frame"),
        Pin::create_int(PinKind::MANUAL, "n_levels", 4, 1, 16),
        Pin::create_int(PinKind::MANUAL, "n_samples", 64, 4, 87),
        Pin::create_int(PinKind::MANUAL, "radius", 16, 1, 32),
        Pin::create_texture(PinKind::OUTPUT, "frame"),
    };
    std::shared_ptr<Node> node(new Node(name, pins, context));
    return node;
}

std::shared_ptr<Node> create_color_outline() {
    auto name = "Color Outline";
    auto context = new FrameProcessingContext("color_outline.frag");
    auto pins = {
        Pin::create_texture(PinKind::INPUT, "frame"),
        Pin::create_color(PinKind::MANUAL, "color", {0.0, 0.0, 0.0}),
        Pin::create_float(PinKind::MANUAL, "threshold", 0.06, 0.0, 0.2),
        Pin::create_int(PinKind::MANUAL, "n_samples", 64, 4, 87),
        Pin::create_int(PinKind::MANUAL, "radius", 16, 1, 32),
        Pin::create_texture(PinKind::OUTPUT, "frame"),
    };
    std::shared_ptr<Node> node(new Node(name, pins, context));
    return node;
}

Graph::Graph() {
    this->node_factory["Video Source"] = create_video_source_node;
    this->node_factory["Color Correction"] = create_color_correction_node;
    this->node_factory["Color Quantization"] = create_color_quantization_node;
    this->node_factory["Color Outline"] = create_color_outline;
}

void Graph::delete_node(int node_id) {
    auto node = this->nodes[node_id];

    for (auto &pin : node->pins) {
        auto link_ids = pin.link_ids;
        for (int link_id : link_ids) {
            this->delete_link(link_id);
        }

        this->pins.erase(pin.id);
    }

    this->nodes.erase(node->id);
}

void Graph::delete_link(int link_id) {
    Link link = this->links[link_id];

    Pin *pin0 = this->pins[link.start_pin_id];
    Pin *pin1 = this->pins[link.end_pin_id];

    pin0->link_ids.erase(link.id);
    pin1->link_ids.erase(link.id);
    this->links.erase(link.id);
}

bool Graph::can_create_link(Link link) {
    auto start_pin = this->pins[link.start_pin_id];
    auto end_pin = this->pins[link.end_pin_id];

    // can connect only OUTPUT to INPUT
    if (start_pin->kind != PinKind::OUTPUT || end_pin->kind != PinKind::INPUT) {
        return false;
    }

    // can connect only the same pin types
    if (start_pin->type != end_pin->type) {
        return false;
    }

    // can connect only pins from different nodes
    if (start_pin->node_id == end_pin->node_id) {
        return false;
    }

    // end pin must not be connected yet
    if (end_pin->link_ids.size() != 0) {
        return false;
    }

    return true;
}

int Graph::create_link(Link link) {
    if (!can_create_link(link)) {
        throw std::runtime_error("Failed to create Link");
    };

    link.id = get_next_id();

    Pin *pin0 = this->pins[link.start_pin_id];
    Pin *pin1 = this->pins[link.end_pin_id];

    pin0->link_ids.insert(link.id);
    pin1->link_ids.insert(link.id);
    this->links[link.id] = link;

    return link.id;
}

int Graph::create_node(std::shared_ptr<Node> node) {
    int id = get_next_id();
    node->id = id;

    for (auto &pin : node->pins) {
        pin.id = get_next_id();
        pin.node_id = id;
        pin.link_ids.clear();
        this->pins[pin.id] = &pin;
    }

    this->nodes[id] = node;
    return id;
}
