#ifndef PUZZLE_EDITOR_HEADER
#define PUZZLE_EDITOR_HEADER

#include <sstream>

#include "config.h"

#include "application/application.h"
#include "application/utils.h"

#include "brush.h"
#include "node.h"
#include "link.h"

// Draw a bezier curve using only 2 points
// derive the two other points needed from the starting points
// they derived points change in function of their relative position
void draw_bezier(ImDrawList* draw_list, ImVec2 p1, ImVec2 p2, ImU32 color);

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
    void remove_pin_link(Pin const* p){
        auto iter = lookup.find(p->ID);
        if (iter != lookup.end()){
            remove_link(iter->second);
        }
    }

    NodeLink* new_link(Pin const* s, Pin const* e){
        remove_pin_link(e);
        remove_pin_link(s);

        links.emplace_back(s, e);
        auto* link = &(*links.rbegin());
        lookup[s->ID] = link;
        lookup[e->ID] = link;

        select_link(link);
        return link;
    }

    void select_link(Pin const* p){
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

    void draw_pin(ImDrawList* draw_list, ImVec2 offset, Pin const& pin) {
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


#endif
