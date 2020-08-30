#include "simulation.h"
#include "editor/forest.h"


SimulationLogic::~SimulationLogic(){}


NodeLink const* SimulationLogic::find_link(Pin const* pin) const {
    return forest->find_link(pin);
}

NodeLink* SimulationLogic::find_link(Pin* pin) {
    return forest->find_link(pin);
}


SimualtionStep fetch_logic(Forest* f, Node* self){
    if (self->is_relay()){
        return std::make_unique<RelayLogic>(f, self);
    }

    if (self->is_storage()){
        return std::make_unique<ContainerLogic>(f, self);
    }

    return std::make_unique<ManufacturerLogic>(f, self);
}

SimualtionStep fetch_logic(Forest* f, NodeLink* self){
    return std::make_unique<LinkLogic>(f, self);
}



void Simulation::compute_production(){
    static int stop = 0;
    static int steps = 0;

    if (stop == 0){
        forest->traverse([](Node* n){ n->logic->tick(); });
        return;
    }

    // for debugging. if stops is set the simulation stops after a few steps
    for(; steps < stop; steps++){
        forest->traverse([](Node* n){ n->logic->tick(); });
    }
}

ProductionBook Simulation::production_statement(){
    ProductionBook prod;

    for(auto& node: forest->iter_nodes()){
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

Engery Simulation::compute_electricity(){
    Engery stat;

    for(auto& node: forest->iter_nodes()){
        if (node.descriptor){
            float energy = node.descriptor->energy * node.efficiency;

            if (node.descriptor->energy > 0){
                stat.produced += energy;
            } else {
                stat.consumed += energy;
            }

            stat.stats[node.descriptor] += energy;
        }
    }

    return stat;
}


ProductionBook Simulation::raw_materials(){
    ProductionBook low_tier;
    for(auto& leaf: forest->roots()){
        ProductionBook const& prod = leaf->production();
        float eff = leaf->efficiency; // compute_efficiency(prod, false);

        for(auto& item: prod){
            if (item.second.produced <= 0)
                continue;

            low_tier[item.first].consumed += item.second.consumed * eff;
            low_tier[item.first].produced += item.second.produced * eff;
            low_tier[item.first].received += item.second.received * eff;
        }
    }
    return low_tier;
}

ProductionBook Simulation::top_items(){
    // Roots Productions (highest tier item)
    ProductionBook high_tier;
    for(auto& leaf: forest->leaves()){
        ProductionBook const& prod = leaf->production();
        float eff = leaf->efficiency; // compute_efficiency(prod, false);

        for(auto& item: prod){
            if (item.second.produced <= 0)
                continue;

            high_tier[item.first].consumed += item.second.consumed * eff;
            high_tier[item.first].produced += item.second.produced * eff;
            high_tier[item.first].received += item.second.received * eff;
        }
    }
    return high_tier;
}
