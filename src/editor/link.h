#ifndef PUZZLE_EDITOR_LINK_HEADER
#define PUZZLE_EDITOR_LINK_HEADER

#include "pin.h"
#include "application/logger.h"

// Link between two pins
struct NodeLink{
    Pin const* start; // outputs
    Pin const* end;   // inputs

    NodeLink(Pin const* s, Pin const* e):
        start(s), end(e)
    {
        assertf(start != nullptr, "start cannot be null");
        assertf(end   != nullptr, "end cannot be null");
    }

    bool operator== (NodeLink const& obj){
        return obj.start == this->start && obj.end == this->end;
    }
};

struct NodeEditor;


// Link builder helper
// do manage drag and droping links between pins
struct LinkDragDropState {
    ImVec2      start_point;                // Starting point of the drawing
    bool        should_draw_path = false;   // Should draw the pending path
    bool        release          = false;   // Is ready to be released
    bool        is_hovering      = false;   // Did we hover over something NOW
                                            // if not we cancel the linking
    Pin const* start = nullptr;  // Output pin
    Pin const* end   = nullptr;  // Input pin

    void start_drag();

    void set_starting_point(ImVec2 pos, Pin const* s);

    void set_end_point(Pin const* e);

    void reset();

    void make_new_link(NodeEditor* editor);

    void draw_path(ImDrawList* draw_list);
};

#endif
