#ifndef PUZZLE_EDITOR_HEADER
#define PUZZLE_EDITOR_HEADER
// Creating a node graph editor for Dear ImGui
// Quick sample, not production code! This is more of a demo of how to use Dear ImGui to create custom stuff.
// See https://github.com/ocornut/imgui/issues/306 for details
// And more fancy node editors: https://github.com/ocornut/imgui/wiki#Useful-widgets--references

// Changelog
// - v0.04 (2020-03): minor tweaks
// - v0.03 (2018-03): fixed grid offset issue, inverted sign of 'scrolling'



#include "config.h"
#include "application/application.h"
#include "application/utils.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include <imgui.h>
#include <math.h> // fmodf

#include <SDL2/SDL_keycode.h>

// NB: You can use math functions/operators on ImVec2 if you #define IMGUI_DEFINE_MATH_OPERATORS and #include "imgui_internal.h"
// Here we only declare simple +/- operators so others don't leak into the demo code.
// static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
// static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

// Dummy data structure provided for the example.
// Note that we storing links as indices (not ID) to make example code shorter.


struct NPin {
    std::size_t ID;

    NPin():
        ID(next_id<std::size_t>())
    {}
};


template <typename T> float sgn(T val) {
    return (T(0) < val) - (val < T(0));
}


// Draw a bezier curve using only 2 points
// derive the two other points needed from the starting points
// they derived points change in function of their relative position
void draw_bezier(ImDrawList* draw_list, ImVec2 p1, ImVec2 p2, ImU32 color){
    if (p1.x > p2.x) {
        std::swap(p1, p2);
    }

    auto offset = ImVec2((p2.x - p1.x + 1) / 2.f , 0);
    auto y_offset = sgn(p2.y - p1.y) * (p2.y - p1.y) / 2.f;

    if (p2.y - p1.y > p2.x - p1.x){
        offset = ImVec2(0, y_offset);
    }

    draw_list->AddBezierCurve(
        p1,
        p1 + offset,
        p2 - offset,
        p2,
        color,
        // Conveyor belt are w=2
        10.f * 2.f);
}


// R: Rotate
// Q: Copy building
struct Node {
    static float constexpr scaling = 10.f;

    std::size_t ID;
    ImVec2      Pos;
    ImVec2      _size;
    float       Value;
    ImVec4      Color;

    std::vector<NPin> inputs;
    std::vector<NPin> outputs;
    Building*         descriptor = nullptr;
    int               building   = -1;
    int               recipe_idx = -1;
    int               rotation   = 0;

