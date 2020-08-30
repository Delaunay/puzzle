#ifndef PUZZLE_EDITOR_NODE_HEADER
#define PUZZLE_EDITOR_NODE_HEADER

#include "config.h"
#include "utils.h"
#include "pin.h"

#include "factory/simulation.h"

#include <array>
#include <unordered_set>


struct Node {
    // TODO: make this configurable
    // building scale
    static float constexpr scaling = 10.f;

    const std::size_t ID;
    ImVec2            Pos;
    ImVec2            _size;
    Building*         descriptor = nullptr;
    int               building   = -1;
    int               recipe_idx = -1;
    int               rotation   =  0;
    float             efficiency =  0.f;
    SimualtionStep    logic      = nullptr;

    ProductionBook const& production () const {
        return logic->production;
    }

    bool is_pipeline_cross() const;

    std::vector<std::vector<Pin>> pins;

    Node(int building, const ImVec2& pos, int recipe_idx=-1, int rotation=0);

    Recipe* recipe() const;

    void update_size(ImVec2);

    ImVec2 size() const;

    std::vector<Pin*> input_pins;
    std::vector<Pin*> output_pins;

    void add_pin(int side, char type, bool input, int pin_side, int i, int n);

    bool is_relay(){
        static std::unordered_set<std::string> relays = {
            "Conveyor Splitter",
            "Conveyor Merger",
            "Pipeline Junction Cross (S)",
            "Pipeline Junction Cross (M)",
            };
        return relays.count(descriptor->name) == 1;
    }

    bool is_storage(){
        static std::unordered_set<std::string> relays = {
            "Storage Container",
        };
        return relays.count(descriptor->name) == 1;
    }

    bool is_input_pipe(int i){
        return input_pins[std::size_t(i)]->belt_type == 'P';
    }

    bool is_input_conveyor(int i){
        return !is_input_pipe(i);
    }

    // Return slot position (no rotation)
    ImVec2 left_slots  (float num, float count) const;
    ImVec2 right_slots (float num, float count) const;
    ImVec2 top_slots   (float num, float count) const;
    ImVec2 bottom_slots(float num, float count) const;

    // Slot position taking rotation into account
    ImVec2 slot_position(int side, float num, float count) const;

    bool operator== (Node const& obj){
        return obj.ID == ID;
    }
};

#endif
