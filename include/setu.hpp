#pragma once

#include <string>
#include <tuple>
#include <array>
#include <vector>
#include <optional>
#include <iostream>
#include <set>
#include <fstream>
#include <sstream>
#include <streambuf>

#include <glad/vulkan.h>
#include <GLFW/glfw3.h>
#include <shaderc/shaderc.hpp>

namespace fhope {
    /***********************
     ** GLOBALS CONSTANTS **
     ***********************/

    inline constexpr const char *ENGINE_NAME = "FinalHope"; // Name of the engine
    inline constexpr std::tuple<uint32_t, uint32_t, uint32_t> ENGINE_VERSION = {0, 0, 1}; // Current engine version

    inline constexpr std::array<const char *, 1> ENGINE_REQUIRED_DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    inline constexpr std::array<const char *, 1> ENGINE_REQUIRED_VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

    /****************
     ** STRUCTURES **
     ****************/

    struct QueueSetup {
        std::optional<uint32_t> graphicsIndex = std::nullopt;
        std::optional<uint32_t> presentIndex  = std::nullopt;

        std::vector<float> priorities;

        bool is_complete() const;
    };

    struct SwapChainSupport {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct SwapChainConfig {
        VkSurfaceFormatKHR surfaceFormat;
        VkPresentModeKHR   presentMode;
        VkExtent2D         extent;

        uint32_t imageCount;
    };

    struct GraphicsPipelineConfig {
        std::string vertexShaderFilename;
        std::string fragmentShaderFilename;

        VkPipelineLayout pipelineLayout;
        VkRenderPass renderPass;

        VkPipeline pipeline;
    };

    struct BaseSyncObjects {
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence     inFlightFence;
    };

    struct InstanceSetup {
        VkInstance instance = VK_NULL_HANDLE; // Vulkan instance
        std::optional<VkDebugUtilsMessengerEXT> debugMessenger = std::nullopt; // Debugger of the instance

        std::optional<VkPhysicalDevice> physicalDevice = std::nullopt;

        std::optional<VkDevice> logicalDevice = std::nullopt;

        std::optional<VkSurfaceKHR> surface = std::nullopt;

        std::optional<QueueSetup> queues = std::nullopt;

        std::optional<SwapChainSupport> swapChainSupport = std::nullopt;
        
        std::optional<VkQueue> graphicsQueue;
        std::optional<VkQueue> presentQueue;

        std::optional<SwapChainConfig> swapChainConfig;

        std::optional<VkSwapchainKHR> swapChain;
        std::vector<VkImage>          swapChainImages;
        std::vector<VkImageView>      swapChainImageViews;

        std::optional<GraphicsPipelineConfig> graphicsPipelineConfig;

        std::vector<VkFramebuffer> swapChainFramebuffers;

        std::optional<VkCommandPool> commandPool;

        std::optional<VkCommandBuffer> commandBuffer;

        std::optional<BaseSyncObjects> syncObjects;
    };

    /***************
     ** CALLBACKS **
     ***************/
    
    // Default dumb error logger for validation layers
    static VKAPI_ATTR VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void *pUserData);

    /***************
     ** FUNCTIONS **
     ***************/

    /**
     * @brief Initializes the engine's dependencies
     * 
     */
    void initialize_dependencies();

    /**
     * @brief Terminates / cleans up the engine's dependencies
     * 
     */
    void terminate_dependencies();

    InstanceSetup generate_vulkan_setup(GLFWwindow *window, std::string appName, std::tuple<uint32_t, uint32_t, uint32_t> appVersion, const std::string &vertexShaderFilename, const std::string &fragmentShaderFilename);

    /**
     * @brief Prepares and returns an instance and it's setup
     * 
     * @param appName Name of the application to run on the instance
     * @param appVersion Version of the application to run on the instance
     * @param debug Wether ot not debug mode is on
     * @return VulkanInstanceSetup The vulkan instances and required companion values
     */
    InstanceSetup create_instance(std::string appName, std::tuple<uint32_t, uint32_t, uint32_t> appVersion);

    VkSurfaceKHR get_surface_from_window(const InstanceSetup &setup, GLFWwindow *source);

    std::optional<VkPhysicalDevice> autopick_physical_device(const InstanceSetup &setup);

    bool is_physical_device_suitable(const InstanceSetup &setup, const VkPhysicalDevice &physicalDevice);

    int32_t score_physical_device(const InstanceSetup &setup, const VkPhysicalDevice &physicalDevice);

    bool check_physical_device_extension_support(const InstanceSetup &setup, const VkPhysicalDevice &physicalDevice);

    QueueSetup find_queue_families(const InstanceSetup &setup, const VkPhysicalDevice &physicalDevice);

    SwapChainSupport check_swap_chain_support(const InstanceSetup &setup, const VkPhysicalDevice &physicalDevice);

    VkDevice create_logical_device(InstanceSetup *setup);

    SwapChainConfig prepare_swap_chain_config(const InstanceSetup &setup, GLFWwindow *window);

    VkSwapchainKHR create_swap_chain(const InstanceSetup &setup, GLFWwindow *window);
    
    std::vector<VkImage> retrieve_swap_chain_images(const InstanceSetup &setup);

    std::vector<VkImageView> create_swap_chain_image_views(const InstanceSetup &setup);

    shaderc::SpvCompilationResult compile_shader(const std::string &filename, const shaderc_shader_kind &shaderKind);

    VkShaderModule create_shader_module(const InstanceSetup &setup, const shaderc::SpvCompilationResult &compiledShader);
    
    VkRenderPass create_render_pass(const InstanceSetup &setup);

    GraphicsPipelineConfig create_graphics_pipeline(const InstanceSetup &setup, const std::string &vertexShaderFilename, const std::string &fragmentShaderFilename);

    std::vector<VkFramebuffer> create_framebuffers(const InstanceSetup &setup);

    VkCommandPool create_command_pool(const InstanceSetup &setup);

    VkCommandBuffer create_command_buffer(const InstanceSetup &setup);

    void record_command_buffer(const InstanceSetup &setup, const VkCommandBuffer &commandBuffer, uint32_t imageIndex);

    BaseSyncObjects create_base_sync_objects(const InstanceSetup &setup);

    void draw_frame(const InstanceSetup &setup);

    void clean_setup(const InstanceSetup &setup);
}
