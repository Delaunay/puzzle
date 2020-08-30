#include "node.h"
#include "factory/simulation.h"

bool Node::is_pipeline_cross() const {
    if (descriptor->name == "Pipeline Junction Cross (S)" || descriptor->name == "Pipeline Junction Cross (M)"){
        return true;
    }
    return false;
}

Node::Node(int building, const ImVec2& pos, int recipe_idx, int rotation):
    ID(next_id<std::size_t>()), building(building), recipe_idx(recipe_idx), rotation(rotation), pins(4)
{
    assertf(building >= 0, "Node should have a building");

    descriptor = &Resources::instance().buildings[std::size_t(building)];

    for(auto& side: descriptor->layout){
        int pin_side = get_side(side.first);
        std::vector<std::string>& pin_str = side.second;

        std::vector<Pin>& side_pins = pins[std::size_t(get_side(side.first))];
        side_pins.reserve(side.second.size());

        for(int i = 0, n = pin_str.size(); i < n; ++i) {
            auto& p = pin_str[std::size_t(i)];
            assertf(p.size() == 2,
                    "Pin descriptor is of size 2");

            assertf(p[0] == 'C' || p[0] == 'P' || p[0] == 'N',
                    "Pin type should be defined");

            assertf(p[1] == 'I' || p[1] == 'O',
                    "Pin type should be defined");

            add_pin(get_side(side.first),
                    p[0],           // Belt Type
                    p[1] == 'I',    // Input
                    pin_side,       // Pin Side
                    i,              // Pin Index
                    n);             // Pin Count on that side
        }
    }
    Pos = pos;
}

void Node::add_pin(int side, char type, bool input, int pin_side, int i, int n){
    std::vector<Pin>& side_array = pins[std::size_t(side)];
    side_array.emplace_back(type, input, pin_side, i, n, this);

    if (is_pipeline_cross()){
        input_pins.emplace_back(&*side_array.rbegin());
        output_pins.emplace_back(&*side_array.rbegin());
    } else if (input){
        input_pins.emplace_back(&*side_array.rbegin());
    } else {
        output_pins.emplace_back(&*side_array.rbegin());
    }
}

void Node::update_size(ImVec2){
    ImVec2 base = {scaling * descriptor->l, scaling *descriptor->w};
    _size = base;
}

ImVec2 Node::size() const {
    switch (Direction(rotation)){
    case LeftToRight:
    case RightToLeft:
        return _size;

    case TopToBottom:
    case BottomToTop:
        return ImVec2(_size.y, _size.x);
    }

    __builtin_unreachable();
}

ImVec2 Node::left_slots(float num, float count) const {
    return ImVec2(
        Pos.x,
        Pos.y + size().y * (float(num + 1)) / (float(count + 1)));
}

ImVec2 Node::right_slots(float num, float count) const {
    return ImVec2(
        Pos.x + size().x,
        Pos.y + size().y * (float(num + 1)) / (float(count + 1)));
}

ImVec2 Node::top_slots(float num, float count) const {
    return ImVec2(
        Pos.x + size().x * (float(num + 1)) / (float(count + 1)),
        Pos.y);
}

ImVec2 Node::bottom_slots(float num, float count) const {
    return ImVec2(
        Pos.x + size().x * (float(num + 1)) / (float(count + 1)),
        Pos.y + size().y);
}

ImVec2 Node::slot_position(int side, float num, float count) const {
    side = (side + rotation) % 4;

    switch (Direction(side)){
    case LeftToRight:
        return left_slots(num, count);

    case RightToLeft:
        return right_slots(num, count);

    case BottomToTop:
        return bottom_slots(num, count);

    case TopToBottom:
        return top_slots(num, count);
    }

    __builtin_unreachable();
}

Recipe* Node::recipe() const {
    if (descriptor == nullptr){
        return nullptr;
    }

    if (recipe_idx < 0){
        return nullptr;
    }

    if (recipe_idx < int(descriptor->recipes.size())){
        return &descriptor->recipes[std::size_t(recipe_idx)];
    }

    return nullptr;
}


#include "node-editor.h"

void NodeEditor::draw_node(Node* node, ImDrawList* draw_list, ImVec2 offset){
    ImGuiIO& io = ImGui::GetIO();

    ImGui::PushID(node->ID);
    ImVec2 node_rect_min = offset + node->Pos;

    // Display node contents first
    draw_list->ChannelsSetCurrent(1); // Foreground

    bool old_any_active = ImGui::IsAnyItemActive();
    ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
    ImGui::BeginGroup(); // Lock horizontal position

    //ImGui::Text("%s", node->descriptor->name.c_str());
    // auto text_size = ImGui::GetItemRectSize();

    auto recipe = node->recipe();

    ImGui::SetCursorScreenPos(node_rect_min);
    draw_recipe_icon(recipe, ImVec2(0.2f, 0.2f), node->size());

    ImGui::EndGroup();

    for(auto& side: node->pins){
        for(Pin const& pin: side){
            draw_pin(draw_list, offset, pin);
        }
    }

    // Save the size of what we have emitted and whether any of the widgets are being used
    bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
    node->update_size(ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING);
    ImVec2 node_rect_max = node_rect_min + node->size();

    // Display node box
    draw_list->ChannelsSetCurrent(0); // Background
    ImGui::SetCursorScreenPos(node_rect_min);
    ImGui::InvisibleButton("node", node->size());

    if (ImGui::IsItemHovered()){
        node_hovered_in_scene = node;
        open_context_menu |= ImGui::IsMouseClicked(1);
    }

    bool node_moving_active = ImGui::IsItemActive();

    if (node_widgets_active || node_moving_active)
        node_selected = node;

    if (node_moving_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)){
        node->Pos = node->Pos + io.MouseDelta;
    } else {
        node->Pos = snap(node->Pos);
    }

    // Shortcuts
    if (ImGui::IsItemHovered()) {
        if (ImGui::IsKeyReleased(SDL_SCANCODE_R)){
            node->rotation = (node->rotation + 1) % 4;
        }
    }

    // Draw rectangle
    ImU32 node_bg_color = IM_COL32(60, 60, 60, 255);
    if (node_hovered_in_list == node || node_hovered_in_scene == node || (!node_hovered_in_list && node_selected == node))
        node_bg_color = IM_COL32(75, 75, 75, 255);

    draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f);
    draw_list->AddRect(node_rect_min, node_rect_max, IM_COL32(100, 100, 100, 255), 4.0f);

    ImGui::PopID();
}
