#include "application.h"

// #define GLFW_INCLUDE_VULKAN
// #include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_vulkan.h"

#include "logger.h"
#include "sys.h"
#include "vertex.h"

#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <optional>
#include <set>
#include <thread>

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};


#include <stb_image.h>

Image::Image(std::string const& path){
    data = stbi_load(path.c_str(), &x, &y, &c, c);

    if (!data){
        RAISE(std::runtime_error, "failed to load texture image! " + path);
    }
    debug("Image {}x{}x{} loaded", x, y, c);
}

Image::~Image(){
    stbi_image_free(data);
    data = nullptr;
}


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define VULKAN_ERROR_CODES(X)\
    X(VK_SUCCESS, 0)\
    X(VK_NOT_READY, 1)\
    X(VK_TIMEOUT, 2)\
    X(VK_EVENT_SET, 3)\
    X(VK_EVENT_RESET, 4)\
    X(VK_INCOMPLETE, 5)\
    X(VK_ERROR_OUT_OF_HOST_MEMORY, -1)\
    X(VK_ERROR_OUT_OF_DEVICE_MEMORY, -2)\
    X(VK_ERROR_INITIALIZATION_FAILED, -3)\
    X(VK_ERROR_DEVICE_LOST, -4)\
    X(VK_ERROR_MEMORY_MAP_FAILED, -5)\
    X(VK_ERROR_LAYER_NOT_PRESENT, -6)\
    X(VK_ERROR_EXTENSION_NOT_PRESENT, -7)\
    X(VK_ERROR_FEATURE_NOT_PRESENT, -8)\
    X(VK_ERROR_INCOMPATIBLE_DRIVER, -9)\
    X(VK_ERROR_TOO_MANY_OBJECTS, -10)\
    X(VK_ERROR_FORMAT_NOT_SUPPORTED, -11)\
    X(VK_ERROR_FRAGMENTED_POOL, -12)\
    X(VK_ERROR_UNKNOWN, -13)\
    X(VK_ERROR_OUT_OF_POOL_MEMORY, -1000069000)\
    X(VK_ERROR_INVALID_EXTERNAL_HANDLE, -1000072003)\
    X(VK_ERROR_FRAGMENTATION, -1000161000)\
    X(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS, -1000257000)\
    X(VK_ERROR_SURFACE_LOST_KHR, -1000000000)\
    X(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR, -1000000001)\
    X(VK_SUBOPTIMAL_KHR, 1000001003)\
    X(VK_ERROR_OUT_OF_DATE_KHR, -1000001004)\
    X(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR, -1000003001)\
    X(VK_ERROR_VALIDATION_FAILED_EXT, -1000011001)\
    X(VK_ERROR_INVALID_SHADER_NV, -1000012000)\
    X(VK_ERROR_INCOMPATIBLE_VERSION_KHR, -1000150000)\
    X(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT, -1000158000)\
    X(VK_ERROR_NOT_PERMITTED_EXT, -1000174001)\
    X(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT, -1000255000)\
    X(VK_THREAD_IDLE_KHR, 1000268000)\
    X(VK_THREAD_DONE_KHR, 1000268001)\
    X(VK_OPERATION_DEFERRED_KHR, 1000268002)\
    X(VK_OPERATION_NOT_DEFERRED_KHR, 1000268003)\
    X(VK_PIPELINE_COMPILE_REQUIRED_EXT, 1000297000)\
    X(VK_RESULT_MAX_ENUM, 0x7FFFFFFF)

std::string to_string(VkResult err){
    switch (err){
    #define TO_STRING(name, value) case value: return #name;
        VULKAN_ERROR_CODES(TO_STRING)
    #undef TO_STRING
    }
}


VkResult create_debug_utils_messenger_EXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = PFN_vkCreateDebugUtilsMessengerEXT(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroy_debug_utils_messenger_EXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = PFN_vkDestroyDebugUtilsMessengerEXT(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}


void check_vk_result(VkResult err, sym::CodeLocation const& loc){
    if (err != VK_SUCCESS) {
        sym::log(sym::LogLevel::DEBUG, loc, "Vulkan Error: {}", to_string(err));
        throw std::runtime_error(to_string(err));
    }
}

namespace puzzle {

VkCommandBuffer Application::begin_single_time_commands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = cmd_pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void Application::end_single_time_commands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device, cmd_pool, 1, &commandBuffer);
}

