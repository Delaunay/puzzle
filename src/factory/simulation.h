#ifndef PUZZLE_SIMULATION_SIM_HEADER
#define PUZZLE_SIMULATION_SIM_HEADER

#include <memory>

class NodeLink;
class Node;
class Forest;
class Recipe;
class Pin;
// Using strategy pattern to define how each building behave in the simulation


// Simulation strategy
struct SimulationLogic{
    Forest* forest = nullptr;

    SimulationLogic(Forest* f):
        forest(f)
    {}

    virtual void tick() = 0;

    virtual ~SimulationLogic();

    NodeLink const* find_link(Pin const* pin) const;

    NodeLink* find_link(Pin* pin);
};


// Manufacture new items given the correct inputs
struct ManufacturerLogic: public SimulationLogic {
    Node* self;

    ManufacturerLogic(Forest* f, Node* s):
        SimulationLogic(f), self(s)
    {}

    void tick() override;

    void fetch_inputs(Recipe* recipe);

    void manufacture(Recipe* recipe);

    void dispatch_outputs(Recipe* recipe);

    void clear_outputs(Recipe* recipe);
};


// Redistribute items through in/out pins
struct RelayLogic: public SimulationLogic{
    Node* self;
    // FIXME: make this configurable
    int capacity = 780.f;

    RelayLogic(Forest* f, Node* s);

    void tick() override;

    virtual void fetch_inputs();

    virtual void dispatch_outputs();

    void clear_outputs();
};

// Store any items for later
struct ContainerLogic: public RelayLogic {
    Node* self;

    ContainerLogic(Forest* f, Node* s):
        RelayLogic(f, s)
    {
        capacity = 1800.f;
    }
};


// Bring items from one node to the other
struct LinkLogic: public SimulationLogic{
    NodeLink* self;

    LinkLogic(Forest* f, NodeLink* s):
        SimulationLogic(f), self(s)
    {}

    void tick() override;
};

using SimualtionStep = std::unique_ptr<SimulationLogic>;

SimualtionStep fetch_logic(Forest* f, Node* self);
SimualtionStep fetch_logic(Forest* f, NodeLink* self);

#endif
