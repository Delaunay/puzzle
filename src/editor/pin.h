#ifndef PUZZLE_EDITOR_PIN_HEADER
#define PUZZLE_EDITOR_PIN_HEADER

#include "utils.h"

struct Node;

// Attachable widget
struct Pin {
    const std::size_t ID;
    char belt_type = ' ';
    bool is_input  = false;

    int   side         = 0;
    int   index        = 0;
    int   count        = 0;

    Node* const parent = nullptr;
    Node* child        = nullptr;

    // Holds all the data necessary for it to compute its position
    ImVec2 position() const;

    // Pins are assigned to Nodes, they should not be created outside them
    Pin(Pin const&) = delete;

    // We need move for std::vector resize event though it should never get resized
    Pin(Pin const&& obj) noexcept;

    Pin(char type, bool is_input, int side, int index, int count, Node* parent);
};

std::ostream& operator<<(std::ostream& out, Pin const& pin);


#endif
