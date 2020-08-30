#ifndef PUZZLE_SIMULATION_SIM_HEADER
#define PUZZLE_SIMULATION_SIM_HEADER

#include <memory>
#include <unordered_map>

#include <spdlog/fmt/bundled/format.h>


struct DoubleEntry{
    float debit  = 0.f;
    float credit = 0.f;

    DoubleEntry& operator+=(float v){
        debit += v;
        return *this;
    }

    DoubleEntry& operator-=(float v){
        credit += v;
        return *this;
    }

    float operator/ (float d) const {
        return total() / d;
    }

    float total() const {
        return debit - credit;
    }
};

inline
    std::ostream& operator<<(std::ostream& out, DoubleEntry const& ent){
    return out << ent.total();
}

template <>
struct fmt::formatter<DoubleEntry>: formatter<float> {
    template <typename FormatContext>
    auto format(DoubleEntry c, FormatContext& ctx) {
        return formatter<float>::format(c.total(), ctx);
    }
};

struct ItemStat{
    float consumed = 0;
    float produced = 0;
    float received = 0;

    float overflow   = 0;
    float efficiency = 0;

    float limit_consumed = 0;
    float limit_produced = 0;
    float limit_received = 0;
};


class Building;

struct Engery{
    float consumed = 0;
    float produced = 0;
    std::unordered_map<Building*, float> stats;
};

using ProductionBook  = std::unordered_map<std::string, ItemStat>;
using ProductionStats = std::unordered_map<std::size_t, ProductionBook>;


class NodeLink;
class Node;
class Forest;
class Recipe;
class Pin;
// Using strategy pattern to define how each building behave in the simulation


struct Simulation{
    ProductionBook raw_materials;
    ProductionBook top_items;
};


// Simulation strategy
struct SimulationLogic{
    Forest* forest = nullptr;
    ProductionBook production;

    SimulationLogic(Forest* f):
        forest(f)
    {}

    virtual void tick() = 0;

    virtual ~SimulationLogic();

    NodeLink const* find_link(Pin const* pin) const;

    NodeLink* find_link(Pin* pin);

    virtual void reset(){
        production.clear();
    }
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

    void reset() override;
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
