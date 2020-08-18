#include "link.h"
#include "node-editor.h"

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
            if (start->is_input != end->is_input){
                // if it is not an input
                if (!start->is_input) {
                    std::swap(start, end);
                }

                if (start->belt_type == end->belt_type){
                    debug("Make Connection {} -> {}", start->ID, end->ID);
                    editor->new_link(start, end);
                } else {
                    debug("Cannot connect {} to {}!", start->belt_type, end->belt_type);
                }
            } else {
                debug("Cannot connect input to input!");
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
