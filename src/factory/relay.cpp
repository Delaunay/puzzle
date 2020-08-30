#include "simulation.h"
#include "editor/forest.h"

RelayLogic::RelayLogic(Forest* f, Node* s):
    SimulationLogic(f), self(s)
{
    if (self->is_input_pipe(0)){
        capacity = 300.f;
    }
}

void RelayLogic::fetch_inputs(){
    // Gather all the resources we are receiving
    for(auto& in_pin: self->input_pins){
        auto in_link = find_link(in_pin);
        if (!in_link)
            continue;

        auto& link_prod = in_link->production;

        for(auto& item: link_prod){
            auto& prod = self->production[item.first];

            float remaining = prod.received;

            // Here use belt speed instead
            auto can_be_received = std::max(capacity - remaining, 0.f);
            can_be_received = std::min(can_be_received, item.second.produced);
            item.second.produced -= can_be_received;

            prod.received += can_be_received;
            prod.consumed += can_be_received;
            prod.limit_received += item.second.limit_produced;
        }
    }
}


void RelayLogic::clear_outputs(){
    for(auto& prod: self->production){
        prod.second.consumed = prod.second.produced;
        prod.second.produced = 0;
    }
}

void RelayLogic::dispatch_outputs(){
    bool is_merger = self->output_pins.size() == 1;

    // Split all the resources accross
    std::vector<NodeLink*> links;
    links.reserve(3);

    for(auto& out_pin: self->output_pins){
        auto out_link = find_link(out_pin);
        if (!out_link)
            continue;

        links.push_back(out_link);
    }

    if (links.size() == 0){
        clear_outputs();
        return;
    }

    if (is_merger){
        debug("# Merger {}", self->ID);
    } else {
        debug("# Splitter {}", self->ID);
    }

    ProductionBook available;
    for(auto& item: self->production){
        available[item.first].received = item.second.received / float(links.size());
        item.second.consumed = 0;
    }

    for(auto& link: links){
        for(auto& item:  self->production){
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

void RelayLogic::tick(){
    fetch_inputs();
    dispatch_outputs();
}