void Application::copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = begin_single_time_commands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
        );

    end_single_time_commands(commandBuffer);
}

void Application::transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout){
    VkCommandBuffer commandBuffer = begin_single_time_commands();
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage,
        destinationStage,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier
    );

    end_single_time_commands(commandBuffer);
}

DeviceImage* Application::load_texture_image(Image const& img){

    DeviceImage* device_img = &texture_cache[img.data];
    if (device_img->view != VK_NULL_HANDLE){
        return device_img;
    }

    debug("load texture to device {}x{}", img.x, img.y);

    VkDeviceSize imageSize = img.x * img.y * img.c;

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    create_buffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, img.data, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    VkImage vk_img;
    VkDeviceMemory img_memory;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(img.x);
    imageInfo.extent.height = static_cast<uint32_t>(img.y);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0; // Optional

    if (vkCreateImage(device, &imageInfo, nullptr, &vk_img) != VK_SUCCESS) {
        RAISE(std::runtime_error, "failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, vk_img, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &img_memory) != VK_SUCCESS) {
        RAISE(std::runtime_error, "failed to allocate image memory!");
    }
    vkBindImageMemory(device, vk_img, img_memory, 0);

    // change to a layout more suited for the GPU
    transition_image_layout(
        vk_img,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    copy_buffer_to_image(
        stagingBuffer,
        vk_img,
        static_cast<uint32_t>(img.x),
        static_cast<uint32_t>(img.y));

    // Prepare for shader access
    transition_image_layout(
        vk_img,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    VkImageView view = create_image_view(vk_img, VK_FORMAT_R8G8B8A8_SRGB);
    VkSampler sampler = new_texture_sampler();

    DeviceImage dimg = DeviceImage{sampler, view, vk_img, img_memory};
    texture_cache[img.data] = dimg;
    return &texture_cache[img.data];
}

//void Application::create_descriptor_pool() {
//    std::array<VkDescriptorPoolSize, 2> poolSizes{};
//    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//    poolSizes[0].descriptorCount = static_cast<uint32_t>(sc_images.size());
//    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    poolSizes[1].descriptorCount = static_cast<uint32_t>(sc_images.size());

//    VkDescriptorPoolCreateInfo poolInfo{};
//    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
//    poolInfo.pPoolSizes = poolSizes.data();
//    poolInfo.maxSets = static_cast<uint32_t>(sc_images.size());

//    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptor_pool) != VK_SUCCESS) {
//        throw std::runtime_error("failed to create descriptor pool!");
//    }
//}

//void Application::new_descriptor_set(){
//    std::vector<VkDescriptorSetLayout> layouts(sc_images.size(), descriptor_set_layout);
//    VkDescriptorSetAllocateInfo allocInfo{};
//    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//    allocInfo.descriptorPool = descriptor_pool;
//    allocInfo.descriptorSetCount = static_cast<uint32_t>(sc_images.size());
//    allocInfo.pSetLayouts = layouts.data();

//    descriptorSets.resize(sc_images.size());
//    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
//        throw std::runtime_error("failed to allocate descriptor sets!");
//    }

//    VkWriteDescriptorSet descriptorWrites;

////    VkDescriptorBufferInfo bufferInfo{};
////    bufferInfo.buffer = uniformBuffers[i];
////    bufferInfo.offset = 0;
////    bufferInfo.range = sizeof(UniformBufferObject);

////    VkDescriptorImageInfo imageInfo{};
////    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
////    imageInfo.imageView = textureImageView;
////    imageInfo.sampler = textureSampler;

//    descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//    descriptorWrites.dstSet = descriptorSets[i];
//    descriptorWrites.dstBinding = 1;
//    descriptorWrites.dstArrayElement = 0;
//    descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    descriptorWrites.descriptorCount = 1;
//    descriptorWrites.pImageInfo = &imageInfo;

//    vkUpdateDescriptorSets(
//        device,
//        static_cast<uint32_t>(1),
//        &descriptorWrites,
//        0,
//        nullptr);

////    for (size_t i = 0; i < swapChainImages.size(); i++) {
////        VkDescriptorBufferInfo bufferInfo{};
////        bufferInfo.buffer = uniformBuffers[i];
////        bufferInfo.offset = 0;
////        bufferInfo.range = sizeof(UniformBufferObject);

////        VkDescriptorImageInfo imageInfo{};
////        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
////        imageInfo.imageView = textureImageView;
////        imageInfo.sampler = textureSampler;

////        ...
////    }

//}

VkSampler Application::new_texture_sampler(){
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkSampler textureSampler;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
    return textureSampler;
}

bool Application::init_window() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)){
        debug("Could not init SDL library", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow(
        "test_project",
        0,
        0,
        int(WIDTH),
        int(HEIGHT),
        // SDL_WINDOW_BORDERLESS
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);


    /*
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    */
    return true;
}


bool Application::init_imgui(){
    debug("init ImGUI");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance;
    init_info.PhysicalDevice = physical_device;
    init_info.Device = device;
    init_info.QueueFamily = find_queue_families(physical_device).graphics_family.value();
    init_info.Queue = graphics_queue;
    init_info.PipelineCache = pipeline_cache;
    init_info.DescriptorPool = descriptor_pool;
    init_info.Allocator = allocator;
    init_info.MinImageCount = uint32_t(MAX_FRAMES_IN_FLIGHT);
    init_info.ImageCount = uint32_t(MAX_FRAMES_IN_FLIGHT);
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, render_pass);

    return true;
}

void Application::init_fonts(){
    debug("loading fonts");

    // Use any command queue
    VkCommandPool command_pool = cmd_pool;
    VkCommandBuffer command_buffer = cmd_buffer[0];

    auto err = vkResetCommandPool(device, command_pool, 0);
    check_vk_result(err, LOC);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    err = vkBeginCommandBuffer(command_buffer, &begin_info);

    check_vk_result(err, LOC);
    ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

    VkSubmitInfo end_info = {};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &command_buffer;
    err = vkEndCommandBuffer(command_buffer);
    check_vk_result(err, LOC);
    err = vkQueueSubmit(graphics_queue, 1, &end_info, VK_NULL_HANDLE);
    check_vk_result(err, LOC);

    err = vkDeviceWaitIdle(device);
    check_vk_result(err, LOC);
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

/*
static void framebufferResizeCallback(SDL_Window* window, int width, int height) {
    auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}
*/

void Application::init_vulkan() {
    create_instance();
    setup_debug_messenger();
    create_surface();
    pick_physical_device();
    create_logical_device();

    create_descriptor_pool();

    create_swap_chain();
    create_image_views();
    create_render_pass();
    create_graphics_pipeline();
    create_framebuffers();
    create_command_pool();
    create_vertex_buffer();
    create_index_buffer();
    create_command_buffers();
    create_sync_objects();
}

void Application::create_index_buffer() {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    create_buffer
        (bufferSize,
         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
         stagingBuffer,
         stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), size_t(bufferSize));
    vkUnmapMemory(device, stagingBufferMemory);

    create_buffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        index_buffer,
        index_buffer_memory);

    copy_buffer(stagingBuffer, index_buffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Application::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory){
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void Application::copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size){
    VkCommandBuffer commandBuffer = begin_single_time_commands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    end_single_time_commands(commandBuffer);
}

void Application::create_vertex_buffer(){
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    create_buffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), size_t(bufferSize));
    vkUnmapMemory(device, stagingBufferMemory);

    create_buffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertex_buffer,
        vertex_buffer_memory);

    copy_buffer(stagingBuffer, vertex_buffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}


