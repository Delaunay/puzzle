#include "application/application.h"
#include "application/vertex.h"
#include "editor/node-editor.h"

#include <memory>

static void ShowExampleAppCustomNodeGraph(bool* opened);

namespace sh {
    struct Window {
        Window(const char* name, bool* open = nullptr, ImGuiWindowFlags flags = 0){
            ImGui::Begin(name, open, flags);
        }

        ~Window(){
            ImGui::End();
        }
    };
}

#undef IM_COL32
#define IM_COL32(R,G,B,A)    ((ImU32(A)<<IM_COL32_A_SHIFT) | (ImU32(B)<<IM_COL32_B_SHIFT) | (ImU32(G)<<IM_COL32_G_SHIFT) | (ImU32(R)<<IM_COL32_R_SHIFT))


class MyGame: public puzzle::Application {
public:
    NodeEditor editor;

    MyGame():
        Application(1980, 1080), editor(*this)
    {}

    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };

    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0
    };

    void handle_event(SDL_Event &) override {}

    void render_frame(VkCommandBuffer cmd) override {
        // Draw Triangle
        VkBuffer vertexBuffers[] = {vertex_buffer};
        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmd, index_buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(cmd, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        // ---
    }

    bool opened = true;

    void imgui_draw() override {
        editor.draw();
        ImGui::ShowDemoWindow(&opened);
    }
};


#include "config.h"

int main() {
    auto& resources = Resources::instance();
    resources.load_configs();

    MyGame app;

    assert(resources.buildings.size() > 0);
    int miner  = resources.find_building("Miner");
    int iron_ore = resources.find_recipe(miner, "Iron Ore");

    int smelter  = resources.find_building("Smelter");
    int iron_ingot = resources.find_recipe(smelter, "Iron Ingot");

    int constructor  = resources.find_building("Constructor");
    int iron_plate = resources.find_recipe(constructor, "Iron Plate");

    assertf(miner >= 0, "miner should be found");
    assertf(iron_ore >= 0, "iron ore should be found");
    assertf(smelter >= 0, "smelter should be found");
    assertf(iron_ingot >= 0, "iron ingot");
    assertf(constructor >= 0, "constructor");
    assertf(iron_plate >= 0, "iron plate");

    Node* n0 = app.editor.new_node(ImVec2(40 ,  50), miner, iron_ore);
    Node* n1 = app.editor.new_node(ImVec2(240 , 50), smelter, iron_ingot);
    Node* n2 = app.editor.new_node(ImVec2(440,  50), constructor, iron_plate);

    // Ore to smelter
    {
        auto outpin = &n0->pins[RightToLeft][0];
        auto inpin  = &n1->pins[LeftToRight][0];
        app.editor.new_link(outpin, inpin);
    }

    // Ingot to constructor
    {
        auto outpin = &n1->pins[RightToLeft][0];
        auto inpin  = &n2->pins[LeftToRight][0];
        app.editor.new_link(outpin, inpin);
    }


    /*
    Node* n0 = app.editor.new_node(ImVec2(40 ,  50), 0, 0);
    Node* n1 = app.editor.new_node(ImVec2(40 , 150), 0, 0);
    Node* n2 = app.editor.new_node(ImVec2(270,  80), 1, 0);

               app.editor.new_node(ImVec2(370, 100), 2, 0);
               app.editor.new_node(ImVec2(470, 120), 3, 0);
               app.editor.new_node(ImVec2(570, 140), 4, 0);
               app.editor.new_node(ImVec2(570, 160), 5, 0);

    {
        auto outpin = &n0->pins[RightToLeft][0];
        auto inpin  = &n2->pins[LeftToRight][0];
        app.editor.new_link(outpin, inpin);
    }

    {
        auto outpin = &n1->pins[RightToLeft][0];
        auto inpin  = &n2->pins[LeftToRight][1];
        app.editor.new_link(outpin, inpin);
    }*/
    app.editor.inited = true;

//    try {
        app.run();
//    } catch (const std::exception& error) {
//        debug("{}", error.what());
//        return EXIT_FAILURE;
//    }

    return EXIT_SUCCESS;
}
