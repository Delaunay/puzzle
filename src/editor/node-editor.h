#ifndef PUZZLE_EDITOR_HEADER
#define PUZZLE_EDITOR_HEADER

#include <sstream>

#include "config.h"

#include "application/application.h"
#include "application/utils.h"

#include "brush.h"
#include "node.h"
#include "link.h"
#include "forest.h"

// Draw a bezier curve using only 2 points
// derive the two other points needed from the starting points
// they derived points change in function of their relative position
void draw_bezier(ImDrawList* draw_list, ImVec2 p1, ImVec2 p2, ImU32 color);

// Foundation is 8 x 8 and we scale by 10
// float GRID_SZ = 8.f * Node::scaling;
// Grid Size: 80x80
inline
ImVec2 snap(ImVec2 pos, float snap_factor = 20.f){
    return ImVec2(std::round(pos.x / snap_factor),
                  std::round(pos.y / snap_factor)) * snap_factor;
}


// R: Rotate
// Q: Copy building to brush
//
// TODO:
//  - DEL : rm selected
//  - WASD: move scroll area

struct NodeEditor{
    puzzle::Application& app;
    Forest               graph;
    Brush                brush;
    ImVec2               scrolling = ImVec2(0.0f, 0.0f);
    LinkDragDropState    link_builder;

    NodeEditor(puzzle::Application& app):
        app(app)
    {
        inited = true;
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
    Node* selected_node         = nullptr;
    NodeLink* selected_link     = nullptr;
    // Link selection has precedence over node selection
    // but node selection happens after so we use this flag to guard
    // overriding the selection
    bool link_selected          = false;

    const float  NODE_SLOT_RADIUS     = 1.0f * Node::scaling;
    const ImVec2 NODE_WINDOW_PADDING = {10.0f, 10.0f};

    // Forest functionality forwarding for a nicer API
    NodeLink* new_link   (Pin const* s, Pin const* e){  return graph.new_link(s, e);     }
    void      remove_link(NodeLink* link)            {  return graph.remove_link(link);  }
    void      remove_node(Node* node)                {  return graph.remove_node(node);  }
    Node*     new_node   (ImVec2 pos, int building, int recipe, int rotation = 0){
        return graph.new_node(pos, building, recipe, rotation);
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

    void select_link(Pin const* p){
        auto result = graph.find_link(p);
        select_link(result);
    }

    // returns nodes that have no parents
    std::vector<Node*> roots(){
        std::vector<Node*> root_nodes;
        for (auto& node: graph.iter_nodes()){

        }

        return root_nodes;
    }

    void draw_pin(ImDrawList* draw_list, ImVec2 offset, Pin const& pin);
    
    void draw_node(Node* node, ImDrawList* draw_list, ImVec2 offset);

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

        for(auto& iter: graph.iter_links()){
            NodeLink* link = &iter;

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

        for (auto& node: graph.iter_nodes()){
            draw_node(&node, draw_list, offset);
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

        if (ImGui::IsKeyReleased(SDL_SCANCODE_Q) && selected_node != nullptr){
            brush.set(selected_node->building, selected_node->recipe_idx, selected_node->rotation);
        }

        if (ImGui::IsKeyReleased(SDL_SCANCODE_DELETE)){
            if (selected_node != nullptr){
                remove_node(selected_node);
            }

            if (selected_link != nullptr){
                remove_link(selected_link);
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
                    graph.new_node(snap(scene_pos), brush.building, brush.recipe, brush.rotation % 4);
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

        // for (std::size_t node_idx = 0; node_idx < nodes.size(); node_idx++){
        for (auto& iter: graph.iter_nodes()){
            Node* node = &iter;
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

    void show_production_stat(){
        auto data = graph.compute_production();

        debug("    {:>20}: {:>10} {:>10} {:>10}", "", "consumed", "produced", "received");
        for(auto& entity: data){
            debug("{}:", entity.first);
            for(auto& item: entity.second){
                debug("    {:>20}: {:10.2f} {:10.2f} {:10.2f}", item.first, item.second.consumed, item.second.produced, item.second.received);
            }
        }

        debug("Bilan");
        debug("{:>20}: {:>10} | {:>10} | {:>10}",
              "resource", "produced", "consumed","remaining");

        auto bilan = graph.production_statement();
        for(auto& item: bilan){
            debug("{:>20}: {:10.2f} | {:10.2f} | {:10.2f}",
                  item.first,
                  item.second.produced,
                  item.second.consumed,
                  item.second.produced - item.second.consumed
                  );
        }
    }

    void draw_tool_panel(){
        ImGui::Begin("Tool box");
        auto open = ImGuiTreeNodeFlags_DefaultOpen;

        if (ImGui::Button("Show Stats")){
            show_production_stat();
        }

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
