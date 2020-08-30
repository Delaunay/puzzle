#include "node-editor.h"

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

ImVec4 operator* (float v, ImVec4 f){
    return ImVec4(f.x * v, f.y * v, f.z * v, f.w * v);
}

ImVec4 operator* (ImVec4 f, float v){
    return ImVec4(f.x * v, f.y * v, f.z * v, f.w * v);
}


void draw_efficiency(float efficiency, const char* label){
    // Show the overall production efficiency as a progress bar
    // this also nicely break with the table above so the table below
    // will be more readable
    auto green = ImVec4(64.f / 255.f, 235.f / 255.f, 52.f / 255.f, efficiency);
    auto red = ImVec4(235.f / 255.f, 79.f / 255.f, 52.f / 255.f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_FrameBg      , red);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, green);
    ImGui::PushStyleColor(ImGuiCol_Text         , ImVec4(0.25f, 0.25f, 0.25f, 1.f));
    ImGui::ProgressBar(efficiency, ImVec2(-1, 40), label);
    ImGui::PopStyleColor(3);

}


float compute_efficiency(ProductionBook const& prod, bool include_prod){
    float node_efficiency = 1;
    for (auto& item: prod){
        if (include_prod && item.second.produced > 0){
            //  2.5 / 5 =  50%
            // 10   / 5 = 100%
            auto production_efficiency = std::min(item.second.consumed / item.second.produced, 1.f);
            node_efficiency = std::min(node_efficiency, production_efficiency);
        }

        if (item.second.received > 0){
            // we are receiving an item we do not need
            if (item.second.consumed <= 0)
                node_efficiency = 0.f;

            else{
                auto consumption_efficiency = std::min(item.second.received / item.second.consumed, 1.f);
                node_efficiency = std::min(node_efficiency, consumption_efficiency);
            }
        }
    }
    return node_efficiency;
}

float NodeEditor::compute_overal_efficiency(){
    float efficiency = 0;
    float count      = 0;

    for(auto& node: graph.iter_nodes()){
        // Splitter have no efficiency
        if (node.descriptor && node.descriptor->recipe_names().size() > 0){
            efficiency += node.efficiency;
            count += 1;
        }
    }

    return efficiency / count;
}


void draw_book(const char* id, ProductionBook const& prod, bool prod_only=false){
    // Production Breakdown
    int cols = 5;
    if (prod_only){
        cols = 2;
    }

    ImGui::Separator();
    ImGui::Columns(cols, id, true);
    ImGui::Text("Item");
    ImGui::Spacing();
    for(auto& item: prod){
        ImGui::Text("%s", item.first.c_str());
    }

    ImGui::NextColumn();
    ImGui::Text("Produced");
    if (!prod_only)
        ImGui::Spacing();
    else
        ImGui::Separator();

    for(auto& item: prod){
        ImGui::Text("%5.2f", item.second.produced);
    }

    if (!prod_only){
        ImGui::NextColumn();
        ImGui::Text("Consumed");
        ImGui::Spacing();
        for(auto& item: prod){
            ImGui::Text("%5.2f", item.second.consumed);
        }

        ImGui::NextColumn();
        ImGui::Text("Received");
        ImGui::Spacing();
        for(auto& item: prod){
            ImGui::Text("%5.2f", item.second.received);
        }

        ImGui::NextColumn();
        ImGui::Text("Overflow");
        ImGui::Separator();
        for(auto& item: prod){
            ImGui::Text("%5.2f", item.second.overflow);
        }
    }

    ImGui::Separator();
    ImGui::Columns(1);
}