uint32_t Application::find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i)&& (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    RAISE(std::runtime_error, "failed to find suitable memory type!")
}

void Application::handle_event(SDL_Event&){}

void Application::imgui_draw(){}

void Application::decorated_imgui_draw(){
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    imgui_draw();

    ImGui::Render();
}


void Application::decorated_handle_event(SDL_Event& event){
    TimeIt event_handling;
    while (SDL_PollEvent(&event)) {
        IMGUI_GUARD(ImGui_ImplSDL2_ProcessEvent(&event));

        if (event.type == SDL_QUIT) {
            running = false;
        }
        // maybe check event.window.windowID == SDL_GetWindowID(window)
        else if (event.type == SDL_WINDOWEVENT) {
            SDL_WindowEvent wevent = event.window;
            switch (wevent.type) {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_RESIZED:
                framebuffer_resized = true;
                break;
            }
        }

        handle_event(event);
    }
}

void Application::decorated_draw_frames(){
    TimeIt draw_time;
    IMGUI_GUARD(decorated_imgui_draw());
    draw_frame();
}

void Application::event_loop() {
    SDL_Event event;

    double ms_per_frame = 1000.0 / double(fps_cap);

    while (running) {
        TimeIt loop_time;

        decorated_handle_event(event);
        decorated_draw_frames();

        auto free_time = ms_per_frame - (loop_time.stop() * 2);
        if (free_time > 0) {
            std::this_thread::sleep_for(TimeIt::Duration(free_time));
        }
    }

    vkDeviceWaitIdle(device);
}