    Recipe* recipe(){
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

    Node(int building, const ImVec2& pos, int recipe_idx=-1, int rotation=0):
        ID(next_id<std::size_t>()), building(building), recipe_idx(recipe_idx), rotation(rotation)
    {
        if (building >= 0){
            descriptor = &Resources::instance().buildings[building];
        }
        Pos = pos;
        inputs = std::vector<NPin>(descriptor->inputs.size());
        outputs = std::vector<NPin>(descriptor->outputs.size());
    }

    void update_size(ImVec2 size){
        ImVec2 base = {scaling * descriptor->l, scaling *descriptor->w};
        _size = base;
    }

    ImVec2 size() const {
        switch (Direction(rotation)){
        case LeftToRight:
        case RightToLeft:
            return _size;

        case TopToBottom:
        case BottomToTop:
            return ImVec2(_size.y, _size.x);
        }
    }

    //                  [b]
    //     *-------------------------------*
    //     |                               |
    // --->x                               |
    // [a] |                               x--->  [c]
    // --->x                               |
    //     |                               |
    //     *-------------------------------*
    //                  [d]
    //

    enum Direction {
        LeftToRight,
        TopToBottom,
        RightToLeft,
        BottomToTop
    };

    ImVec2 left_slots(float num, float count) const {
        return ImVec2(
            Pos.x,
            Pos.y + size().y * (float(num + 1)) / (float(count + 1)));
    }

    ImVec2 right_slots(float num, float count) const {
        return ImVec2(
            Pos.x + size().x,
            Pos.y + size().y * (float(num + 1)) / (float(count + 1)));
    }

    ImVec2 top_slots(float num, float count) const {
        return ImVec2(
            Pos.x + size().x * (float(num + 1)) / (float(count + 1)),
            Pos.y);
    }

    ImVec2 bottom_slots(float num, float count) const {
        return ImVec2(
            Pos.x + size().x * (float(num + 1)) / (float(count + 1)),
            Pos.y + size().y);
    }

    ImVec2 get_input_slot_position(std::size_t slot_no) const {
        switch (Direction(rotation)){
        case LeftToRight:
            return left_slots(float(slot_no), float(inputs.size()));

        case RightToLeft:
            return right_slots(float(slot_no), float(inputs.size()));

        case BottomToTop:
            return bottom_slots(float(slot_no), float(inputs.size()));

        case TopToBottom:
            return top_slots(float(slot_no), float(inputs.size()));
        }
    }

    ImVec2 get_output_slot_position(std::size_t slot_no) const {
        switch (Direction(rotation)){
        case LeftToRight:
            return right_slots(float(slot_no), float(outputs.size()));

        case RightToLeft:
            return left_slots(float(slot_no), float(outputs.size()));

        case BottomToTop:
            return top_slots(float(slot_no), float(outputs.size()));

        case TopToBottom:
            return bottom_slots(float(slot_no), float(outputs.size()));
        }
    }
};

struct NodeLink{
    int InputIdx, InputSlot, OutputIdx, OutputSlot;

    NodeLink(int input_idx, int input_slot, int output_idx, int output_slot) {
        InputIdx   = input_idx;
        InputSlot  = input_slot;
        OutputIdx  = output_idx;
        OutputSlot = output_slot;
    }
};

struct LinkDragDropState {
    ImVec2      start_point;
    bool        should_draw_path = false;
    bool        release          = false;

    int         start_pin_slot = -1;
    int         start_node_id  = -1;
    const Node* start_node     = nullptr;
    bool        start_pin_type = false;

    int         end_pin_slot   = -1;
    int         end_node_id    = -1;
    const Node* end_node       = nullptr;
    bool        end_pin_type   = false;

    void set_starting_point(ImVec2 pos, Node const* node, int node_id, int pin_slot, bool input){
        should_draw_path = true;
        start_point = pos;
        start_node = node;
        start_pin_slot = pin_slot;
        start_pin_type = input;
        start_node_id = node_id;
        release = false;

        debug("Starting new link from ({}, {})", start_node->ID, start_pin_slot);
    }

    void set_end_point(Node const* node, int node_id, int pin_slot, bool input){
        if (should_draw_path && node != start_node && node != end_node){
            end_node_id = node_id;
            end_node = node;
            end_pin_slot = pin_slot;
            end_pin_type = input;
            release = true;
            debug("End new link to ({}, {}) {}", end_node->ID, end_pin_slot, release);
        }
    }

    void reset(){
        start_point = ImVec2(-1, -1);
        should_draw_path = false;
        release = false;
        start_node = nullptr;
        start_pin_slot = -1;
        end_node = nullptr;
        end_pin_slot = -1;
        debug("Reset dragndrop");
    }

    void make_new_link(std::vector<NodeLink>& links){
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (release){
                if (start_pin_type != end_pin_type){
                    // if it is not an input
                    if (start_pin_type) {
                        std::swap(start_node, end_node);
                        std::swap(start_pin_slot, end_pin_slot);
                    }

                    auto start_type = start_node->descriptor->outputs[start_pin_slot][0];
                    auto end_type = end_node->descriptor->inputs[end_pin_slot][0];

                    if (start_type == end_type){
                        debug("Make Connection {} -> {}", start_node->ID, end_node->ID);
                        links.emplace_back(
                            start_node_id,
                            start_pin_slot,
                            end_node_id,
                            end_pin_slot);
                    } else {
                        debug("Cannot connect {} to {}!", start_type, end_type);
                    }
                } else {
                    debug("Cannot connect input to input!");
                }
            }
            reset();
        }
    }

