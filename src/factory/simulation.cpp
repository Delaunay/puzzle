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
