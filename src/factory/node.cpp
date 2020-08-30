#include "simulation.h"
#include "editor/forest.h"


void ManufacturerLogic::tick(){
    auto recipe = self->recipe();

    if (recipe){
        dispatch_outputs(recipe);
        fetch_inputs(recipe);
        manufacture(recipe);
    }
}

void ManufacturerLogic::fetch_inputs(Recipe* recipe) {
    for(auto& ingredient: recipe->inputs){
        auto& prod = production[ingredient.name];

        for(auto& in_pin: self->input_pins){
            auto in_link = find_link(in_pin);
            if (!in_link)
                continue;

            if (in_pin->compatible(ingredient)) {
                auto can_be_received = std::max(ingredient.speed - prod.received, 0.f);
                auto received = std::min(can_be_received, in_link->production[ingredient.name].produced);

                in_link->production[ingredient.name].produced -= received;
                prod.received += received;
            }
        }
    }
}

void ManufacturerLogic::reset() {
    production.clear();
    self->efficiency = 0.f;
}

void ManufacturerLogic::manufacture(Recipe* recipe) {
    float in_efficiency = 1.f;
    float out_efficiency = 1.f;

    // check if our input is full
    for(auto& ingredient: recipe->inputs){
        auto& prod = production[ingredient.name];
        in_efficiency = std::min(in_efficiency, prod.received / ingredient.speed);

        // debug("{} {} ", ingredient.name, in_efficiency, );
    }

    // check if our output is full
    for(auto& ingredient: recipe->outputs){
        auto& prod = production[ingredient.name];

        // was not produced yet
        if (prod.produced <= 0)
            continue;

        out_efficiency = std::min(out_efficiency, prod.consumed / ingredient.speed);
    }

    self->efficiency = std::min(in_efficiency, out_efficiency);
    debug("({} {}) => (i: {}) (o: {})",
          self->ID,
          self->recipe()->recipe_name,
          in_efficiency,
          out_efficiency
          );

    // consume inputs
    for(auto& ingredient: recipe->inputs){
        auto& prod = production[ingredient.name];
        prod.received -= self->efficiency * ingredient.speed;
    }

    // produce outputs
    for(auto& ingredient: recipe->outputs){
        auto& prod = production[ingredient.name];
        prod.produced += self->efficiency * ingredient.speed;
    }
}

void ManufacturerLogic::dispatch_outputs(Recipe* recipe) {
    int out_link_count = 0;

    for (auto& ingredient: recipe->outputs){
        auto& prod = production[ingredient.name];
        prod.limit_produced = ingredient.speed;

        for(auto& out_pin: self->output_pins){
            auto out_link = find_link(out_pin);

            if (!out_link)
                continue;

            if (out_pin->compatible(ingredient)) {
                // the amount of resources remaining since last tick
                auto remaining = out_link->production[ingredient.name].produced;
                auto can_be_send = std::max(prod.produced - remaining, 0.f);

                if (can_be_send >= 0.f){
                    debug("max({}- {}, {})", prod.produced, remaining, 0.f);
                }

                out_link->production[ingredient.name].produced += can_be_send;
                out_link->production[ingredient.name].limit_produced = prod.limit_produced;

                prod.produced -= can_be_send;
                prod.consumed = can_be_send;
                out_link_count += 1;
            }
        }
    }

    // Leaf node, clear outputs
    if (out_link_count == 0) {
        clear_outputs(recipe);
    }
}


void ManufacturerLogic::clear_outputs(Recipe* recipe){
    for (auto& ingredient: recipe->outputs){
        auto& prod = production[ingredient.name];
        prod.produced = 0;
        prod.consumed = ingredient.speed;
    }
}
