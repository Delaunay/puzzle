#ifndef PUZZLE_EDITOR_FOREST_HEADER
#define PUZZLE_EDITOR_FOREST_HEADER

#include "link.h"
#include "node.h"

struct DoubleEntry{
    float consumed = 0;
    float produced = 0;
    float received = 0;
};

struct Engery{
    std::unordered_map<Building*, float> stats;
    float consumed = 0;
    float produced = 0;
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

// Data structure to manage and query the graph drawn on the screen
struct Forest{
public:
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
        // Memoization table
        ProductionStats stats;
        _compute_production(stats);
        return stats;
    }

    // Compute the
    void _compute_production(ProductionStats& stats) const {
        for(auto& node: iter_nodes()){
            _compute_node_production(&node, stats);
        }
    }

    void _compute_node_production(Node const* node, ProductionStats& stats) const {

        if (node == nullptr) {
            debug("nullptr was passed!");
            return;
        }

        if (stats.count(node->ID) > 0) {
            // we are recursively computing node prod
            // so some nodes might get computed before we reach them
            return;
        }

        debug("Compute production for {}:{}", node->ID, node->descriptor->name);

        // TODO: add logic for splitter/merger
        // std::unordered_map<std::string, DoubleEntry>;
        ProductionBook node_prod;

        // Add the ingredients we need
        auto recipe = node->recipe();
        if (recipe){

            for (auto& in: recipe->inputs){
                DoubleEntry v;
                v.consumed = in.speed;
                node_prod.insert(std::make_pair(in.name, v));
            }
        }

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

        // Check input pins first
        // we need to know if the node require input is satified to
        // known how much we output
        for(auto pin: input_pins){
            auto link = find_link(pin);
            if (!link)
                continue;

            // Find out which item this belt in bringing
            ProductionBook* link_prod = get(stats, link->ID);

            // Input link does not have the production info
            // recusively compute the parents
            if (!link_prod){
                _compute_node_production(link->start->parent, stats);
            }

            link_prod = get(stats, link->ID);

            // result was cached!
            if (link_prod){
                // for each item this link provide consume as much as we can
                for(auto& link_item: (*link_prod)){
                    // is the item a requirement for us?
                    DoubleEntry* node_item = get(node_prod, link_item.first);

                    if (node_item){
                        auto consumed = std::min(node_item->consumed, link_item.second.produced);
                        node_item->received = consumed;
                        link_item.second.consumed = consumed;
                    }
                }
                // --
            } else {
                debug("Link was not computed! {}", link->ID);
            }
        }

        // Compute craft-time efficiency
        float efficiency = 1;
        for (auto& item: node_prod){
            if (item.second.consumed > 0){
                efficiency = std::min(efficiency, item.second.received / item.second.consumed);
            }
        }

        // Populate output connection with production info
        if (recipe){
            // For each output set the production
            for (auto& out: recipe->outputs){
                std::vector<Pin const*> out_pin;

                if (out.type == 'S') {
                    out_pin = solid_output;
                } else if (out.type == 'L') {
                    out_pin = liquid_output;
                }

                if (out_pin.size() > 0) {
                    float total_production = out.speed * efficiency;
                    // this is incorrect a connection could be consuming less
                    // but we need to go down stream and up again to know that

                    // to compute this correctly we need:
                    //    - put the partial total in each
                    //    - traverse down each and see if anyone is not using it in to its fullest
                    //    - move unused quantities to the other branches
                    //
                    // Obviously this would cost a lot of resources to compute
                    // might mitigate the cost if we could compute that in reverse
                    // leaves send up what they need and from that we know if the max will be reached or not
                    float prod_by_link = total_production / out_pin.size();

                    for (auto& pin: out_pin){
                        auto link = find_link(pin);

                        DoubleEntry out_prod;
                        out_prod.produced = prod_by_link;

                        ProductionBook link_prod;
                        link_prod[out.name] = out_prod;

                        stats[link->ID] = link_prod;
                    }
                }
            }
        }

        stats[node->ID] = node_prod;
    }

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
