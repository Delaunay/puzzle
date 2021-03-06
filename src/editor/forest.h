#ifndef PUZZLE_EDITOR_FOREST_HEADER
#define PUZZLE_EDITOR_FOREST_HEADER

#include "link.h"
#include "node.h"

using CircleGuard = std::unordered_map<std::size_t, bool>;


// Safe Get
template<typename K, typename V>
V* get(std::unordered_map<K, V>& map, K const& k){
    if (map.count(k) == 0){
        return nullptr;
    }
    return &map[k];

//    try {
//        return &map.at(k);
//    } catch (std::out_of_range&) {
//        return nullptr;
//    }
}

inline
Node* get_next(NodeLink const* link, Node const* p){
    if (link->start->parent == p)
        return link->end->parent;
    return link->start->parent;
}


inline
Node* get_next(NodeLink* link, Node* p){
    if (link->start->parent == p)
        return link->end->parent;
    return link->start->parent;
}

// Data structure to manage and query the graph drawn on the screen
struct Forest{
public:
    friend void to_json(json& j, const Forest& n);
    friend void from_json(const json& j, Forest& n);

    // check if a pin is connected only once
    // if not remove it and make the new connection
    void remove_pin_link(Pin const* p){
        auto iter = lookup.find(p->ID);
        if (iter != lookup.end()){
            remove_link(iter->second);
        }
    }

    // Add a new connection between two pins
    // checks that none of the pins are already connected to other pin
    // if so remove the previous connection
    NodeLink* new_link(Pin const* s, Pin const* e){
        remove_pin_link(e);
        remove_pin_link(s);

        links.emplace_back(s, e);
        auto* link = &(*links.rbegin());
        lookup[s->ID] = link;
        lookup[e->ID] = link;

        link->logic = fetch_logic(this, link);
        return link;
    }

    void remove_link(NodeLink* link){
        debug("Removing link");
        if (link == nullptr){
            return;
        }

        debug("{}", link->start->ID);
        debug("{}", link->end->ID);

        lookup.erase(link->start->ID);
        lookup.erase(link->end->ID);
        links.remove(*link);
    }

    Node* new_node(ImVec2 pos, int building, int recipe, int rotation = 0){
        // Node(building, pos, recipe, rotation);

        Node& inserted_node = nodes.emplace_back(building, pos, recipe, rotation);
        inserted_node.logic = fetch_logic(this, &inserted_node);
        return &inserted_node;
    }

    void remove_node(Node* node){
        // remove all pins
        for(auto& side: node->pins){
            for(auto& pin: side){
                remove_pin_link(&pin);
            }
        }
        // remove node from the vector
        nodes.remove(*node);
    }

    // checks that the Node pointer is valid
    // should only be used in debug mode
    bool is_valid(Node* n) const {
        for(auto i = nodes.begin(); i != nodes.end(); ++i){
            if (n == (&*i))
                return true;
        }
        return false;
    }

    NodeLink* find_link(Pin const* pin) const {
        auto result = lookup.find(pin->ID);
        if (result == lookup.end())
            return nullptr;
        return result->second;
    }

    int node_count() const {
        return int(nodes.size());
    }

    int link_count() const {
        return int(links.size());
    }

    using NodeIterator = std::list<Node>::iterator;
    using LinkIterator = std::list<NodeLink>::iterator;

    using NodeConstIterator = std::list<Node>::const_iterator;
    using LinkConstIterator = std::list<NodeLink>::const_iterator;

    Iterator<NodeIterator>      iter_nodes()       { return Iterator(nodes.begin(), nodes.end()); }
    Iterator<LinkIterator>      iter_links()       { return Iterator(links.begin(), links.end()); }
    Iterator<NodeConstIterator> iter_nodes() const { return Iterator(nodes.begin(), nodes.end()); }
    Iterator<LinkConstIterator> iter_links() const { return Iterator(links.begin(), links.end()); }

    // Roots do not have input links
    std::vector<Node*> find_roots_leaves(bool match);

    // Roots do not have input links
    std::vector<Node*> roots();

    // Leaves do not have output links
    std::vector<Node*> leaves();

    void traverse(std::function<void(Node*)> fun);

    void reverse(std::function<void(Node*)> fun);

    void save(std::string const& filename, bool override=false);

    void load(std::string const& filename, bool clear=false);

    void clear();

private:
    // We are using list because we want to use pointer as optional reference to a Node/NodeLink
    // vector would invalidate all the pointers on reallocation
    // to make prevent fragmentation a custom allocator could be provided if it becomes
    // an issue
    std::list<Node>     nodes;
    std::list<NodeLink> links;
    // Pin to Link lookup
    std::unordered_map<std::size_t, NodeLink*> lookup;
};


#endif
