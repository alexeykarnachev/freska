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
Node::Node(std::string name, std::vector<Pin> pins)
    : name(name)
    , pins(pins) {}

Link::Link() = default;
Link::Link(int start_pin_id, int end_pin_id)
    : start_pin_id(start_pin_id)
    , end_pin_id(end_pin_id) {}

Graph::Graph() = default;

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