    void draw_path(ImDrawList* draw_list){
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
};

struct NodeEditor{
    using NodePtr = std::shared_ptr<Node>;
    puzzle::Application& app;

    NodeEditor(puzzle::Application& app):
        app(app)
    {
        inited = true;
    }

    Node* get_node(std::size_t idx) {
        return nodes[idx].get();
    }

    struct Brush {
        int       building = 0;
        int       recipe   = -1;
        int       rotation = 0;

        std::vector<const char*> building_names;
        std::vector<const char*> recipe_names;

        Building* building_descriptor(){
            if (building == -1)
                return nullptr;
            return &Resources::instance().buildings[building];
        }

        Recipe* recipe_descriptor(){
            if (recipe == -1)
                return nullptr;

            Building* building = building_descriptor();
            if (!building)
                return nullptr;

            return &building->recipes[recipe];
        }

        void set(int b, int r, int rot = 0){
            building = b;
            rotation = rot;

            if (b >= 0){
                recipe_names = building_descriptor()->recipe_names();
            } else {
                recipe_names.clear();
            }

            recipe = r;
        }
    };

    Brush brush;

    std::vector<NodePtr>  nodes;
    std::vector<NodeLink> links;
    ImVec2 scrolling = ImVec2(0.0f, 0.0f);

    // returns nodes that have no parents
    std::vector<Node*> roots(){
        for (auto& node: nodes){

        }
    }

    bool inited = false;
    bool show_grid = true;
    bool opened = true;

    // Draw a list of nodes on the left side
    bool open_context_menu = false;

    // Node Selection
    Node* node_selected         = nullptr;
    Node* node_hovered_in_list  = nullptr;
    Node* node_hovered_in_scene = nullptr;

    LinkDragDropState link_builder;

    const float NODE_SLOT_RADIUS = 2.0f * 10.f * 0.75f;
    const ImVec2 NODE_WINDOW_PADDING = {10.0f, 10.0f};

    void draw_pin(std::size_t slot_idx, Node const* node, NPin const& pin, ImDrawList* draw_list, ImVec2 offset, bool is_input, std::size_t node_idx){
        char type;
        if (is_input) {
            type = node->descriptor->inputs[slot_idx][0];
        } else {
            type = node->descriptor->outputs[slot_idx][0];
        }

        if (type != 'C' && type != 'P'){
            return;
        }

        ImGui::PushID(int(pin.ID));

        ImVec2 center;
        ImU32 color;

        if (is_input){
            center = offset + node->get_input_slot_position(slot_idx);
            color = IM_COL32(255, 179, 119, 255);
        } else{
            center = offset + node->get_output_slot_position(slot_idx);
            color = IM_COL32(84, 252, 193, 255);
        }

        auto rad = ImVec2(NODE_SLOT_RADIUS, NODE_SLOT_RADIUS);
        ImRect bb(center - ImVec2(rad), center + rad);

        if (type == 'C') {
            draw_list->AddRectFilled(
                center - ImVec2(rad),
                center + rad,
                color,
                4.0f,
                ImDrawCornerFlags_All);
        }
        else if (type == 'P') {
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
            link_builder.set_starting_point(center, node, node_idx, slot_idx, is_input);
        }

        // Set Ending point
        if (hovered) {
            link_builder.set_end_point(node, node_idx, slot_idx, is_input);
        }

        ImGui::PopID();
    }

