#include "application/application.h"
#include "application/vertex.h"
#include "node-editor.h"

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


    {
        app.editor.brush.building = 0;
    }

    {
        auto new_node = std::make_shared<Node>(0, ImVec2( 40,  50));
        new_node->recipe_idx = 0;
        app.editor.nodes.push_back(new_node);
    }

    {
        auto n1 = std::make_shared<Node>(0, ImVec2( 40, 150));
        n1->recipe_idx = 0;
        app.editor.nodes.push_back(n1);
    }

    {
        auto n2 = std::make_shared<Node>(1, ImVec2(270,  80));
        n2->recipe_idx = 0;
        app.editor.nodes.push_back(n2);
    }

    {
        auto n2 = std::make_shared<Node>(2, ImVec2(370,  100));
        n2->recipe_idx = 0;
        app.editor.nodes.push_back(n2);
    }

    {
        auto n2 = std::make_shared<Node>(3, ImVec2(470,  120));
        n2->recipe_idx = 0;
        app.editor.nodes.push_back(n2);
    }

    {
        auto n2 = std::make_shared<Node>(4, ImVec2(570,  140));
        n2->recipe_idx = 0;
        app.editor.nodes.push_back(n2);
    }


    {
        auto n2 = std::make_shared<Node>(5, ImVec2(570,  160));
        n2->recipe_idx = 0;
        app.editor.nodes.push_back(n2);
    }

    app.editor.links.push_back(NodeLink(0, 0, 2, 0));
    app.editor.links.push_back(NodeLink(1, 0, 2, 1));
    app.editor.inited = true;

    try {
        app.run();
    } catch (const std::exception&) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
