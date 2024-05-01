#include "graph.hpp"

#include <cstdio>
#include <stdexcept>
#include <unordered_map>

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

void update_video_source(Node *node) {
    printf("Updating video source node\n");
}

void update_color_correction(Node *node) {
    printf("Updating color correction node\n");
}

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
