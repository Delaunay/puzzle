#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>

#include <vector>
#include <optional>

#include "logger.h"

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

class Application {
public:
    Application(uint32_t w, uint32_t h, bool enable_validation=false):
        WIDTH(w), HEIGHT(h), enable_validation_layers(enable_validation)
    {}

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

    void run() {
        init_window();
        init_vulkan();
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
    void create_command_buffers();
    void create_sync_objects();

    void event_loop();
    void handle_event(SDL_Event& event);
    void draw_frame();

    VkShaderModule create_shader_module(const std::vector<char>& code);

    VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR   choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D         choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

    bool check_device_extension_support(VkPhysicalDevice device);
    bool check_validation_layer_support();

    SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device);
    QueueFamilyIndices find_queue_families(VkPhysicalDevice device);

    std::vector<const char*> required_extensions();
    bool is_device_suitable(VkPhysicalDevice device);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
        debug("validation: {}/{}: {}", messageSeverity, messageType, pCallbackData->pMessage);
        return VK_FALSE;
    }
};

}