    void draw_node(std::size_t node_idx, Node* node, ImDrawList* draw_list, ImVec2 offset){
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

        // Pin selection is on top of everything
        for (std::size_t slot_idx = 0; slot_idx < node->inputs.size(); slot_idx++){
            NPin& pin = node->inputs[slot_idx];
            draw_pin(slot_idx, node, pin, draw_list, offset, true, node_idx);
        }

        for (std::size_t slot_idx = 0; slot_idx < node->outputs.size(); slot_idx++){
            NPin& pin = node->outputs[slot_idx];
            draw_pin(slot_idx, node, pin, draw_list, offset, false, node_idx);
        }

        ImGui::EndGroup();

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

        if (node_moving_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            node->Pos = node->Pos + io.MouseDelta;

        // Shortcuts
        if (ImGui::IsItemHovered()) {
            if (ImGui::IsKeyReleased(SDL_SCANCODE_R)){
                node->rotation = (node->rotation + 1) % 4;
            }

            if (ImGui::IsKeyReleased(SDL_SCANCODE_Q)){
                brush.set(node->building, node->recipe_idx, node->rotation);
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

    void draw_workspace(){
        ImGuiIO& io = ImGui::GetIO();
        ImGui::BeginGroup();

        // Create our child canvas
        ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", scrolling.x, scrolling.y);
        ImGui::SameLine(ImGui::GetWindowWidth() - 100);
        ImGui::Checkbox("Show grid", &show_grid);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(60, 60, 70, 200));
        ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
        ImGui::PopStyleVar(); // WindowPadding
        ImGui::PushItemWidth(120.0f);

        const ImVec2 offset = ImGui::GetCursorScreenPos() + scrolling;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Display grid
        if (show_grid)
        {
            ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
            // Foundation is 8 x 8 and we scale by 10
            float GRID_SZ = 8.f * 10.0f;
            ImVec2 win_pos = ImGui::GetCursorScreenPos();
            ImVec2 canvas_sz = ImGui::GetWindowSize();

            for (float x = fmodf(scrolling.x, GRID_SZ); x < canvas_sz.x; x += GRID_SZ)
                draw_list->AddLine(ImVec2(x, 0.0f) + win_pos, ImVec2(x, canvas_sz.y) + win_pos, GRID_COLOR);

            for (float y = fmodf(scrolling.y, GRID_SZ); y < canvas_sz.y; y += GRID_SZ)
                draw_list->AddLine(ImVec2(0.0f, y) + win_pos, ImVec2(canvas_sz.x, y) + win_pos, GRID_COLOR);
        }

        // Display links
        draw_list->ChannelsSplit(2);
        draw_list->ChannelsSetCurrent(0); // Background
        for (int link_idx = 0; link_idx < links.size(); link_idx++)
        {
            NodeLink* link = &links[link_idx];
            Node* node_inp = get_node(link->InputIdx);
            Node* node_out = get_node(link->OutputIdx);

            ImVec2 p1 = offset + node_inp->get_output_slot_position(link->InputSlot);
            ImVec2 p2 = offset + node_out->get_input_slot_position(link->OutputSlot);

            draw_bezier(
                draw_list,
                p1,
                p2,
                IM_COL32(200, 200, 100, 255)
            );
        }

        // Display nodes
        for (std::size_t node_idx = 0; node_idx < nodes.size(); node_idx++){
            draw_node(node_idx, get_node(node_idx), draw_list, offset);
        }

        link_builder.draw_path(draw_list);
        link_builder.make_new_link(links);

        draw_list->ChannelsMerge();

        // Open context menu
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)){
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) || !ImGui::IsAnyItemHovered()){
                node_selected = node_hovered_in_list = node_hovered_in_scene = nullptr;
                open_context_menu = true;
            }
        }

