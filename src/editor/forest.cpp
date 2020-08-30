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


// Roots do not have input links
std::vector<Node*> Forest::find_roots_leaves(bool match){
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
std::vector<Node*> Forest::roots(){
    return find_roots_leaves(true);
}

// Leaves do not have output links
std::vector<Node*> Forest::leaves(){
    return find_roots_leaves(false);
}

void Forest::traverse(std::function<void(Node*)> fun){
    // We should only visit a child once
    // children can be shared
    CircleGuard guard;

    // Breadth-first traversal so production gets populated evenly
    std::list<Node*> children;
    for(auto& n: roots())
        children.push_back(n);

    while (children.size() > 0){
        Node* node = *children.begin();
        children.pop_front();

        if (guard.count(node->ID) > 0){
            continue;
        }

        guard[node->ID] = true;
        fun(node);

        for(auto& out_pin: node->output_pins){
            auto link = find_link(out_pin);
            if (!link)
                continue;

            auto next = get_next(link, node);
            children.push_back(next);
        }
    }
}

void Forest::reverse(std::function<void(Node*)> fun){
    // We should only visit a child once
    // children can be shared
    CircleGuard guard;

    std::list<Node*> children;
    for(auto& n: leaves())
        children.push_back(n);

    while (children.size() > 0){
        Node* node = *children.begin();
        children.pop_front();

        if (guard.count(node->ID) > 0){
            continue;
        }

        guard[node->ID] = true;
        fun(node);

        for(auto& out_pin: node->input_pins){
            auto link = find_link(out_pin);
            if (!link)
                continue;

            auto next = get_next(link, node);
            children.push_back(next);
        }
    }
}



