#ifndef PUZZLE_APP_HEADER
#define PUZZLE_APP_HEADER

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>

#include <vector>
#include <optional>

#define IMGUI_GUARD(X) X

#include "logger.h"
#include "utils.h"
#include "imgui_impl_vulkan.h"


namespace puzzle {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;

    bool is_complete() {
        return graphics_family.has_value() && present_family.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   present_modes;
};

struct ImgGuiState {
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
};

class Application {
public:
    Application(uint32_t w, uint32_t h, bool enable_validation=false):
        WIDTH(w), HEIGHT(h), enable_validation_layers(enable_validation)
    {}

    virtual ~Application(){}

    std::vector<const char*> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    std::vector<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    uint32_t     WIDTH;
    uint32_t     HEIGHT;
    const size_t MAX_FRAMES_IN_FLIGHT     = 2;
    bool         enable_validation_layers = true;
    bool         running                  = true;
    bool         framebuffer_resized      = false;

    SDL_Window*                  window;
    VkInstance                   instance;
    VkDebugUtilsMessengerEXT     debug_messenger;
    VkSurfaceKHR                 surface;
    VkPhysicalDevice             physical_device = VK_NULL_HANDLE;
    VkDevice                     device;
    VkQueue                      graphics_queue;
    VkQueue                      present_queue;
    VkSwapchainKHR               swap_chain;
    std::vector<VkImage>         sc_images;
    VkFormat                     sc_format;
    VkExtent2D                   sc_extent;
    std::vector<VkImageView>     sc_views;
    std::vector<VkFramebuffer>   sc_framebuffers;
    VkRenderPass                 render_pass;
    VkPipelineLayout             pipeline_layout;
    VkPipeline                   graphics_pipeline;
    VkCommandPool                cmd_pool;
    std::vector<VkCommandBuffer> cmd_buffer;
    std::vector<VkSemaphore>     image_available;
    std::vector<VkSemaphore>     render_finished;
    std::vector<VkFence>         inflight_fences;
    std::vector<VkFence>         inflight_images;
    size_t                       current_frame = 0;

    VkBuffer                     vertex_buffer;
    VkDeviceMemory               vertex_buffer_memory;
    VkBuffer                     index_buffer;
    VkDeviceMemory               index_buffer_memory;

    // ImGUI
    VkDescriptorPool             descriptor_pool = VK_NULL_HANDLE;
    VkAllocationCallbacks*       allocator = nullptr;
    VkPipelineCache              pipeline_cache = VK_NULL_HANDLE;

    ImGui_ImplVulkanH_Window     imgui_data;
    ImgGuiState                  gui;

    RollingAverage<120>          render_times;
    // VK_PRESENT_MODE_MAILBOX_KHR: v-sync like
    // specifies that the presentation engine waits for the next vertical blanking period to update the current image.
    // Tearing cannot be observed.
    // Queue of size 1, if full newest frame replace the old one

    // VK_PRESENT_MODE_FIFO_KHR: v-sync like
    // Images are put on a queue to be displayed
    // REQUIRED by the standard

    // VK_PRESENT_MODE_IMMEDIATE_KHR: No capping
    // request applied immediately
    VkPresentModeKHR             preferred_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    float                        fps_cap        = 244;

    bool init_imgui();

    void init_fonts();

    void run() {
        init_window();
        init_vulkan();

        // We are ready to render frames
        // start loading ImGUI and its fonts
        IMGUI_GUARD(init_imgui());
        IMGUI_GUARD(init_fonts());

        event_loop();
        cleanup();
    }

    bool init_window();
    void init_vulkan();

    void cleanup_swap_chain();
    void cleanup();

    void pick_physical_device();

    void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setup_debug_messenger();

    void create_instance();
    void create_surface();
    void create_logical_device();
    void create_swap_chain();
    void recreate_swap_chain();
    void create_image_views();
    void create_render_pass();
    void create_graphics_pipeline();
    void create_framebuffers();
    void create_command_pool();
    void create_index_buffer();
    void create_vertex_buffer();
    void create_command_buffers();
    void create_sync_objects();
    void create_descriptor_pool();
    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    void event_loop();
    void draw_frame();

    void decorated_imgui_draw();
    void decorated_render_frame(VkCommandBuffer cmd, VkFramebuffer frame_buffer);
    void decorated_draw_frames();
    void decorated_handle_event(SDL_Event& event);

    virtual void handle_event(SDL_Event& event);
    virtual void render_frame(VkCommandBuffer cmd);
    virtual void imgui_draw();

    void present_frame();

    void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    VkShaderModule create_shader_module(const std::vector<char>& code);

    VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR   choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D         choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

    bool check_device_extension_support(VkPhysicalDevice device);
    bool check_validation_layer_support();

    SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device);
    QueueFamilyIndices find_queue_families(VkPhysicalDevice device);
    uint32_t           find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    std::vector<const char*> required_extensions();
    bool is_device_suitable(VkPhysicalDevice device);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
        debug("validation: {}/{}: {}", messageSeverity, messageType, pCallbackData->pMessage);
        return VK_FALSE;
    }
};

}

#endif
