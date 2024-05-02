#pragma once
#include "raylib/raylib.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Pin;
class Node;
class Link;

enum class PinType {
    INT,
    FLOAT,
    COLOR,
    TEXTURE,
};

enum class PinKind {
    INPUT,
    OUTPUT,
    MANUAL,
};

class Pin {
public:
    int id;
    int node_id;
    PinType type;
    PinKind kind;
    std::string name;
    std::unordered_set<int> link_ids;
    union {
        struct {
            int val;
            int min;
            int max;
        } _int;

        struct {
            float val;
            float min;
            float max;
        } _float;

        Vector3 _color;
        Texture _texture;
    };

    Pin();
    static Pin create_int(PinKind kind, std::string name, int val, int min, int max);
    static Pin create_float(
        PinKind kind, std::string name, float val, float min, float max
    );
    static Pin create_color(PinKind kind, std::string name, Vector3 val);
    static Pin create_texture(PinKind kind, std::string name);
};

class Node {
public:
    int id;
    std::string name;
    std::vector<Pin> pins;

    void *context = nullptr;
    void (*update)(Node *) = nullptr;

    Node();
    Node(std::string name, std::vector<Pin> pins, void (*update)(Node *));
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
public:
    std::unordered_map<int, Pin *> pins;
    std::unordered_map<int, Node> nodes;
    std::unordered_map<int, Link> links;
    std::unordered_map<std::string, Node> node_templates;

    Graph();

    void delete_node(int node_id);
    void delete_link(int link_id);

    bool can_create_link(Link link);

    int create_link(Link link);
    int create_node(Node node);

    void update();
};
