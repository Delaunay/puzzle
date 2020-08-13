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


struct Node {
    std::size_t ID;
    ImVec2  Pos;
    ImVec2  _size;
    float   Value;
    ImVec4  Color;

    std::vector<NPin> inputs;
    std::vector<NPin> outputs;
    Building&         descriptor;
    int               recipe_idx = -1;
    static float constexpr  scaling = 10.f;

    Recipe* recipe(){
        if (recipe_idx > -1 && recipe_idx < int(descriptor.recipes.size())){
            return &descriptor.recipes[std::size_t(recipe_idx)];
        }
        return nullptr;
    }

    Node(Building& descriptor, const ImVec2& pos):
        ID(next_id<std::size_t>()), descriptor(descriptor)
    {
        Pos = pos;
        inputs = std::vector<NPin>(descriptor.inputs.size());
        outputs = std::vector<NPin>(descriptor.outputs.size());
    }

    void update_size(ImVec2 size){
        ImVec2 base = {scaling * descriptor.l, scaling *descriptor.w};
        _size = base;
    }

    ImVec2 size() const {
        return _size;
    }

    ImVec2 GetInputSlotPos(std::size_t slot_no) const {
        return ImVec2(
            Pos.x,
            Pos.y + size().y * (float(slot_no + 1)) / (float(inputs.size() + 1)));
    }

    ImVec2 GetOutputSlotPos(std::size_t slot_no) const {
        return ImVec2(
            Pos.x + size().x,
            Pos.y + size().y * (float(slot_no + 1)) / (float(outputs.size() + 1)));
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
        Building* desc = nullptr;
    };

    Brush brush;

    std::vector<NodePtr>  nodes;
    std::vector<NodeLink> links;
    ImVec2 scrolling = ImVec2(0.0f, 0.0f);

    bool inited = false;
    bool show_grid = true;
    int  node_selected = -1;
    bool opened = true;

    // Draw a list of nodes on the left side
    bool open_context_menu = false;
    int node_hovered_in_list = -1;
    int node_hovered_in_scene = -1;

    const float NODE_SLOT_RADIUS = 8.0f;
    const ImVec2 NODE_WINDOW_PADDING = {8.0f, 8.0f};

    bool        pending_path   = false;
    ImVec2      pending_path_start;
    bool        pending_release = false;

    std::size_t pending_pin_id = 0;
    const NPin* pending_pin    = nullptr;
    const Node* pending_node   = nullptr;
    std::size_t pending_node_idx;
    std::size_t pending_slot_idx;
    bool        pending_pin_type = false;
    //
    std::size_t pending_pin_id_e = 0;
    const NPin* pending_pin_e    = nullptr;
    const Node* pending_node_e   = nullptr;
    std::size_t pending_node_idx_e;
    std::size_t pending_slot_idx_e;
    bool        pending_pin_type_e = false;

    void draw_pin(std::size_t slot_idx, Node const* node, NPin const& pin, ImDrawList* draw_list, ImVec2 offset, bool is_input, std::size_t node_idx){
        char type;
        if (is_input) {
            type = node->descriptor.inputs[slot_idx][0];
        } else {
            type = node->descriptor.outputs[slot_idx][0];
        }

        if (type != 'C' && type != 'P'){
            return;
        }

        ImGui::PushID(int(pin.ID));

        ImVec2 center;
        ImU32 color;

        if (is_input){
            center = offset + node->GetInputSlotPos(slot_idx);
            color = IM_COL32(255, 179, 119, 255);
        } else{
            center = offset + node->GetOutputSlotPos(slot_idx);
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
            pending_path_start = center;
            pending_path = true;
            pending_pin_id = pin.ID;
            pending_pin = &pin;
            pending_node = node;
            pending_node_idx = node_idx;
            pending_slot_idx = slot_idx;
            pending_pin_type = is_input;

            debug("Starting new link from ({}, {})", pending_node->ID, pending_slot_idx);
        }

        // Set Endingpoint
        if (pending_path && hovered && pending_pin_id != pin.ID) {
            if (pin.ID != pending_pin_id_e){
                pending_release = true;
                pending_pin_id_e = pin.ID;
                pending_pin_e = &pin;
                pending_node_e = node;
                pending_node_idx_e = node_idx;
                pending_slot_idx_e = slot_idx;
                pending_pin_type_e = is_input;

                debug("End new link to ({}, {})", pending_node_e->ID, pending_slot_idx_e);
            }
        } else if (!hovered && pin.ID == pending_pin_id_e) {
            pending_release = false;
        }

        ImGui::PopID();
    }

