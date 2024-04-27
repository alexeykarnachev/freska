#pragma once
#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Pin;
class Node;
class Link;

enum class PinType {
    INTEGER,
    FLOAT,
    TEXTURE,
};

enum class PinKind {
    INPUT,
    OUTPUT,
};

class Pin {
public:
    int id;
    PinType type;
    PinKind kind;
    std::string name;
    std::unordered_set<int> link_ids;

    Pin();
    Pin(PinType type, PinKind kind, std::string name);
};

class Node {
public:
    int id;
    std::string name;
    std::vector<Pin> pins;

    Node();
    Node(std::string name, std::vector<Pin> pins);
};

class Link {
public:
    int id;
    int start_pin_id;
    int end_pin_id;

    Link();
    Link(int start_pin_id, int end_pin_id);
};

enum class PinType;
enum class PinKind;

class Graph {
private:
    std::unordered_map<int, Pin> pins;
    std::unordered_map<int, Node> nodes;
    std::unordered_map<int, Link> links;

public:
    Graph();

    const Pin &get_pin(int pin_id);
    const Node &get_node(int node_id);
    const Link &get_link(int link_id);

    void delete_node(int node_id);
    void delete_link(int link_id);

    bool can_create_link(Link link);

    int create_link(Link link);
    int create_node(Node node);

    const std::unordered_map<int, Node> &get_nodes() const;
    const std::unordered_map<int, Link> &get_links() const;
};
