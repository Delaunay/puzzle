#ifndef PUZZLE_APP_SHAPES_HEADER
#define PUZZLE_APP_SHAPES_HEADER

#include <array>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription binding_description() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> attribute_description() {
        /*
            float: VK_FORMAT_R32_SFLOAT
            vec2 : VK_FORMAT_R32G32_SFLOAT
            vec3 : VK_FORMAT_R32G32B32_SFLOAT
            vec4 : VK_FORMAT_R32G32B32A32_SFLOAT
        */

        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        return attributeDescriptions;
    }
};


#endif