void Application::cleanup_swap_chain() {
    for (auto framebuffer : sc_framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    vkFreeCommandBuffers(device, cmd_pool, static_cast<uint32_t>(cmd_buffer.size()), cmd_buffer.data());

    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);

    for (auto imageView : sc_views) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swap_chain, nullptr);
}

void Application::cleanup() {
    IMGUI_GUARD(ImGui_ImplVulkan_Shutdown());
    IMGUI_GUARD(ImGui_ImplSDL2_Shutdown());
    IMGUI_GUARD(ImGui::DestroyContext());

    cleanup_swap_chain();

    for (auto& item: texture_cache){
        auto& img = item.second;
        vkDestroySampler(device, img.sampler, nullptr);
        vkDestroyImageView(device, img.view, nullptr);
        vkDestroyImage(device, img.image, nullptr);
        vkFreeMemory(device, img.memory, nullptr);
    }

    vkDestroyBuffer(device, index_buffer, nullptr);
    vkFreeMemory(device, index_buffer_memory, nullptr);

    vkDestroyBuffer(device, vertex_buffer, nullptr);
    vkFreeMemory(device, vertex_buffer_memory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, render_finished[i], nullptr);
        vkDestroySemaphore(device, image_available[i], nullptr);
        vkDestroyFence(device, inflight_fences[i], nullptr);
    }

    vkDestroyCommandPool(device, cmd_pool, nullptr);

    vkDestroyDescriptorPool(device, descriptor_pool, allocator);
    vkDestroyDevice(device, nullptr);

    if (enable_validation_layers) {
        destroy_debug_utils_messenger_EXT(instance, debug_messenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Application::recreate_swap_chain() {
    /*
        // This pauses the rendering when the window is minimized
        int width = 0, height = 0;

    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }*/

    vkDeviceWaitIdle(device);
    cleanup_swap_chain();

    create_swap_chain();
    create_image_views();
    create_render_pass();
    create_graphics_pipeline();
    create_framebuffers();
    create_command_buffers();
}

void Application::create_instance() {
    if (enable_validation_layers && !check_validation_layer_support()) {
        RAISE(std::runtime_error, "validation layers requested, but not available!")
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = required_extensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enable_validation_layers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        createInfo.ppEnabledLayerNames = validation_layers.data();

        populate_debug_messenger_create_info(debugCreateInfo);
        createInfo.pNext = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        RAISE(std::runtime_error, "failed to create instance!")
    }
}

void Application::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debug_callback;
}

void Application::setup_debug_messenger() {
    if (!enable_validation_layers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populate_debug_messenger_create_info(createInfo);

    if (create_debug_utils_messenger_EXT(instance, &createInfo, nullptr, &debug_messenger) != VK_SUCCESS) {
        RAISE(std::runtime_error,"failed to set up debug messenger!")
    }
}

void Application::create_surface() {
    // if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
    if (SDL_Vulkan_CreateSurface(window, instance, &surface) != SDL_TRUE) {
        RAISE(std::runtime_error, "failed to create window surface!")
    }
}

void Application::pick_physical_device() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        RAISE(std::runtime_error, "failed to find GPUs with Vulkan support!")
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (is_device_suitable(device)) {
            physical_device = device;
            break;
        }
    }

    if (physical_device == VK_NULL_HANDLE) {
        RAISE(std::runtime_error, "failed to find a suitable GPU!")
    }
}

void Application::create_logical_device() {
    QueueFamilyIndices indices = find_queue_families(physical_device);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphics_family.value(), indices.present_family.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    createInfo.ppEnabledExtensionNames = device_extensions.data();

    if (enable_validation_layers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        createInfo.ppEnabledLayerNames = validation_layers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physical_device, &createInfo, nullptr, &device) != VK_SUCCESS) {
        RAISE(std::runtime_error, "failed to create logical device!")
    }

    vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);
    vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
}

