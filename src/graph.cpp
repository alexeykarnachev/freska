#include "graph.hpp"

#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "raylib/raylib.h"
#include "raylib/rlgl.h"
#include <atomic>
#include <cstdio>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>

// -----------------------------------------------------------------------
// node context
class NodeContext {
public:
    virtual ~NodeContext() {}
};

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
};

void update_video_source(Node *node) {
    if (!node->context) {
        node->context = new VideoSourceContext();
    }
    auto context = (VideoSourceContext *)node->context;

    // TODO: put pins in some kind of map and access them by name,
    // not by index
    node->pins[0].value.texture_value = context->get_texture();
}

// -----------------------------------------------------------------------
// color correction node
void update_color_correction(Node *node) {
    printf("Updating color correction node\n");
}

// -----------------------------------------------------------------------
// graph
int get_next_id() {
    static int id = 1;
    return id++;
}

Pin::Pin() = default;
Pin::Pin(PinType type, PinKind kind, std::string name)
    : type(type)
    , kind(kind)
    , name(name) {}

Node::Node() = default;
Node::Node(std::string name, std::vector<Pin> pins, void (*update)(Node *))
    : name(name)
    , pins(pins)
    , update(update) {}

Link::Link() = default;
Link::Link(int start_pin_id, int end_pin_id)
    : start_pin_id(start_pin_id)
    , end_pin_id(end_pin_id) {}

void Graph::update() {
    // TODO: this is incorrect! Nodes must be sorted topologically
    for (auto &[name, node] : this->nodes) {
        if (node.update) node.update(&node);
    }
}

Graph::Graph() {
    this->node_templates["Video Source"] = Node(
        "Video Source",
        {
            Pin(PinType::TEXTURE, PinKind::OUTPUT, "texture"),
        },
        update_video_source
    );

    this->node_templates["Color Correction"] = Node(
        "Color Correction",
        {
            Pin(PinType::TEXTURE, PinKind::OUTPUT, "texture"),
            Pin(PinType::FLOAT, PinKind::INPUT, "brightness"),
            Pin(PinType::FLOAT, PinKind::INPUT, "saturation"),
            Pin(PinType::FLOAT, PinKind::INPUT, "contrast"),
            Pin(PinType::FLOAT, PinKind::INPUT, "temperature"),
            Pin(PinType::TEXTURE, PinKind::INPUT, "texture"),
        },
        update_color_correction
    );
}

const Pin &Graph::get_pin(int pin_id) {
    return this->pins[pin_id];
}

const Node &Graph::get_node(int node_id) {
    return this->nodes[node_id];
}

const Link &Graph::get_link(int link_id) {
    return this->links[link_id];
}

void Graph::delete_node(int node_id) {
    auto &node = this->get_node(node_id);

    for (auto &pin : node.pins) {
        for (int link_id : pin.link_ids) {
            this->delete_link(link_id);
        }

        this->pins.erase(pin.id);
    }

    if (node.context) {
        delete (NodeContext *)node.context;
    }

    this->nodes.erase(node.id);
}

void Graph::delete_link(int link_id) {
    Link link = this->links[link_id];

    Pin &pin0 = this->pins[link.start_pin_id];
    Pin &pin1 = this->pins[link.end_pin_id];

    pin0.link_ids.erase(link.id);
    pin1.link_ids.erase(link.id);
    this->links.erase(link.id);
}

bool Graph::can_create_link(Link link) {
    auto start_pin = this->pins[link.start_pin_id];
    auto end_pin = this->pins[link.end_pin_id];

    // can connect only OUTPUT to INPUT
    if (start_pin.kind != PinKind::OUTPUT || end_pin.kind != PinKind::INPUT) {
        return false;
    }

    // can connect only the same pin types
    if (start_pin.type != end_pin.type) {
        return false;
    }

    // can connect only pins from different nodes
    if (start_pin.node_id == end_pin.node_id) {
        return false;
    }

    // end pin must not be connected yet
    if (end_pin.link_ids.size() != 0) {
        return false;
    }

    return true;
}

int Graph::create_link(Link link) {
    if (!can_create_link(link)) {
        throw std::runtime_error("Failed to create Link");
    };

    link.id = get_next_id();

    Pin &pin0 = this->pins[link.start_pin_id];
    Pin &pin1 = this->pins[link.end_pin_id];

    pin0.link_ids.insert(link.id);
    pin1.link_ids.insert(link.id);
    this->links[link.id] = link;

    return link.id;
}

int Graph::create_node(Node node) {
    node.id = get_next_id();
    for (auto &pin : node.pins) {
        pin.id = get_next_id();
        pin.node_id = node.id;
        pin.link_ids.clear();
        this->pins[pin.id] = pin;
    }

    this->nodes[node.id] = node;
    return node.id;
}

const std::unordered_map<int, Node> &Graph::get_nodes() const {
    return this->nodes;
}

const std::unordered_map<int, Link> &Graph::get_links() const {
    return this->links;
}

const std::unordered_map<std::string, Node> &Graph::get_node_templates() const {
    return this->node_templates;
}
