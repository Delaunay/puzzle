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

//    void backward(Node* const node, Node* const prev, ProductionStats& stats){
//        auto recipe = node->recipe();
//        ProductionBook& node_prod = stats[node->ID];
//        float efficiency = 1;

//        // Production Nodes
//        if (recipe){
//            // pass down all the consumed resources to the top
//            for(auto& out_pin: node->output_pins){
//                auto out_link = find_link(out_pin);
//                if (!out_link)
//                    continue;

//                auto link_prod = stats[out_link->ID];
//                for(auto& out_prod: link_prod){
//                    node_prod[out_prod.first].consumed = out_prod.second.consumed;
//                }
//            }

////            for(auto& item: recipe->outputs){
////                auto& node_item = node_prod[item.name];
////                efficiency = std::max(efficiency, 1.f - node_item.overflow / node_item.produced);
////            }

////            for(auto* in_pin: node->input_pins){
////                auto in_link = find_link(in_pin);
////                if (!in_link)
////                    continue;

////                auto link_prod = stats[in_link->ID];
////                for(auto& item: link_prod){
////                    item.second.overflow = node_prod
////                }
////            }
//        }
//    }

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
                        node_prod[in_item.first].overflow = node_prod[in_item.first].received;
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

            // Compute how much item we are receiving
            for(auto& in_pin: node->input_pins){
                auto in_link = find_link(in_pin);
                if (!in_link)
                    continue;

                auto link_prod = stats[in_link->ID];
                for(auto& in_item: link_prod){
                    node_prod[in_item.first].produced += in_item.second.produced;
                }
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

            for(auto& recipe: node->recipe()->outputs){
                node_prod[recipe.name].produced = recipe.speed;
            }

            // One output
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
                forward(next_node, node, stats);
            }
        }
    }

    void backward(Node const* node, Node const* prev, ProductionStats& stats){
        auto recipe = node->recipe();
        ProductionBook& node_prod = stats[node->ID];
        float efficiency = 1;

        if (recipe){

            std::vector<NodeLink*> out_links;
            // Compute how much resources we are consumed accross lanes
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
                    float total_received = node_prod[item.first].received;
                    float contributed    = item.second.produced;
                    item.second.consumed = node_prod[item.first].consumed * (contributed / total_received);
                }

                auto next_node = get_next(in_link, node);
                // TODO: might need to backtrack here
                backward(next_node, node, stats);
            }
            return;
        }

        if (recipe){
            // we have solid & liquid output
            // TODO
            if (recipe->outputs.size() > 1)
                return;

            for(auto& in_pin: node->input_pins){
                auto in_link = find_link(in_pin);
                if (!in_link)
                    continue;

                ProductionBook& in_prod = stats[in_link->ID];
                for(auto& item: recipe->inputs){
                    in_prod[item.name].consumed = item.speed;
                }

                auto next_node = get_next(in_link, node);
                backward(next_node, node, stats);
            }
        }
    }

    // roots (Raw material) => Leaves (propagate max production)
    // Leaves => roots (propagate overflow)

    void propagate_overflow(NodeLink* link, Node const* prev, ProductionStats& stats) const {
        if (!link)
            return;

        Node* next = nullptr;

        if (link->end->parent == prev)
            next = link->start->parent;

        if (link->start->parent == prev)
            next = link->end->parent;

        auto& link_prod = stats[link->ID];
        auto& next_prod = stats[next->ID];

        for(auto& item: link_prod){
            next_prod[item.first].overflow = item.second.overflow;
        }

        propagate_overflow(next, link, stats);
    }

    // leaves to roots
    void propagate_overflow(Node const* node, NodeLink const* prev, ProductionStats& stats) const {
        auto sorted_pins = get_connected_pins(node);

        // Compute craft-time efficiency
        auto& node_prod = stats[node->ID];

        // check all the item we are producing
        // if some are backed up it reduces efficiency
        float efficiency = 1;
        for (auto& item: node_prod){
            if (item.second.produced > 0 && item.second.overflow > 0){
                efficiency = std::min(efficiency, item.second.overflow / item.second.produced);
            }
        }

        // produced - consumed + overflow

        // reduce our consumption of goods
        for (auto& item: node_prod){
            // production node
            if (item.second.consumed > 0){
                item.second.overflow = item.second.consumed - item.second.consumed * efficiency;
            }
            // resource node
            else if (item.second.produced > 0) {
                debug("{}", efficiency);
            }
        }

        // Propagte the change to the links
        for(auto& inpin: sorted_pins.input_pins){
            auto link = find_link(inpin);
            if (!link)
                continue;

            auto link_prod = stats[link->ID];
            for(auto& item: link_prod){
                link_prod[item.first].overflow = node_prod[item.first].overflow;
            }

            propagate_overflow(link, node, stats);
        }
    }

    // Roots => leaves
    ProductionBook _compute_node_production(Node const* node, ProductionStats& stats) const {
        // TODO: add logic for splitter/merger
        // std::unordered_map<std::string, DoubleEntry>;


        if (node == nullptr) {
            debug("nullptr was passed!");
            return ProductionBook();
        }

        if (stats.count(node->ID) > 0) {
            // we are recursively computing node prod
            // so some nodes might get computed before we reach them
            return ProductionBook();
        }

        debug("Compute production for {}:{}", node->ID, node->descriptor->name);

        // register it now to avoid infinite recursion
        ProductionBook& node_prod = stats[node->ID];

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
        std::vector<Pin const*> solid_output;
        std::vector<Pin const*> liquid_output;

        for (auto& side: node->pins){
            for(auto& pin: side){
                if (find_link(&pin)) {
                    if (!pin.is_input) {
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
        for(auto pin: node->input_pins){
            auto link = find_link(pin);
            if (!link)
                continue;

            // Find out which item this belt in bringing
            ProductionBook* link_prod = get(stats, link->ID);

            // Input link does not have the production info
            // recusively compute the parents
            if (!link_prod){
                // Only conveyor belts have start as guaranteed parent
                // Pipe could have end as well
                Node const* parent = link->start->parent;
                if (parent == node){
                    parent = link->end->parent;
                }
                _compute_node_production(parent, stats);
            }

            link_prod = get(stats, link->ID);

            // result was cached!
            if (link_prod){
                // for each item this link provide consume as much as we can
                for(auto& link_item: (*link_prod)){
                    // is the item a requirement for us?
                    DoubleEntry* node_item = get(node_prod, link_item.first);
                    float consumed = 0;

                    if (node_item){
                        consumed = std::min(node_item->consumed, link_item.second.produced);
                        node_item->received = consumed;
                    }

                    // If the item is not needed it fully overflows
                    link_item.second.consumed = consumed;

                    // This is the first overflow
                    // we are receiving more items that we need
                    link_item.second.overflow = std::max(link_item.second.produced - consumed, 0.f);
                    propagate_overflow(link, node, stats);
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

                float total_production = out.speed * efficiency;

                // Set the production info the the node itself as well
                // Building can consume & produce the same good!
                DoubleEntry& node_prod_entry = node_prod[out.name];
                node_prod_entry.produced = total_production;

                // set production to the correct pin
                if (out.type == 'S') {
                    out_pin = solid_output;
                } else if (out.type == 'L') {
                    out_pin = liquid_output;
                } else {
                    debug("{} output not known", out.type);
                }

                if (out_pin.size() > 0) {
                    // this is incorrect a connection could be consuming less
                    // but we need to go downstream and up again to know that

                    // to compute this correctly we need:
                    //    - put the partial total in each
                    //    - traverse down each and see if anyone is not using it in to its fullest
                    //    - move unused quantities to the other branches
                    //
                    // Obviously this would cost a lot of resources to compute
                    // might mitigate the cost if we could compute that in reverse
                    // leaves send up what they need and from that we know if the max will be reached or not
                    float prod_by_link = total_production / out_pin.size();


                    // This is the second possible overflow
                    // We receive enough but it is not consumed
                    // For now it is ignored
                    for (auto& pin: out_pin){
                        auto link = find_link(pin);

                        DoubleEntry out_prod;
                        out_prod.produced = prod_by_link;

                        ProductionBook link_prod;
                        link_prod[out.name] = out_prod;

                        stats[link->ID] = link_prod;
                    }
                } else {
                    debug("No valid output pin found for {}", node->ID);
                }
            }
        } else {
            // Merger/Splitter logic
            // Find out how much items are passing through this
            for(auto& inpin: node->input_pins){
                auto link = find_link(inpin);
                if (!link)
                    continue;

                ProductionBook* link_prod = get(stats, link->ID);
                for(auto& item_received: *link_prod){
                    node_prod[item_received.first].received += item_received.second.produced;
                }
            }

            std::vector<Pin const*> out_pin;

            // set production to the correct pin
            if (solid_output.size() > 0) {
                out_pin = solid_output;
            } else {
                out_pin = liquid_output;
            }

            for(auto& outpin: out_pin){
                auto link = find_link(outpin);
                if (!link)
                    continue;

                ProductionBook link_prod;

                for(auto& produced_item: node_prod){
                    if (produced_item.second.received > 0){
                        DoubleEntry out_prod;
                        out_prod.produced = produced_item.second.received / float(out_pin.size());
                        link_prod[produced_item.first] = out_prod;
                    }
                }

                stats[link->ID] = link_prod;
            }
        }

        return node_prod;
    }

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
