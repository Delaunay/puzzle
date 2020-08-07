#ifndef PUZZLE_APP_UNIFORM_BUFFER_HEADER
#define PUZZLE_APP_UNIFORM_BUFFER_HEADER

#include <glm/glm.hpp>


struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

#endif
