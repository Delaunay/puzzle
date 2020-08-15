#ifndef PUZZLE_CONFIG_HEADER
#define PUZZLE_CONFIG_HEADER

#include "application/application.h"
#include "application/sys.h"


#include <fstream>
#include <algorithm>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

struct Item {
    std::string name;   // Item Name
    float       qty;    // qty
    float       speed;  // qty/min
};

struct Recipe {
    std::string       recipe_name;
    float             crafting_time;
    std::vector<Item> inputs;
    std::vector<Item> outputs;
    std::string       texture;
};

struct Building {
    std::string name;
    float       energy;
    float       w;
    float       l;
    float       h;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
    std::vector<Recipe>      recipes;

    std::vector<const char*> _cached_names;

    std::vector<const char*> const& recipe_names(){
        if (_cached_names.size() == 0){
            _cached_names.reserve(recipes.size());

            for (auto& recipe: recipes){
                _cached_names.push_back(recipe.recipe_name.c_str());
            }
        }
        return _cached_names;
    }
};


void from_json(const json& j, Item& p);

void from_json(const json& j, Recipe& p);

void from_json(const json& j, Building& p);

struct Resources {
    static Resources& instance(){
        static Resources rsc;
        return rsc;
    }

    Resources(){}

    Resources(Resources const&) = delete;

    void load_configs(){
        auto path = puzzle::binary_path() + "/resources/buildings.json";
        std::ifstream buildings_file(path, std::ios::in | std::ios::binary);

        if (!buildings_file){
            RAISE(std::runtime_error, "File was not found: " + path);
        }

        json obj;
        buildings_file >> obj;
        obj.get_to(buildings);
        debug("Found {} buildings", buildings.size());

        // load the recipes of each buildings
        for(auto& building: buildings){
            load_recipes(building);
        }
    }

    std::vector<const char*> building_names(){
        assert(buildings.size() > 0 && "Should have loaded buildings first");

        std::vector<const char*> names;
        names.reserve(buildings.size());

        for (auto& building: buildings){
            names.push_back(building.name.c_str());
        }
        debug("{}", names.size());
        return names;
    }

    void load_recipes(Building& building){
        std::string name(building.name.size(), ' ');
        std::transform(std::begin(building.name), std::end(building.name), name.begin(), [] (char c) -> char {
            if (c == ' ')
                return '_';

            return std::tolower(c);
        });

        auto path = puzzle::binary_path() + "/resources/" + name + ".json";
        std::ifstream recipes_file(path, std::ios::in | std::ios::binary);

        if (!recipes_file){
            warn("File was not found:{} ", path);
            return;
        }

        json recipes;
        recipes_file >> recipes;
        recipes.get_to(building.recipes);

        debug("Found {} recipes for {}", building.recipes.size(), building.name);
    }

    Image* load_texture(std::string path){
        if (path.size() == 0) {
            return nullptr;
        }

        Image* img = _texture_cache[path].get();
        if (img != nullptr){
            return img;
        }

        debug("load texture {}", path);
        auto absolute_path = puzzle::binary_path() + "/" + path;

        auto p = std::make_shared<Image>(absolute_path);
        _texture_cache[path] = p;
        return p.get();
    }

    std::vector<Building> buildings;
    std::unordered_map<std::string, std::shared_ptr<Image>> _texture_cache;
};

#endif