        // Select node and show stats
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)){
            if (node_hovered_in_scene != nullptr) {
                selected_node = node_hovered_in_scene;
            }
        }

        if (open_context_menu) {
            ImGui::OpenPopup("context_menu");
            if (node_hovered_in_list != nullptr)
                node_selected = node_hovered_in_list;

            if (node_hovered_in_scene != nullptr)
                node_selected = node_hovered_in_scene;
        }

        // Draw context menu
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        if (ImGui::BeginPopup("context_menu"))
        {
            Node* node = node_selected;
            ImVec2 scene_pos = ImGui::GetMousePosOnOpeningCurrentPopup() - offset;

            if (node)
            {
                ImGui::Text("Node '%ul'", node->ID);
                ImGui::Separator();
                if (ImGui::MenuItem("Rename..", nullptr, false, false)) {}
                if (ImGui::MenuItem("Delete", nullptr, false, false)) {}
                if (ImGui::MenuItem("Copy", nullptr, false, false)) {}
            }
            else
            {
                if (ImGui::MenuItem("Add")) {
                    debug("New node");
                    nodes.push_back(std::make_shared<Node>(brush.building, scene_pos, brush.recipe, brush.rotation % 4));
                }
                if (ImGui::MenuItem("Paste", nullptr, false, false)) {

                }
            }

            open_context_menu = false;
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();

        // Scrolling
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
            scrolling = scrolling + io.MouseDelta;

        ImGui::PopItemWidth();
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::EndGroup();
    }

    bool insert_shortcuts(){
        // Insert r into ImGui KeyMap
        // puzzle::add_new_key<SDLK_r>();
        return true;
    }

    void draw_brush(){
        // ImGui::Text("Brush");
        // ImGui::Separator();

        Resources& rsc = Resources::instance();
        if (brush.building_names.size() == 0){
            brush.building_names = rsc.building_names();
        }

        // debug("{} {}", brush.building_names.size(), rsc.building_names().size());

        bool building_changed = ImGui::Combo(
            "Building",
            &brush.building,
            brush.building_names.data(),
            brush.building_names.size());

        if (ImGui::InputInt("Rotation", &brush.rotation)){
            brush.rotation = brush.rotation % 4;
        }


        if (building_changed){
            brush.set(brush.building, -1);
        }

        auto building = brush.building_descriptor();
        if (brush.recipe_names.size() == 0 && building != nullptr){
            brush.recipe_names = building->recipe_names();
        }

        ImGui::Combo(
            "Brush Recipe",
            &brush.recipe,
            brush.recipe_names.data(),
            brush.recipe_names.size());

        draw_recipe_icon(
            brush.recipe_descriptor(),
            ImVec2(0.3f, 0.3f),
            ImVec2(ImGui::GetWindowWidth(), 0))
        ;
    }

    Node* selected_node = nullptr;

    void draw_recipe_icon(Recipe* recipe, ImVec2 scale, ImVec2 size = ImVec2(0, 0)){
        auto offset = ImVec2(0, 0);
        auto img_size = ImVec2(256, 256) * scale;
        auto pos = ImGui::GetCursorScreenPos();

        if (recipe && recipe->texture.size() > 0){
            Image* _ = Resources::instance().load_texture(recipe->texture);
            auto image = app.load_texture_image(*_);

            if (image->descriptor_set == nullptr){
                image->descriptor_set = ImGui_ImplVulkan_AddTexture(
                    image->sampler,
                    image->view,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            img_size = ImVec2(_->x, _->y) * scale;
            if (size.x + size.y > 0){
                offset = (size - img_size) * ImVec2(
                             0.5f * float(size.x != 0),
                             0.5f * float(size.y != 0));
            }

            ImGui::SetCursorScreenPos(pos + offset);

            if (image->descriptor_set){
                ImGui::Image(
                    image->descriptor_set,
                    img_size,
                    ImVec2(0.0f, 0.0f),
                    ImVec2(1.0f, 1.0f),
                    ImVec4(1, 1, 1, 1),
                    ImVec4(1, 1, 1, 0.25));
            }
        // Placeholder code
        } else {
            if (size.x + size.y > 0){
                offset = (size - img_size) * ImVec2(
                             0.5f * float(size.x != 0),
                             0.5f * float(size.y != 0));
            }

            ImGui::SetCursorScreenPos(pos + offset);
            ImGui::Button("Missing Image", img_size);
        }
    }

    void draw_node_list(){
        // ImGui::BeginChild("node_list", ImVec2(100, 0));
        // ImGui::Text("Nodes");
        // ImGui::Separator();

        for (std::size_t node_idx = 0; node_idx < nodes.size(); node_idx++){
            Node* node = get_node(node_idx);
            ImGui::PushID(int(node->ID));

            if (ImGui::Selectable(node->descriptor->name.c_str(), node == node_selected)){
                node_selected = node;
            }

            if (ImGui::IsItemHovered()){
                node_hovered_in_list = node;
                open_context_menu |= ImGui::IsMouseClicked(1);
            }
            ImGui::PopID();
        }
        // ImGui::EndChild();
        // ImGui::SameLine();
    }

    bool brush_panel_open = true;
    bool entity_panel_open = true;
    bool selected_panel = true;

    std::vector<const char*> const* available_recipes;

    void draw_selected_info(){
        if (selected_node == nullptr){
            return;
        }

        Building* b = selected_node->descriptor;
        if (b == nullptr){
            return;
        }

        {
            ImGui::Text("Building Spec");
            ImGui::Columns(2, "spec-cols", true);
            ImGui::Separator();

            ImGui::Text("Name");
            ImGui::Text("Energy");
            ImGui::Text("Dimension");

            ImGui::NextColumn();

            ImGui::Text("%s", b->name.c_str());
            ImGui::Text("%.2f", b->energy);
            ImGui::Text("%.0f x %.0f", b->w, b->l);

            ImGui::Separator();
        }

        // back to normal mode
        ImGui::Columns(1);

        // Select a new recipe
        available_recipes = &b->recipe_names();

        ImGui::Combo(
            "Recipe",
            &selected_node->recipe_idx,
            available_recipes->data(),
            available_recipes->size());

        // display selected recipe
        auto selected_recipe = selected_node->recipe();
        draw_recipe_icon(selected_recipe, ImVec2(0.3f, 0.3f), ImVec2(ImGui::GetWindowWidth(), 0));

        if (selected_recipe != nullptr){
            ImGui::Text("Inputs");
            ImGui::Separator();
            ImGui::Columns(2, "inputs-cols", true);

            ImGui::Text("Name");
            for (auto& in: selected_recipe->inputs){
                ImGui::Text("%s", in.name.c_str());
            }

            ImGui::NextColumn();
            ImGui::Text("Speed (item/min)");
            ImGui::Separator();
            for (auto& in: selected_recipe->inputs){
                ImGui::Text("%.2f", in.speed);
            }

            ImGui::Separator();
            ImGui::Columns(1);
            // Finished

            ImGui::Spacing();

            ImGui::Text("Outputs");
            ImGui::Separator();
            ImGui::Columns(2, "outputs-cols", true);

            ImGui::Text("Name");
            for (auto& out: selected_recipe->outputs){
                ImGui::Text("%s", out.name.c_str());
            }

            ImGui::NextColumn();
            ImGui::Text("Speed (item/min)");
            ImGui::Separator();
            for (auto& out: selected_recipe->outputs){
                ImGui::Text("%.2f", out.speed);
            }

            ImGui::Separator();
            ImGui::Columns(1);
            // Finished
        }
    }

    void draw_tool_panel(){
        ImGui::Begin("Tool box");
        auto open = ImGuiTreeNodeFlags_DefaultOpen;

        if (ImGui::CollapsingHeader("Brush", &brush_panel_open, open)){
            draw_brush();
        }

        if (ImGui::CollapsingHeader("Selected Building", &selected_panel, open)){
            draw_selected_info();
        }

        if (ImGui::CollapsingHeader("Entity List", &entity_panel_open, open)){
            draw_node_list();
        }

        ImGui::End();
    }

    void draw(){
        static bool _ = insert_shortcuts();

        ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Example: Custom Node Graph", &opened))
        {
            ImGui::End();
            return;
        }

        draw_tool_panel();
        draw_workspace();

        ImGui::End();
    }
};

#endif
