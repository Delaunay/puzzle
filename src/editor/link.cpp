#include "link.h"
#include "node-editor.h"

#include "application/logger.h"


NodeLink::NodeLink(Pin const* s, Pin const* e):
    ID(next_id<std::size_t>()), start(s), end(e)
{
    assertf(start != nullptr, "start cannot be null");
    assertf(end   != nullptr, "end cannot be null");

    if (start->belt_type == 'C'){
        assertf(!start->is_input, "Start the the output of a node");
        assertf(end->is_input   , "End   the the output of a node");
    }
}

void LinkDragDropState::start_drag(){
    is_hovering = false;
}

void LinkDragDropState::set_starting_point(ImVec2 pos, Pin const* s){
    if (s != nullptr) {
        should_draw_path = true;
        start_point = pos;
        start = s;
        release = false;
        debug("Starting new link from ({}, {})", s->parent->ID, s->index);
    }
}

void LinkDragDropState::make_new_link(NodeEditor* editor){
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (release && is_hovering){
            if (start->belt_type != end->belt_type){
                debug("Cannot connect {} to {}!", start->belt_type, end->belt_type);
                reset();
                return;
            }

            bool make_connection = false;

            // Conveyor belts have more constraints than pipes
            if (start->belt_type == 'C'){
                if (start->is_input == end->is_input){
                    debug("Cannot connect input to input!");
                    reset();
                    return;

                }

                // Start needs to be the output of a node
                if (start->is_input) {
                    std::swap(start, end);
                }
                make_connection = true;
            } else if (start->belt_type == 'P'){
                make_connection = true;
            }

            if (make_connection){
                debug("Make Connection {} -> {}", start->ID, end->ID);
                editor->select_link(editor->new_link(start, end));
            }
        }
        reset();
    }
}

void LinkDragDropState::set_end_point(Pin const* e){
    if (should_draw_path && e->parent != start->parent){
        if (end != nullptr && end->ID != e->ID){
            debug("End new link to ({}, {}) {}", e->parent->ID, e->index, release);
        }

        end = e;
        release = true;
        is_hovering = true;
    }
}

void LinkDragDropState::reset(){
    start_point = ImVec2(-1, -1);
    should_draw_path = false;
    release = false;
    start = nullptr;
    end = nullptr;
    debug("Reset dragndrop");
}

void LinkDragDropState::draw_path(ImDrawList* draw_list){
    if (should_draw_path){
        ImVec2 p1 = start_point;
        ImVec2 p2 = ImGui::GetMousePos();

        draw_bezier(
            draw_list,
            p1,
            p2,
            IM_COL32(200, 200, 100, 255)
            );
    }
}
