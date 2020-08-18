#ifndef PUZZLE_EDITOR_HEADER
#define PUZZLE_EDITOR_HEADER
// Creating a node graph editor for Dear ImGui
// Quick sample, not production code! This is more of a demo of how to use Dear ImGui to create custom stuff.
// See https://github.com/ocornut/imgui/issues/306 for details
// And more fancy node editors: https://github.com/ocornut/imgui/wiki#Useful-widgets--references

// Changelog
// - v0.04 (2020-03): minor tweaks
// - v0.03 (2018-03): fixed grid offset issue, inverted sign of 'scrolling'

#include <sstream>

#include "config.h"
#include "application/application.h"
#include "application/utils.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include <imgui.h>
#include <math.h> // fmodf
#include <list>

#include <SDL2/SDL_keycode.h>

#ifdef IM_COL32
#undef IM_COL32
#endif

#define IM_COL32(R,G,B,A)\
    (ImU32(ImU32(A)<<IM_COL32_A_SHIFT) | ImU32(ImU32(B)<<IM_COL32_B_SHIFT) | ImU32(ImU32(G)<<IM_COL32_G_SHIFT) | ImU32(ImU32(R)<<IM_COL32_R_SHIFT))

// NB: You can use math functions/operators on ImVec2 if you #define IMGUI_DEFINE_MATH_OPERATORS and #include "imgui_internal.h"
// Here we only declare simple +/- operators so others don't leak into the demo code.
// static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
// static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

// Dummy data structure provided for the example.
// Note that we storing links as indices (not ID) to make example code shorter.


int get_side(std::string const& name){
    if (name == "left")   return 0;
    if (name == "top")    return 1;
    if (name == "right")  return 2;
    if (name == "bottom") return 3;
    return 0;
}


template <typename T> float sgn(T val) {
    return (T(0) < val) - (val < T(0));
}


struct Node;

struct NPin {
    const std::size_t ID;
    char belt_type = ' ';
    bool is_input  = ' ';

    int   side         = 0;
    int   index        = 0;
    int   count        = 0;

    Node* const parent = nullptr;
    Node* child        = nullptr;

    // Pins are assigned to Nodes, they should not be created outside them
    NPin(NPin const&) = delete;

    // We need move for std::vector resize event though it should never get resized
    NPin(NPin const&& obj) noexcept;

    // Holds all the data necessary for it to compute its position
    ImVec2 position() const;

    NPin(char type, bool is_input, int side, int index, int count, Node* parent):
        ID(next_id<std::size_t>()), belt_type(type), is_input(is_input),
        side(side), index(index), count(count), parent(parent)
    {}
};


NPin::NPin(NPin const&& obj) noexcept:
    ID(obj.ID), belt_type(obj.belt_type), side(obj.side), index(obj.index),
    count(obj.count), parent(obj.parent), child(obj.child)
{}

std::ostream& operator<<(std::ostream& out, NPin const& pin);

// R: Rotate
// Q: Copy building
struct Node {
    static float constexpr scaling = 10.f;

    const std::size_t ID;
    ImVec2            Pos;
    ImVec2            _size;
    float             Value;
    ImVec4            Color;
    Building*         descriptor = nullptr;
    int               building   = -1;
    int               recipe_idx = -1;
    int               rotation   =  0;

    std::array<std::vector<NPin>, 4> pins;

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
        assertf(building >= 0, "Node shoudl have a building");

        descriptor = &Resources::instance().buildings[std::size_t(building)];

