#include "forest.h"
#include <filesystem>

void to_json(json& j, const ImVec2& v){
    j = json{v.x, v.y};
}

void from_json(const json& j, ImVec2& v){
    std::vector<float> data;
    j.get_to(data);
    v.x = data[0];
    v.y = data[1];
}

using IDRemaper = std::unordered_map<std::size_t, void*>;

// we only need id
void to_json(json& j, const Pin& p){
    j = json{
        {"id", p.ID},
        {"type", p.belt_type},
        {"input", p.is_input},
        {"side", p.side},
        {"index", p.index},
        {"count", p.count},
        {"parent", p.parent->ID},
    };
}

void to_json(json& j, const Node& n){
    std::string bname = "";
    std::string rname = "";

    if (n.descriptor){
        bname = n.descriptor->name;
    }

    if (n.recipe()){
        rname = n.recipe()->recipe_name;
    }

    j = json{
        {"id"      , n.ID},
        {"pos"     , n.Pos},
        {"size"    , n.size()},
        {"building", bname},
        {"recipe"  , rname},
        {"rotation", n.rotation},
        {"sides"   , n.pins}
    };
}

void to_json(json& j, const NodeLink& l){
    j = json{
        {"start", l.start->ID},
        {"end"  , l.end->ID}
    };
}


void to_json(json& j, const Forest& n){
    j = json{
        {"nodes", n.nodes},
        {"links", n.links}
    };
}


void from_json(const json& j, Forest& f, IDRemaper& remap){
    auto b = j.at("building").get<std::string>();
    auto r = j.at("recipe").get<std::string>();

    auto& rsc = Resources::instance();
    int building = rsc.find_building(b);
    int recipe = rsc.find_recipe(building, r);

    // insert the new node
    auto n = f.new_node(
        j.at("pos").get<ImVec2>(),
        building,
        recipe,
        j.at("rotation").get<int>()
    );

    auto old_pins = j.at("sides");
    assertf(old_pins.size() == 4, "There are 4 sides!");

    // add pins to remap
    for(auto i = 0u; i < 4u; ++i){
        auto& new_side = n->pins[i];
        auto& old_side = old_pins[i];

        assertf(new_side.size() == old_side.size(), "Side must match");
        for(auto k = 0u; k < new_side.size(); ++k){
            assertf(old_side[k].is_object(), "should be pin object");

            auto old_id = old_side[k].at("id").get<std::size_t>();
            remap[old_id] = reinterpret_cast<void*>(&new_side[k]);
        }
    }

    // save the old => new id for links
    remap[j.at("id").get<std::size_t>()] = reinterpret_cast<void*>(n);
}

void from_json_link(const json& j, Forest& f, IDRemaper& remap){
    auto start_id = j.at("start").get<std::size_t>();
    auto end_id = j.at("end").get<std::size_t>();

    void* start_pin = remap.at(start_id);
    void* end_pin = remap.at(end_id);

    f.new_link(reinterpret_cast<Pin const*>(start_pin),
               reinterpret_cast<Pin const*>(end_pin));
}

void from_json(const json& j, Forest& n){
    IDRemaper remap_id;
    assertf(j.is_object(), "Expect j to be a forest object");

    auto nodes = j.at("nodes").get<std::vector<json>>();
    for(auto& jnode: nodes){
        assertf(jnode.is_object(), "Should be a node object");
        from_json(jnode, n, remap_id);
    }

    auto links = j.at("links").get<std::vector<json>>();
    for(auto& jlink: links){
        assertf(jlink.is_object(), "Should be a link object");
        from_json_link(jlink, n, remap_id);
    }
}

void Forest::save(std::string const& filename, bool override){
    auto path = puzzle::binary_path() + "/saves/" + filename + ".json";

    if (std::filesystem::exists(path) && !override){
         warn("File exist:{} ", path);
         return;
    }

    std::ofstream save_file(path, std::ios::out | std::ios::binary);

    if (!save_file){
        warn("File was not found:{} ", path);
        return;
    }

    json forest = *this;
    save_file << forest;
}

void Forest::clear(){
    nodes.clear();
    links.clear();
    lookup.clear();
}


void Forest::load(std::string const& filename, bool clear){
    auto path = puzzle::binary_path() + "/saves/" + filename + ".json";
    std::ifstream save_file(path, std::ios::in | std::ios::binary);

    if (!save_file){
        warn("File was not found:{} ", path);
        return;
    }

    if (clear){
        this->clear();
    }

    json forest;
    save_file >> forest;
    from_json(forest, *this);
}


