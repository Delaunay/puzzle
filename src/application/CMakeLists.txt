FILE(GLOB APPLICATION_SRC *.cpp *.h)

ADD_LIBRARY(vkApplication ${APPLICATION_SRC})
TARGET_LINK_LIBRARIES(vkApplication spdlog::spdlog SDL2 Vulkan::Vulkan)
