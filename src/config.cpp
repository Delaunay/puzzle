#include "config.h"
#include "application/sys.h"


void from_json(const json& j, Item& p) {
    j.at("name").get_to(p.name);
    j.at("qty").get_to(p.qty);
    j.at("speed").get_to(p.speed);
}

void from_json(const json& j, Recipe& p) {
    j.at("Recipe Name").get_to(p.recipe_name);
    j.at("Crafting Time (sec)").get_to(p.crafting_time);
    j.at("Ingredients").get_to(p.inputs);
    j.at("Products").get_to(p.outputs);

    if (j.contains("texture")){
        j.at("texture").get_to(p.texture);
    }
}

void from_json(const json& j, Building& p) {
    j.at("name").get_to(p.name);
    j.at("energy").get_to(p.energy);
    j.at("w").get_to(p.w);
    j.at("l").get_to(p.l);
    j.at("h").get_to(p.h);

    if (j.contains("inputs")){
        j.at("inputs").get_to(p.inputs);
    }

    if (j.contains("outputs")){
        j.at("outputs").get_to(p.outputs);
    }
}
