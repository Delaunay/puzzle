#ifndef PUZZLE_EDITOR_NODE_HEADER
#define PUZZLE_EDITOR_NODE_HEADER

#include "config.h"
#include "utils.h"
#include "pin.h"

#include <array>

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

    std::vector<std::vector<Pin>> pins;

    Node(int building, const ImVec2& pos, int recipe_idx=-1, int rotation=0);

    Recipe* recipe() const;

    void update_size(ImVec2);

    ImVec2 size() const;

    std::vector<Pin*> input_pins;
    std::vector<Pin*> output_pins;

    void add_pin(int side, char type, bool input, int pin_side, int i, int n);

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