void Application::create_swap_chain() {
    SwapChainSupportDetails swapChainSupport = query_swap_chain_support(physical_device);

    VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format(swapChainSupport.formats);
    VkPresentModeKHR presentMode = choose_swap_present_mode(swapChainSupport.present_modes);
    VkExtent2D extent = choose_swap_extent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = find_queue_families(physical_device);
    uint32_t queueFamilyIndices[] = {indices.graphics_family.value(), indices.present_family.value()};

    if (indices.graphics_family != indices.present_family) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swap_chain) != VK_SUCCESS) {
        RAISE(std::runtime_error, "failed to create swap chain!")
    }

    vkGetSwapchainImagesKHR(device, swap_chain, &imageCount, nullptr);
    sc_images.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swap_chain, &imageCount, sc_images.data());

    sc_format = surfaceFormat.format;
    sc_extent = extent;
}

VkImageView Application::create_image_view(VkImage image, VkFormat format) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        RAISE(std::runtime_error, "failed to create texture image view!");
    }

    return imageView;
}

void Application::create_image_views() {
    sc_views.resize(sc_images.size());

    for (size_t i = 0; i < sc_images.size(); i++) {
        sc_views[i] = create_image_view(sc_images[i], sc_format);
    }
}

void Application::create_render_pass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = sc_format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &render_pass) != VK_SUCCESS) {
        RAISE(std::runtime_error, "failed to create render pass!")
    }
}