        for(auto& side: descriptor->layout){
            int pin_side = get_side(side.first);
            std::vector<std::string>& pin_str = side.second;

            std::vector<NPin>& side_pins = pins[std::size_t(get_side(side.first))];
            side_pins.reserve(side.second.size());

            for(int i = 0, n = pin_str.size(); i < n; ++i) {
                auto& p = pin_str[std::size_t(i)];
                assertf(p.size() == 2,
                        "Pin descriptor is of size 2");

                assertf(p[0] == 'C' || p[0] == 'P' || p[0] == 'N',
                        "Pin type should be defined");

                assertf(p[1] == 'I' || p[1] == 'O',
                        "Pin type should be defined");

                side_pins.emplace_back(
                    p[0],           // Belt Type
                    p[1] == 'I',    // Input
                    pin_side,       // Pin Side
                    i,              // Pin Index
                    n,              // Pin Count on that side
                    this);          // Parent

                const NPin& pin = *side_pins.rbegin();
                std::stringstream ss;
                ss << pin;
                debug("{}", ss.str());
            }
        }
        Pos = pos;
    }

    void update_size(ImVec2){
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

        __builtin_unreachable();
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

    ImVec2 slot_position(int side, float num, float count) const {
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
};


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
        Node::scaling * 2.f);
}



ImVec2 NPin::position() const {
    return parent->slot_position(side, float(index), float(count));
}

std::ostream& operator<<(std::ostream& out, NPin const& pin){
    return out << fmt::format(
               "Pin(ID={}, type={}, input={}, side={}, index={}, count={}, parent={})",
               pin.ID, pin.belt_type, pin.is_input, pin.side, pin.index, pin.count, pin.parent->ID);
}

struct NodeLink{
    NPin const* start; // outputs
    NPin const* end;   // inputs

    NodeLink(NPin const* s, NPin const* e):
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
struct LinkDragDropState {
    ImVec2      start_point;                // Starting point of the drawing
    bool        should_draw_path = false;   // Should draw the pending path
    bool        release          = false;   // Is ready to be released
    bool        is_hovering      = false;   // Did we hover over something NOW
                                            // if not we cancel the linking
    NPin const* start = nullptr;  // Output pin
    NPin const* end   = nullptr;  // Input pin

    void start_drag(){
        is_hovering = false;
    }

    void set_starting_point(ImVec2 pos, NPin const* s){
        if (s != nullptr) {
            should_draw_path = true;
            start_point = pos;
            start = s;
            release = false;
            debug("Starting new link from ({}, {})", s->parent->ID, s->index);
        }
    }

    void set_end_point(NPin const* e){
        if (should_draw_path && e->parent != start->parent){
            if (end != nullptr && end->ID != e->ID){
                debug("End new link to ({}, {}) {}", e->parent->ID, e->index, release);
            }

            end = e;
            release = true;
            is_hovering = true;
        }
    }

    void reset(){
        start_point = ImVec2(-1, -1);
        should_draw_path = false;
        release = false;
        start = nullptr;
        end = nullptr;
        debug("Reset dragndrop");
    }

    void make_new_link(NodeEditor* editor);

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


// Building brush
struct Brush {
    int       building = 0;
    int       recipe   = -1;
    int       rotation = 0;

    std::vector<const char*> building_names;
    std::vector<const char*> recipe_names;

    Building* building_descriptor(){
        if (building < 0)
            return nullptr;
        return &Resources::instance().buildings[std::size_t(building)];
    }

