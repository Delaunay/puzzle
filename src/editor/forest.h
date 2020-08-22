#ifndef PUZZLE_EDITOR_FOREST_HEADER
#define PUZZLE_EDITOR_FOREST_HEADER

#include "link.h"
#include "node.h"

struct DoubleEntry{
    float consumed = 0;
    float produced = 0;
    float received = 0;
    float overflow = 0;
    float efficiency = 0;
};

struct Engery{
    float consumed = 0;
    float produced = 0;
    std::unordered_map<Building*, float> stats;
};

using ProductionBook = std::unordered_map<std::string, DoubleEntry>;

struct NodeProduction {

};


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

struct SortedPins{
    std::vector<Pin const*> input_pins;
    std::vector<Pin const*> solid_output;
    std::vector<Pin const*> liquid_output;
};



struct GraphIteratorForward{

};

struct GraphIteratorBackward{

};

inline
Node const* get_next(NodeLink const* link, Node const* p){
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

    // Return the consumption of electricity per buildings and in total
    Engery compute_electricity(){
        Engery stat;

        for(auto& node: iter_nodes()){
            if (node.descriptor){
                if (node.descriptor->energy > 0){
                    stat.produced += node.descriptor->energy;
                } else {
                    stat.consumed += node.descriptor->energy;
                }

                stat.stats[node.descriptor] += node.descriptor->energy;
            }
        }

        return stat;
    }

    // Compute the item consumption/production
    // This is more informative than anything else
    // Craft Time Efficiency is better suited to detect bottlenecks
    //
    //  Iron Rod            + 939
    //                              - 203
    ProductionBook production_statement(){
        ProductionBook prod;

        for(auto& node: iter_nodes()){
            auto recipe = node.recipe();

            if (recipe){
                for(auto& input: recipe->inputs){
                    prod[input.name].consumed += input.speed;
                }

                for(auto& output: recipe->outputs){
                    prod[output.name].produced += output.speed;
                }
            }
        }
        return prod;
    }

    SortedPins get_connected_pins(Node const* node) const {
        // Extract the pin we are interested in
        std::vector<Pin const*> input_pins;
        std::vector<Pin const*> solid_output;
        std::vector<Pin const*> liquid_output;

        for (auto& side: node->pins){
            for(auto& pin: side){
                if (find_link(&pin)) {
                    if (pin.is_input) {
                        input_pins.push_back(&pin);
                    }  else {
                        if (pin.belt_type == 'C'){
                            solid_output.push_back(&pin);
                        } else if (pin.belt_type == 'P'){
                            liquid_output.push_back(&pin);
                        }
                    }
                }
            }
        }

        return SortedPins{
            input_pins,
            solid_output,
            liquid_output};
    }

    // Roots do not have input links
    std::vector<Node*> find_roots_leaves(bool match){
        std::vector<Node*> prod;

        for(auto& node: iter_nodes()){
            bool has_inputs = false;

            for (auto& side: node.pins){
                for(auto& pin: side){
                    if (pin.is_input == match && find_link(&pin)){
                        has_inputs = true;
                    }
                }
            }

            if (!has_inputs){
                prod.push_back(&node);
            }
        }
        return prod;
    }

    // Roots do not have input links
    std::vector<Node*> roots(){
        return find_roots_leaves(true);
    }

    // Leaves do not have output links
    std::vector<Node*> leaves(){
        return find_roots_leaves(false);
    }

    // Save a per entity stats about what kind of item and the qty of item
    // going through it
    using ProductionStats = std::unordered_map<std::size_t, ProductionBook>;


    ProductionStats compute_production(){
        ProductionStats prod_stats = full_forward();
        full_backward(prod_stats);
        return prod_stats;
    }

    ProductionStats full_forward(){
        ProductionStats prod_stats;
        for(auto& node: roots()){
            forward(node, nullptr, prod_stats);
        }
        return prod_stats;
    }

    void full_backward(ProductionStats& prod_stats){
        for(auto& node: leaves()){
            backward(node, nullptr, prod_stats);
        }
    }

    void forward(Node const* node, Node const* prev, ProductionStats& stats){
        auto recipe = node->recipe();
        ProductionBook& node_prod = stats[node->ID];
        float efficiency = 1;

        if (recipe){
            // Resource Node, no inputs, only outputs
            if (recipe->inputs.size() == 0){
                for(auto& out: recipe->outputs){
                    node_prod[out.name].produced = out.speed;
                }
            }
            // start Building, fetch inputs
            else {
                std::vector<NodeLink*> in_links;

                // Compute how much resources we are receving accross lanes
                for(auto& in_pin: node->input_pins){
                    auto in_link = find_link(in_pin);
                    if (!in_link)
                        continue;

                    auto link_prod = stats[in_link->ID];
                    in_links.push_back(in_link);

                    for(auto& in_item: link_prod){
                        auto receiving = link_prod[in_item.first].produced;
                        node_prod[in_item.first].received += receiving;

                        // by default mark all resources we received as not accepted
                        // node_prod[in_item.first].overflow = node_prod[in_item.first].received;
                    }
                }

                 // now that we know the amount of item we are receiving we can compute
                 // efficient and we can backward overflow
                 //for(auto& item: recipe->inputs){
                 //    auto& node_item = node_prod[item.name];
                     // node_item.consumed = std::min(node_item.received, item.speed);
                     // update overflow to reflect our usage
                 //    node_item.overflow = std::max(node_item.received - item.speed, 0.f);
                     // efficiency = std::min(efficiency, node_item.consumed / item.speed);
                 //}
            }
        }
        // Splitter / Merger
        else {
            ProductionBook receiving;

            // Compute how much item we are receiving
            for(auto& in_pin: node->input_pins){
                auto in_link = find_link(in_pin);
                if (!in_link)
                    continue;

                auto link_prod = stats[in_link->ID];
                for(auto& in_item: link_prod){
                    receiving[in_item.first].produced += in_item.second.produced;
                }
            }

            //
            for(auto& item: receiving){
                node_prod[item.first].produced = item.second.produced;
            }

            // Redistribute the items accross all the output links
            std::vector<NodeLink*> out_links;
            for(auto& out_pin: node->output_pins){
                auto out_link = find_link(out_pin);
                if (!out_link)
                    continue;

                out_links.push_back(out_link);
            }

            auto div = float(out_links.size());
            ProductionBook out_prod;
            for (auto& item: node_prod){
                out_prod[item.first].produced = item.second.produced / div;
            }

            for(auto& out_link: out_links){
                stats[out_link->ID] = out_prod;

                auto next_node = get_next(out_link, node);
                // TODO: might need to backtrack here
                forward(next_node, node, stats);
            }
            return;
        }

        if (recipe){
            // we have solid & liquid output
            // TODO
            if (recipe->outputs.size() > 1)
                return;

            // Set node production
            for(auto& recipe: node->recipe()->outputs){
                node_prod[recipe.name].produced = recipe.speed;
            }

            // forward production to outputs
            for(auto& out_pin: node->output_pins){
                auto out_link = find_link(out_pin);
                if (!out_link)
                    continue;

                ProductionBook out_prod;
                for(auto& item: recipe->outputs){
                    out_prod[item.name].produced = item.speed;
                }

                stats[out_link->ID] = out_prod;

                auto next_node = get_next(out_link, node);

                if (next_node != prev){
                    forward(next_node, node, stats);
                }
            }
        }
    }

    void backward(Node const* node, Node const* prev, ProductionStats& stats){
        auto recipe = node->recipe();
        ProductionBook& node_prod = stats[node->ID];
        float efficiency = 1;

        // set it to 0
        for(auto& item: node_prod){
            item.second.consumed = 0;
        }
        // -

        if (recipe){
            std::vector<NodeLink*> out_links;

            // Compute how much resources we are consuming accross lanes
            for(auto& out_pin: node->output_pins){
                auto out_link = find_link(out_pin);
                if (!out_link)
                    continue;

                auto link_prod = stats[out_link->ID];
                out_links.push_back(out_link);

                for(auto& out_item: link_prod){
                    auto consumed = link_prod[out_item.first].consumed;
                    node_prod[out_item.first].consumed += consumed;
                }
            }
        }
        // Splitter / Merger
        else {
            // Compute how much item we are consuming
            for(auto& out_pin: node->output_pins){
                auto out_link = find_link(out_pin);
                if (!out_link)
                    continue;

                auto link_prod = stats[out_link->ID];
                for(auto& out_item: link_prod){
                    node_prod[out_item.first].consumed += out_item.second.consumed;
                }
            }

            // Redistribute the items accross all the input links
            std::vector<NodeLink*> in_links;
            for(auto& int_pin: node->input_pins){
                auto in_link = find_link(int_pin);
                if (!in_link)
                    continue;

                in_links.push_back(in_link);
            }

            auto div = float(in_links.size());
            for(auto& in_link: in_links){
                ProductionBook& in_prod = stats[in_link->ID] ;

                // Consume the % of its contribution
                for(auto& item: in_prod){
                    float total_received = node_prod[item.first].produced;
                    float contributed    = item.second.produced;
                    item.second.consumed = node_prod[item.first].consumed * (contributed / total_received);
                }

                auto next_node = get_next(in_link, node);
                // TODO: might need to backtrack here

                if (next_node != prev){
                    backward(next_node, node, stats);
                }
            }
            return;
        }

        if (recipe){
            // we have solid & liquid output
            // TODO
            if (recipe->outputs.size() > 1)
                return;

            // Set node consumption
            for(auto& recipe: node->recipe()->inputs){
                node_prod[recipe.name].consumed = recipe.speed;
            }

            // Forward consumption to inputs
            for(auto& in_pin: node->input_pins){
                auto in_link = find_link(in_pin);
                if (!in_link)
                    continue;

                ProductionBook& in_prod = stats[in_link->ID];
                for(auto& item: recipe->inputs){
                    if (in_prod.count(item.name) == 1){
                        in_prod[item.name].consumed = item.speed;
                    }
                }

                auto next_node = get_next(in_link, node);
                backward(next_node, node, stats);
            }
        }
    }

    // roots (Raw material) => Leaves (propagate max production)
    // Leaves => roots (propagate overflow)

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