void Application::create_graphics_pipeline() {
    auto vertShaderCode = read_file("shaders/shader.vert.spv", binary_path());
    auto fragShaderCode = read_file("shaders/shader.frag.spv", binary_path());

    VkShaderModule vertShaderModule = create_shader_module(vertShaderCode);
    VkShaderModule fragShaderModule = create_shader_module(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto binding_desc = Vertex::binding_description();
    auto attribute_desc = Vertex::attribute_description();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_desc.size());
    vertexInputInfo.pVertexBindingDescriptions = &binding_desc;
    vertexInputInfo.pVertexAttributeDescriptions = attribute_desc.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = float(sc_extent.width);
    viewport.height = float(sc_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = sc_extent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 2.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipeline_layout) != VK_SUCCESS) {
        RAISE(std::runtime_error, "failed to create pipeline layout!")
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipeline_layout;
    pipelineInfo.renderPass = render_pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphics_pipeline) != VK_SUCCESS) {
        RAISE(std::runtime_error, "failed to create graphics pipeline!")
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void Application::create_framebuffers() {
    sc_framebuffers.resize(sc_views.size());

    for (size_t i = 0; i < sc_views.size(); i++) {
        VkImageView attachments[] = {
            sc_views[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = render_pass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = sc_extent.width;
        framebufferInfo.height = sc_extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &sc_framebuffers[i]) != VK_SUCCESS) {
            RAISE(std::runtime_error, "failed to create framebuffer!")
        }
    }
}


void Application::create_descriptor_pool() {
    debug("Init descriptor pool");
    VkDescriptorPoolSize pool_sizes[] ={
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

//    std::array<VkDescriptorPoolSize, 2> poolSizes{};
//    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//    poolSizes[0].descriptorCount = static_cast<uint32_t>(sc_images.size());
//    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    poolSizes[1].descriptorCount = static_cast<uint32_t>(sc_images.size());

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = uint32_t(IM_ARRAYSIZE(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(device, &pool_info, allocator, &descriptor_pool) != VK_SUCCESS) {
        RAISE(std::runtime_error, "failed to create descriptor pool!")
    }
}

void Application::create_command_pool() {
    QueueFamilyIndices queueFamilyIndices = find_queue_families(physical_device);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphics_family.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &cmd_pool) != VK_SUCCESS) {
        RAISE(std::runtime_error, "failed to create command pool!")
    }
}

void Application::create_command_buffers() {
    cmd_buffer.resize(sc_framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = cmd_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = uint32_t(cmd_buffer.size());

    if (vkAllocateCommandBuffers(device, &allocInfo, cmd_buffer.data()) != VK_SUCCESS) {
        RAISE(std::runtime_error, "failed to allocate command buffers!")
    }

    for (size_t i = 0; i < cmd_buffer.size(); i++) {

    }
}

void Application::create_sync_objects() {
    image_available.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished.resize(MAX_FRAMES_IN_FLIGHT);
    inflight_fences.resize(MAX_FRAMES_IN_FLIGHT);
    inflight_images.resize(sc_images.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &image_available[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &render_finished[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inflight_fences[i]) != VK_SUCCESS) {
            RAISE(std::runtime_error, "failed to create synchronization objects for a frame!")
        }
    }
}

void Application::render_frame(VkCommandBuffer cmd){
    // Draw Triangle
    VkBuffer vertexBuffers[] = {vertex_buffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmd, index_buffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(cmd, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    // ---
}

void Application::decorated_render_frame(VkCommandBuffer cmd, VkFramebuffer frame_buffer){
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        RAISE(std::runtime_error, "failed to begin recording command buffer!")
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = render_pass;
    renderPassInfo.framebuffer = frame_buffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = sc_extent;

    VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

    render_frame(cmd);

    // Draw ImGUI
    IMGUI_GUARD(ImDrawData* draw_data = ImGui::GetDrawData());
    IMGUI_GUARD(ImGui_ImplVulkan_RenderDrawData(draw_data, cmd));
    // ---

    // Done
    vkCmdEndRenderPass(cmd);
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        RAISE(std::runtime_error, "failed to record command buffer!")
    }
}

void Application::draw_frame() {
    // Wait for previous work on the image to finish
    vkWaitForFences(device, 1, &inflight_fences[current_frame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    // acquire the image from the swap chain when the image is finished being presented
    // and drawing can happen again
    {
        VkResult result = vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX, image_available[current_frame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR  | result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
            recreate_swap_chain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            RAISE(std::runtime_error, "failed to acquire swap chain image!")
        }
    }

    // ---
    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (inflight_images[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &inflight_images[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    inflight_images[imageIndex] = inflight_fences[current_frame];
    //

    // Submit the work
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {image_available[current_frame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        // Wait for the image to be available
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd_buffer[imageIndex];

        // When rendering is done signal it to the rendering semaphore
        VkSemaphore signalSemaphores[] = {render_finished[current_frame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        // Make Rendering commands
        vkResetCommandBuffer(cmd_buffer[imageIndex], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        decorated_render_frame(cmd_buffer[imageIndex], sc_framebuffers[imageIndex]);

        vkResetFences(device, 1, &inflight_fences[current_frame]);
        if (vkQueueSubmit(graphics_queue, 1, &submitInfo, inflight_fences[current_frame]) != VK_SUCCESS) {
            RAISE(std::runtime_error, "failed to submit draw command buffer!")
        }
    }

    // Present the image
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;

        // Wait for rendering to be done before presenting the image
        VkSemaphore signalSemaphores[] = {render_finished[current_frame]};
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swap_chain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        auto result = vkQueuePresentKHR(present_queue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
            framebuffer_resized = false;
            recreate_swap_chain();
        } else if (result != VK_SUCCESS) {
            RAISE(std::runtime_error, "failed to present swap chain image!")
        }
    }

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

VkShaderModule Application::create_shader_module(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        RAISE(std::runtime_error, "failed to create shader module!")
    }

    return shaderModule;
}

VkSurfaceFormatKHR Application::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Application::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    bool non_compliance_found = true;

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == preferred_mode) {
            return availablePresentMode;
        }

        if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR){
            non_compliance_found = false;
        }
    }

    if (non_compliance_found){
        RAISE(std::runtime_error, "Device is not compliant; VK_PRESENT_MODE_FIFO_KHR not implemented")
    }

    // v-sync (2)
    // specifies that the presentation engine waits for the next vertical blanking period to update the current image.
    // Tearing cannot be observed
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        // glfwGetFramebufferSize(window, &width, &height);
        SDL_GetWindowSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

SwapChainSupportDetails Application::query_swap_chain_support(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.present_modes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.present_modes.data());
    }

    return details;
}

bool Application::is_device_suitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = find_queue_families(device);

    bool extensionsSupported = check_device_extension_support(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = query_swap_chain_support(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.present_modes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.is_complete()
           && extensionsSupported
           && swapChainAdequate
           && supportedFeatures.samplerAnisotropy;
}

bool Application::check_device_extension_support(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(device_extensions.begin(), device_extensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

QueueFamilyIndices Application::find_queue_families(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport) {
            indices.present_family = i;
        }

        if (indices.is_complete()) {
            break;
        }

        i++;
    }

    return indices;
}

std::vector<const char*> Application::required_extensions() {
    uint32_t glfwExtensionCount = 0;
    // const char** glfwExtensions;
    // glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &glfwExtensionCount, nullptr);

    std::vector<const char*> extensions(glfwExtensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &glfwExtensionCount, extensions.data());

    if (enable_validation_layers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool Application::check_validation_layer_support() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validation_layers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}
}