    Recipe* recipe_descriptor(){
        if (recipe < 0)
            return nullptr;

        Building* building = building_descriptor();
        if (!building)
            return nullptr;

        return &building->recipes[std::size_t(recipe)];
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

    Brush brush;

    ImVec2 scrolling = ImVec2(0.0f, 0.0f);
    std::vector<NodePtr>  nodes;

private:
    std::list<NodeLink> links;
    std::unordered_map<std::size_t, NodeLink*> lookup;

public:
    bool inited = false;
    bool show_grid = true;
    bool opened = true;

    // Draw a list of nodes on the left side
    bool open_context_menu = false;

    // Node Selection
    Node* node_selected         = nullptr;
    Node* node_hovered_in_list  = nullptr;
    Node* node_hovered_in_scene = nullptr;
    Node* selected_node         = nullptr;
    NodeLink* selected_link     = nullptr;
    // Link selection has precedence over node selection
    // but node selection happens after so we use this flag to guard
    // overriding the selection
    bool link_selected          = false;

    LinkDragDropState link_builder;

    const float NODE_SLOT_RADIUS     = 1.0f * Node::scaling;
    const ImVec2 NODE_WINDOW_PADDING = {10.0f, 10.0f};

    // check if a pin is connected only once
    // if not remove it and make the new connection
    void remove_pin_link(NPin const* p){
        auto iter = lookup.find(p->ID);
        if (iter != lookup.end()){
            remove_link(iter->second);
        }
    }

    NodeLink* new_link(NPin const* s, NPin const* e){
        remove_pin_link(e);
        remove_pin_link(s);

        links.emplace_back(s, e);
        auto* link = &(*links.rbegin());
        lookup[s->ID] = link;
        lookup[e->ID] = link;

        select_link(link);
        return link;
    }

    void select_link(NPin const* p){
        auto result = lookup.find(p->ID);
        if (result != lookup.end()){
            select_link((*result).second);
        }
    }

    void select_link(NodeLink* link){
        selected_link = link;
        selected_node = nullptr;
        link_selected = true;
    }

    void select_node(Node* node){
        if (!link_selected){
            selected_node = node;
            selected_link = nullptr;
            debug("node selected");
        }
    }

    void remove_link(NodeLink* link){
        debug("Removing link");
        if (link == nullptr){
            return;
        }

        if (link == selected_link){
            selected_link = nullptr;
        }

        debug("{}", link->start->ID);
        debug("{}", link->end->ID);

        lookup.erase(link->start->ID);
        lookup.erase(link->end->ID);
        links.remove(*link);
    }

    void remove_node(Node* node){
        // remove all pins
        for(auto& side: node->pins){
            for(auto& pin: side){
                remove_pin_link(&pin);
            }
        }

        // remove node from the vector
        for(auto i = 0u; i < nodes.size(); ++i){
            if (nodes[i].get() == node){
                nodes.erase(nodes.begin() + i);
                return;
            }
        }
    }
    // returns nodes that have no parents
    std::vector<Node*> roots(){
        std::vector<Node*> root_nodes;
        for (auto& node: nodes){

        }

        return root_nodes;
    }

    void draw_pin(ImDrawList* draw_list, ImVec2 offset, NPin const& pin) {
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

    void draw_node(Node* node, ImDrawList* draw_list, ImVec2 offset){
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
            for(NPin const& pin: side){
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

    void reset(){
        node_hovered_in_scene = nullptr;
        link_selected = false;
    }

    void draw_workspace(){
        reset();

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
            // Foundation is 8 x 8
            float GRID_SZ = 8.f * Node::scaling;
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

        for(auto iter = links.begin(); iter != links.end(); ++iter){
            NodeLink* link = &*iter;

            auto p1 = offset + link->start->position();
            auto p2 = offset + link->end->position();

            auto color = IM_COL32(200, 200, 100, 200);

            if (link->start->belt_type == 'P'){
                color = IM_COL32(168, 123, 50, 200);
            }

            if (link == selected_link){
                color = color | Uint32(255 << IM_COL32_A_SHIFT);
            }

            draw_bezier(
                draw_list,
                p1,
                p2,
                color
            );
        }

        // Display nodes
        link_builder.start_drag();

        for (auto& node: nodes){
            draw_node(node.get(), draw_list, offset);
        }

        link_builder.draw_path(draw_list);
        link_builder.make_new_link(this);

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
                select_node(node_hovered_in_scene);
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
                ImGui::Text("Node '%lu'", node->ID);
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
        } /*else {
            if (size.x + size.y > 0){
                offset = (size - img_size) * ImVec2(
                             0.5f * float(size.x != 0),
                             0.5f * float(size.y != 0));
            }

            ImGui::SetCursorScreenPos(pos + offset);
            ImGui::Button("Missing Image", img_size);
        }*/
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

        if (ImGui::CollapsingHeader("Brush", open)){
            draw_brush();
        }

        if (ImGui::CollapsingHeader("Selected Building", open)){
            draw_selected_info();
        }

        if (ImGui::CollapsingHeader("Entity List", open)){
            draw_node_list();
        }

        ImGui::End();
    }

    void draw(){
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

#endif
