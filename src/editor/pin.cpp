#include "pin.h"
#include "node.h"
#include "application/logger.h"


Pin::Pin(char type, bool is_input, int side, int index, int count, Node* parent):
    ID(next_id<std::size_t>()), belt_type(type), is_input(is_input),
    side(side), index(index), count(count), parent(parent)
{}

Pin::Pin(Pin const&& obj) noexcept:
    ID(obj.ID), belt_type(obj.belt_type), side(obj.side), index(obj.index),
    count(obj.count), parent(obj.parent)
{}


ImVec2 Pin::position() const {
    return parent->slot_position(side, float(index), float(count));
}

std::ostream& operator<<(std::ostream& out, Pin const& pin){
    return out << fmt::format(
               "Pin(ID={}, type={}, input={}, side={}, index={}, count={}, parent={})",
               pin.ID, pin.belt_type, pin.is_input, pin.side, pin.index, pin.count, pin.parent->ID);
}


#include "node-editor.h"

void NodeEditor::draw_pin(ImDrawList* draw_list, ImVec2 offset, Pin const& pin) {
    if (pin.belt_type != 'C' && pin.belt_type != 'P'){
        // debug("Ignored {}", pin.belt_type);
        return;
    }

    ImGui::PushID(int(pin.ID));
    ImU32 color;
    auto center = offset + pin.position();

    if (pin.is_input){
        color = IM_COL32(255, 179, 119, 255);
    } else{
        color = IM_COL32(84, 252, 193, 255);
    }

    auto rad = ImVec2(NODE_SLOT_RADIUS, NODE_SLOT_RADIUS);
    ImRect bb(center - ImVec2(rad), center + rad);

    if (pin.belt_type == 'C') {
        draw_list->AddRectFilled(
            center - ImVec2(rad),
            center + rad,
            color,
            4.0f,
            ImDrawCornerFlags_All);
    }
    else if (pin.belt_type == 'P') {
        draw_list->AddCircleFilled(
            center,
            NODE_SLOT_RADIUS,
            color);
    }

    bool hovered;
    bool held;
    auto flags = ImGuiButtonFlags_PressedOnClick;
    bool pressed = ImGui::ButtonBehavior(bb, unsigned(pin.ID), &hovered, &held, flags);

    // Get Starting point
    if (pressed && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        link_builder.set_starting_point(center, &pin);
    }

    // Set Ending point
    else if (hovered) {
        link_builder.set_end_point(&pin);
    }

    if (pressed) {
        select_link(&pin);
    }

    ImGui::PopID();
}