void NodeEditor::draw_overall_performance(){
    ImGui::Begin("Performance");

    // Energy Consumption
    auto e = graph.compute_electricity();
    auto energy_label = fmt::format("Energy ({:6.2f} MW)", -1.f * e.consumed);
    draw_efficiency(-1.f * e.produced / e.consumed, energy_label.c_str());

    // Raw material use (lowest tier item)
    ProductionBook low_tier;
    for(auto& leaf: graph.roots()){
        ProductionBook& prod = prod_stats[leaf->ID];
        float eff = leaf->efficiency; // compute_efficiency(prod, false);

        for(auto& item: prod){
            if (item.second.produced <= 0)
                continue;

            low_tier[item.first].consumed += item.second.consumed * eff;
            low_tier[item.first].produced += item.second.produced * eff;
            low_tier[item.first].received += item.second.received * eff;
        }
    }

    ImGui::Text("Raw Material Usage");
    draw_book("Raw Material", low_tier, true);

    // Production Efficiency
    float efficiency = compute_overal_efficiency();
    auto label = fmt::format("Overall Efficiency ({:6.2f} %)", efficiency * 100.f);
    draw_efficiency(efficiency, label.c_str());

    // Roots Productions (highest tier item)
    ProductionBook high_tier;
    for(auto& leaf: graph.leaves()){
        ProductionBook& prod = prod_stats[leaf->ID];
        float eff = leaf->efficiency; // compute_efficiency(prod, false);

        for(auto& item: prod){
            if (item.second.produced <= 0)
                continue;

            high_tier[item.first].consumed += item.second.consumed * eff;
            high_tier[item.first].produced += item.second.produced * eff;
            high_tier[item.first].received += item.second.received * eff;
        }
    }

    ImGui::Text("Production Available");
    draw_book("Production", high_tier, true);

    // Space
    // TODO
    // --

    ImGui::End();
};

void NodeEditor::draw_production(ProductionBook const& prod, float efficiency){
    auto open = ImGuiTreeNodeFlags_DefaultOpen;

    if (ImGui::TreeNode("Production", open)){
        if (efficiency > -1.f){
            auto label = fmt::format("Efficiency ({:6.2f})", efficiency * 100.f);
            draw_efficiency(efficiency, label.c_str());
        }

        draw_book("link-prod-cols", prod);
        ImGui::TreePop();
    }
}

void NodeEditor::draw_selected_info(){
    ImGui::TreePush("selected-info");

    if (selected_link != nullptr){
        draw_production(selected_link->production, -1.f);
        ImGui::TreePop();
        return;
    }

    if (selected_node == nullptr){
        ImGui::TreePop();
        return;
    }

    Building* b = selected_node->descriptor;
    auto open = ImGuiTreeNodeFlags_DefaultOpen;

    if (b && ImGui::TreeNode("Building Spec", open)){
        ImGui::Columns(2, "spec-cols", true);
        ImGui::Separator();

        ImGui::Text("Name");
        ImGui::Text("Energy");
        ImGui::Text("Dimension");
        ImGui::Text("ID");

        ImGui::NextColumn();

        ImGui::Text("%s", b->name.c_str());
        ImGui::Text("%.2f", b->energy);
        ImGui::Text("%.0f x %.0f", b->w, b->l);
        ImGui::Text("%lu", selected_node->ID);

        ImGui::Separator();
        ImGui::Columns(1);
        ImGui::TreePop();
    }
    // Building Specs


    if (b && ImGui::TreeNode("Recipe", open)){
        // Select a new recipe
        available_recipes = &b->recipe_names();

        if (ImGui::Combo(
            "Recipe",
            &selected_node->recipe_idx,
            available_recipes->data(),
            available_recipes->size())){
            need_recompute_prod = true;
            selected_node->production = ProductionBook();
        }

        // display selected recipe
        auto selected_recipe = selected_node->recipe();
        draw_recipe_icon(selected_recipe, ImVec2(0.3f, 0.3f), ImVec2(ImGui::GetWindowWidth(), 0));

        if (selected_recipe != nullptr){
            if (selected_recipe->inputs.size() > 0){
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
            }

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
        ImGui::TreePop();
    }
    // Recipe Stop
    draw_production(selected_node->production, selected_node->efficiency);

    ImGui::TreePop();
}