    void draw_node_list(){
        ImGui::BeginChild("node_list", ImVec2(100, 0));
        ImGui::Text("Nodes");
        ImGui::Separator();

        for (std::size_t node_idx = 0; node_idx < nodes.size(); node_idx++){
            Node* node = get_node(node_idx);
            ImGui::PushID(int(node->ID));

            if (ImGui::Selectable(node->descriptor.name.c_str(), node->ID == node_selected)){
                node_selected = node->ID;
            }

            if (ImGui::IsItemHovered()){
                node_hovered_in_list = node->ID;
                open_context_menu |= ImGui::IsMouseClicked(1);
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::SameLine();
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

        ImGui::Text("%s", node->descriptor.name.c_str());
        auto text_size = ImGui::GetItemRectSize();

        auto recipe = node->recipe();
        if (recipe != nullptr){
            if (recipe->texture.size() >  0){
                auto cpu_img = Resources::instance().load_texture(recipe->texture);
                auto device_img = app.load_texture_image(*cpu_img);

                if (device_img->descriptor_set == nullptr){
                    device_img->descriptor_set = ImGui_ImplVulkan_AddTexture(
                        device_img->sampler,
                        device_img->view,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                }

                auto img_size = ImVec2(cpu_img->x * 0.20f, cpu_img->y * 0.20f);

                auto pos = ImGui::GetCursorPos();
                auto remaining_size = node->size() - ImVec2(0, text_size.y + 20);
                auto offset = (remaining_size - img_size) * 0.5f;

                ImGui::SetCursorPos(pos + offset);
                ImGui::Image(
                    device_img->descriptor_set,
                    img_size,
                    ImVec2(0.0f, 0.0f),
                    ImVec2(1.0f, 1.0f),
                    ImVec4(1, 1, 1, 1),
                    ImVec4(1, 1, 1, 0.25));
            }
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
            node_hovered_in_scene = node->ID;
            open_context_menu |= ImGui::IsMouseClicked(1);
        }

        bool node_moving_active = ImGui::IsItemActive();

        if (node_widgets_active || node_moving_active)
            node_selected = node->ID;

        if (node_moving_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            node->Pos = node->Pos + io.MouseDelta;

        ImU32 node_bg_color = IM_COL32(60, 60, 60, 255);
        if (node_hovered_in_list == node->ID || node_hovered_in_scene == node->ID || (node_hovered_in_list == -1 && node_selected == node->ID))
            node_bg_color = IM_COL32(75, 75, 75, 255);

        draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f);
        draw_list->AddRect(node_rect_min, node_rect_max, IM_COL32(100, 100, 100, 255), 4.0f);


        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (pending_release){
                if (pending_pin_type != pending_pin_type_e){
                    // if it is not an input
                    if (pending_pin_type) {
                        std::swap(pending_node_idx, pending_node_idx_e);
                        std::swap(pending_slot_idx, pending_slot_idx_e);
                    }
                    debug("Make Connection {} -> {}", pending_node->ID, node->ID);
                    links.push_back(NodeLink(pending_node_idx, pending_slot_idx, pending_node_idx_e, pending_slot_idx_e));
                } else {
                    debug("Cannot connect input to input!");
                }

                pending_pin_id_e = -1;
                pending_release = false;
            }

            pending_path = false;
            pending_pin_id = -1;
            pending_node = nullptr;
        }

        for (std::size_t slot_idx = 0; slot_idx < node->inputs.size(); slot_idx++){
            NPin& pin = node->inputs[slot_idx];
            draw_pin(slot_idx, node, pin, draw_list, offset, true, node_idx);
        }

        for (std::size_t slot_idx = 0; slot_idx < node->outputs.size(); slot_idx++){
            NPin& pin = node->outputs[slot_idx];
            draw_pin(slot_idx, node, pin, draw_list, offset, false, node_idx);
        }

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
            float GRID_SZ = 64.0f;
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

            ImVec2 p1 = offset + node_inp->GetOutputSlotPos(link->InputSlot);
            ImVec2 p2 = offset + node_out->GetInputSlotPos(link->OutputSlot);

            draw_list->AddBezierCurve(p1, p1 + ImVec2(+50, 0), p2 + ImVec2(-50, 0), p2, IM_COL32(200, 200, 100, 255), 3.0f);
        }

        // Display nodes
        for (std::size_t node_idx = 0; node_idx < nodes.size(); node_idx++){
            draw_node(node_idx, get_node(node_idx), draw_list, offset);
        }
        draw_list->ChannelsMerge();

        // Open context menu
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)){
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) || !ImGui::IsAnyItemHovered()){
                node_selected = node_hovered_in_list = node_hovered_in_scene = -1;
                open_context_menu = true;
            }
        }

        if (open_context_menu) {
            ImGui::OpenPopup("context_menu");
            if (node_hovered_in_list != -1)
                node_selected = node_hovered_in_list;

            if (node_hovered_in_scene != -1)
                node_selected = node_hovered_in_scene;
        }

        // Draw context menu
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        if (ImGui::BeginPopup("context_menu"))
        {
            Node* node = nullptr;
            if (node_selected >= 0 && node_selected < nodes.size()){
                node = get_node(node_selected);
            }

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
                    if (brush.desc != nullptr){
                        debug("New node");
                        nodes.push_back(std::make_shared<Node>(*brush.desc, scene_pos));
                    }
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

        if (pending_path){
            ImVec2 p1 = pending_path_start;
            ImVec2 p2 = ImGui::GetMousePos();

            if (p1.x > p2.x) {
                std::swap(p1, p2);
            }

            draw_list->AddBezierCurve(
                p1,
                p1 + ImVec2(+50, 0),
                p2 + ImVec2(-50, 0),
                p2,
                IM_COL32(200, 200, 100, 255),
                3.0f);
        }

        ImGui::PopItemWidth();
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::EndGroup();
    }

    void draw(){
        ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Example: Custom Node Graph", &opened))
        {
            ImGui::End();
            return;
        }

        draw_node_list();
        draw_workspace();
        ImGui::End();
    }
};

#endif