/*

void Forest::forward(Node const* node, Node const* prev, ProductionStats& stats, CircleGuard* guard){
    if (guard->count(node->ID) > 0){
        return;
    } else {
        (*guard)[node->ID] = true;
    }

    auto recipe = node->recipe();
    ProductionBook& node_prod = stats[node->ID];

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
                }
            }
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
            // min = item.second.produced / div
            // max = item.second.produced
            forward(next_node, node, stats, guard);
        }
        return;
    }

    if (recipe){
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
                // Make sure output type match the item type
                if ((out_pin->belt_type == 'C' && item.type == 'S' )|| (out_pin->belt_type == 'P' && item.type == 'L' )){
                    out_prod[item.name].produced = item.speed;
                }
            }

            stats[out_link->ID] = out_prod;

            auto next_node = get_next(out_link, node);

            if (next_node != prev){
                forward(next_node, node, stats, guard);
            }
        }
    }
}

void Forest::backward(Node const* node, Node const* prev, ProductionStats& stats, CircleGuard* guard){
    if (guard->count(node->ID) > 0){
        return;
    } else {
        (*guard)[node->ID] = true;
    }

    auto recipe = node->recipe();
    ProductionBook& node_prod = stats[node->ID];

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
                backward(next_node, node, stats, guard);
            }
        }
        return;
    }

    if (recipe){
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
                    if ((in_pin->belt_type == 'C' && item.type == 'S' )|| (in_pin->belt_type == 'P' && item.type == 'L' )){
                        in_prod[item.name].consumed = item.speed;
                    }
                }
            }

            auto next_node = get_next(in_link, node);
            backward(next_node, node, stats, guard);
        }
    }
}


void Forest::simulate_node(Node* n){
    simulate_node_send_out(n);
    simulate_node_fetch_in(n);
    simulate_node_manufacture(n);
}

void Forest::simulate_node_send_out(Node* n){
    auto recipe = n->recipe();
    if (recipe){
        for (auto& ingredient: recipe->outputs){
            auto& prod = n->production[ingredient.name];
            prod.limit_produced = ingredient.speed;

            //
            int out_link_count = 0;
            for(auto& out_pin: n->output_pins){
                auto out_link = find_link(out_pin);

                if (!out_link)
                    continue;

                if (out_pin->compatible(ingredient)) {
                    // the amount of resources remaining since last tick
                    auto remaining = out_link->production[ingredient.name].produced;
                    auto can_be_send = std::max(ingredient.speed - remaining, 0.f);

                    out_link->production[ingredient.name].produced += can_be_send;
                    out_link->production[ingredient.name].limit_produced = prod.limit_produced;

                    prod.produced -= can_be_send;
                    prod.consumed = can_be_send;
                    out_link_count += 1;
                }
            }

            if (out_link_count == 0) {
                prod.produced = 0;
                prod.consumed = ingredient.speed;
            }
        }
    }

    // Splitter/Merger
    if (n->is_relay()){


        // Pipe splitter
        //} else if (n->input_pins[0]->belt_type == 'P') {

  //      }
    }


    if (n->descriptor->name == "Storage Container"){
        int out_link_count = 0;
        for(auto& out_pin: n->output_pins){
            auto out_link = find_link(out_pin);

            if (!out_link)
                continue;

            out_link_count += 1;
        }

        // empty last node
        if (out_link_count == 0) {
            for(auto& item: n->production){
                item.second.consumed = item.second.produced;
                item.second.produced = 0;
                item.second.received = 0;
            }
        }
    }
}


void Forest::simulate_node_fetch_in(Node* n){
    auto recipe = n->recipe();
    if (recipe){
        for(auto& ingredient: recipe->inputs){
            auto& prod = n->production[ingredient.name];
            prod.limit_consumed = ingredient.speed;

            for(auto& in_pin: n->input_pins){
                auto in_link = find_link(in_pin);
                if (!in_link)
                    continue;

                if (in_pin->compatible(ingredient)) {
                    auto can_be_received = std::max(ingredient.speed - prod.received, 0.f);
                    auto received = std::min(can_be_received, in_link->production[ingredient.name].produced);

                    in_link->production[ingredient.name].produced -= received;
                    prod.received += received;
                    prod.consumed = received;
                }
            }
        }
    }

    // Splitter/Merger
    if (n->is_relay()){
        // Conveyor Belt
        if (n->is_input_conveyor(0)){
            bool is_merger = n->output_pins.size() == 1;

            for(auto& item: n->production){
                item.second.consumed = 0;
                item.second.limit_produced = 0;
            }

            // Gather all the resources we are receiving
            for(auto& in_pin: n->input_pins){
                auto in_link = find_link(in_pin);
                if (!in_link)
                    continue;

                auto& link_prod = in_link->production;

                for(auto& item: link_prod){
                    auto& prod = n->production[item.first];

                    float remaining = prod.received;

                    // Here use belt speed instead
                    auto can_be_received = std::max(780.f - remaining, 0.f);
                    can_be_received = std::min(can_be_received, item.second.produced);

                    item.second.produced -= can_be_received;
                    prod.received += can_be_received;
                    prod.consumed += can_be_received;
                    prod.limit_received += item.second.limit_produced;
                }
            }

            // Split all the resources accross
            std::vector<NodeLink*> links;
            links.reserve(3);

            for(auto& out_pin: n->output_pins){
                auto out_link = find_link(out_pin);
                if (!out_link)
                    continue;

                links.push_back(out_link);
            }

            if (is_merger){
                debug("# Merger {}", n->ID);
            } else {
                debug("# Splitter {}", n->ID);
            }

            ProductionBook available;
            for(auto& item: n->production){
                available[item.first].received = item.second.received / float(links.size());
                item.second.consumed = 0;
            }

            for(auto& link: links){
                for(auto& item:  n->production){
                    auto remaining = link->production[item.first].produced;

                    debug("    {}: {} {}", item.first, available[item.first].received, remaining);
                    auto can_be_send = std::max(available[item.first].received - remaining, 0.f);

                    link->production[item.first].produced += can_be_send;
                    item.second.received -= can_be_send;
                    item.second.consumed += can_be_send;

                    debug("    {} {} {} | {}",
                          item.first, item.second.received, item.second.consumed, link->production[item.first].produced);
                }
            }

        // Pipe splitter
        } else if (n->is_input_pipe(0)) {
            bool is_merger = n->output_pins.size() == 1;

            for(auto& item: n->production){
                item.second.consumed = 0;
                item.second.limit_produced = 0;
            }

            // Gather all the resources we are receiving
            for(auto& in_pin: n->input_pins){
                auto in_link = find_link(in_pin);
                if (!in_link)
                    continue;

                auto& link_prod = in_link->production;

                for(auto& item: link_prod){
                    auto& prod = n->production[item.first];

                    float remaining = prod.received;

                    // Here use belt speed instead
                    auto can_be_received = std::max(300.f - remaining, 0.f);
                    can_be_received = std::min(can_be_received, item.second.produced);

                    item.second.produced -= can_be_received;
                    prod.received += can_be_received;
                    prod.consumed += can_be_received;
                    prod.limit_received += item.second.limit_produced;
                }
            }
            // done

            // Split all the resources accross
            std::vector<NodeLink*> links;
            links.reserve(3);

            for(auto& out_pin: n->output_pins){
                auto out_link = find_link(out_pin);
                if (!out_link)
                    continue;

                links.push_back(out_link);
            }

            if (is_merger){
                debug("# Merger {}", n->ID);
            } else {
                debug("# Splitter {}", n->ID);
            }

            ProductionBook available;
            for(auto& item: n->production){
                available[item.first].received = item.second.received / float(links.size());
                item.second.consumed = 0;
            }

            for(auto& link: links){
                for(auto& item:  n->production){
                    auto remaining = link->production[item.first].produced;

                    debug("    {}: {} {}", item.first, available[item.first].received, remaining);
                    auto can_be_send = std::max(available[item.first].received - remaining, 0.f);

                    link->production[item.first].produced += can_be_send;
                    item.second.received -= can_be_send;
                    item.second.consumed += can_be_send;

                    debug("    {} {} {} | {}",
                          item.first, item.second.received, item.second.consumed, link->production[item.first].produced);
                }
            }
        }
    }
}


void Forest::simulate_node_manufacture(Node* n){
    auto recipe = n->recipe();
    if (recipe){
        float efficiency = 1.f;

        for(auto& ingredient: recipe->inputs){
            auto& prod = n->production[ingredient.name];
            // debug("{} ({} / {})", ingredient.name, prod.consumed, ingredient.speed);
            efficiency = std::min(efficiency, prod.consumed / ingredient.speed);
        }

        for(auto& ingredient: recipe->outputs){
            auto& prod = n->production[ingredient.name];
            // debug("{} ({} / {})", ingredient.name, prod.consumed, ingredient.speed);
            efficiency = std::min(efficiency, prod.consumed / ingredient.speed);
        }

        n->efficiency = efficiency;
        debug("({} {}) => {}", n->ID, n->recipe()->recipe_name, n->efficiency);

        // consume inputs
        for(auto& ingredient: recipe->inputs){
             auto& prod = n->production[ingredient.name];
             prod.received -= efficiency * ingredient.speed;
        }

        // produce outputs
        for(auto& ingredient: recipe->outputs){
            auto& prod = n->production[ingredient.name];
            prod.produced += efficiency * ingredient.speed;
        }
    }
}
*/
