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
#include <cmath>
#include <unordered_map>

#include <glad/vulkan.h>
#include <GLFW/glfw3.h>
#include <shaderc/shaderc.hpp>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include "vertex.hpp"

namespace fhope {
    /***********************
     ** GLOBALS CONSTANTS **
     ***********************/

    inline constexpr const char *ENGINE_NAME = "FinalHope"; // Name of the engine
    inline constexpr std::tuple<uint32_t, uint32_t, uint32_t> ENGINE_VERSION = {0, 0, 1}; // Current engine version

    inline constexpr std::array<const char *, 1> ENGINE_REQUIRED_DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    inline constexpr std::array<const char *, 1> ENGINE_REQUIRED_VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

    inline constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    /*inline constexpr std::array<Vertex3D, 8> EXAMPLE_VERTICES = {
        Vertex3D{.position=glm::vec3{-0.5f, -0.5f,  0.0f}, .color=glm::vec3{1.0f, 0.0f, 0.0f}, .uv=glm::vec2{1.0f, 0.0f}},
        Vertex3D{.position=glm::vec3{ 0.5f, -0.5f,  0.0f}, .color=glm::vec3{0.0f, 1.0f, 0.0f}, .uv=glm::vec2{0.0f, 0.0f}},
        Vertex3D{.position=glm::vec3{ 0.5f,  0.5f,  0.0f}, .color=glm::vec3{0.0f, 0.0f, 1.0f}, .uv=glm::vec2{0.0f, 1.0f}},
        Vertex3D{.position=glm::vec3{-0.5f,  0.5f,  0.0f}, .color=glm::vec3{1.0f, 1.0f, 1.0f}, .uv=glm::vec2{1.0f, 1.0f}},

        Vertex3D{.position=glm::vec3{-0.5f, -0.5f, -0.5f}, .color=glm::vec3{1.0f, 0.0f, 0.0f}, .uv=glm::vec2{1.0f, 0.0f}},
        Vertex3D{.position=glm::vec3{ 0.5f, -0.5f, -0.5f}, .color=glm::vec3{0.0f, 1.0f, 0.0f}, .uv=glm::vec2{0.0f, 0.0f}},
        Vertex3D{.position=glm::vec3{ 0.5f,  0.5f, -0.5f}, .color=glm::vec3{0.0f, 0.0f, 1.0f}, .uv=glm::vec2{0.0f, 1.0f}},
        Vertex3D{.position=glm::vec3{-0.5f,  0.5f, -0.5f}, .color=glm::vec3{1.0f, 1.0f, 1.0f}, .uv=glm::vec2{1.0f, 1.0f}},
    };

    inline constexpr std::array<uint16_t, 12> EXAMPLE_INDICES = {0, 1, 2, 2, 3, 0,
                                                                 4, 5, 6, 6, 7, 4};*/

    /****************
     ** STRUCTURES **
     ****************/

    struct LoadedModel {
        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;
    };

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
    };

    struct QueueSetup {
        std::optional<uint32_t> graphicsIndex = std::nullopt;
        std::optional<uint32_t> presentIndex  = std::nullopt;
        std::optional<uint32_t> transferIndex = std::nullopt;

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
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence>     inFlightFences;
    };

    struct WrappedBuffer {
        VkBuffer buffer;
        VkDeviceMemory memory;
        VkDeviceSize sizeInBytes;
        std::optional<void*> mapping;
    };

    struct WrappedTexture {
        VkImage texture;
        VkDeviceMemory memory;
        std::optional<uint32_t> mipLevels;
    };

    struct ViewableImage {
        WrappedTexture image;
        VkImageView imageView;
    };

    struct CommandPools {
        VkCommandPool graphics;
        VkCommandPool transfer;
    };

    struct DepthBuffer {
        WrappedTexture image;
        VkImageView view;
        VkFormat format;
        bool hasStencil;
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
        std::optional<VkQueue> transferQueue;

        std::optional<SwapChainConfig> swapChainConfig;

        std::optional<VkSwapchainKHR> swapChain;
        std::vector<VkImage>          swapChainImages;
        std::vector<VkImageView>      swapChainImageViews;

        std::optional<VkDescriptorSetLayout> uniformLayout;

        std::optional<GraphicsPipelineConfig> graphicsPipelineConfig;

        std::vector<VkFramebuffer> swapChainFramebuffers;

        std::optional<CommandPools> commandPools;

        std::optional<DepthBuffer> depthBuffer;
        
        std::optional<WrappedTexture> texture;
        std::optional<VkImageView> textureView;

        std::optional<VkSampler> textureSampler;

        std::optional<WrappedBuffer> vertexBuffer;

        std::optional<WrappedBuffer> indexBuffer;
        std::optional<size_t> indexCount;

        std::vector<WrappedBuffer> uniformBuffers;

        std::optional<VkDescriptorPool> descriptorPool;
        std::vector<VkDescriptorSet>    descriptorSets;

        std::vector<VkCommandBuffer> commandBuffers;

        std::optional<BaseSyncObjects> syncObjects;

        std::optional<VkSampleCountFlagBits> maxSamplesFlag;

        std::optional<ViewableImage> colorImage;

        uint32_t currentFrame = 0;
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

    InstanceSetup generate_vulkan_setup(GLFWwindow *window, std::string appName, std::tuple<uint32_t, uint32_t, uint32_t> appVersion, const std::string &vertexShaderFilename, const std::string &fragmentShaderFilename, const std::string &textureFilename, const std::string &modelFilename);

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

    VkDescriptorSetLayout create_descriptor_set_layout(const InstanceSetup &setup);

    GraphicsPipelineConfig create_graphics_pipeline(const InstanceSetup &setup, const std::string &vertexShaderFilename, const std::string &fragmentShaderFilename);

    std::vector<VkFramebuffer> create_framebuffers(const InstanceSetup &setup);

    CommandPools create_command_pool(const InstanceSetup &setup);

    DepthBuffer create_depth_buffer(const InstanceSetup &setup);

    std::vector<VkFormat> find_supported_formats(const InstanceSetup &setup, const std::vector<VkFormat> &candidates, const VkImageTiling &tiling, const VkFormatFeatureFlags &features);

    WrappedBuffer create_buffer(const InstanceSetup &setup, VkDeviceSize sizeInBytes, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

    void copy_buffer(const InstanceSetup &setup, const WrappedBuffer &source, WrappedBuffer *dest);

    WrappedBuffer create_vertex_buffer(const InstanceSetup &setup, const std::vector<Vertex3D> &vertices);

    WrappedBuffer create_index_buffer(const InstanceSetup &setup, const std::vector<uint32_t> &indices);
    
    std::vector<WrappedBuffer> create_uniform_buffers(const InstanceSetup &setup);

    VkDescriptorPool create_descriptor_pool(const InstanceSetup &setup);

    std::vector<VkDescriptorSet> create_descriptor_sets(const InstanceSetup &setup);

    std::vector<VkCommandBuffer> create_command_buffers(const InstanceSetup &setup);

    void record_command_buffer(const InstanceSetup &setup, const VkCommandBuffer &commandBuffer, uint32_t imageIndex, size_t currentFrame);

    BaseSyncObjects create_base_sync_objects(const InstanceSetup &setup);

    void update_uniform_buffer(const InstanceSetup &setup, size_t frame);

    WrappedTexture create_texture_from_image(const InstanceSetup &setup, const std::string &textureFilname);

    WrappedTexture create_texture(const InstanceSetup &setup, int width, int height, VkSampleCountFlagBits flags, uint32_t mipLevels, VkFormat depthFormat, VkImageUsageFlags usage);

    VkImageView create_texture_image_view(const InstanceSetup &setup, const WrappedTexture &texture, const VkFormat &format, uint32_t mipLevels);

    void generate_mipmaps(const InstanceSetup &setup, const VkImage &image, const VkFormat &format, int width, int height, uint32_t mipLevels);

    VkSampler create_texture_sampler(const InstanceSetup &setup, std::optional<uint32_t> mipLevels);

    VkCommandBuffer begin_one_shot_command(const InstanceSetup &setup, const VkCommandPool &selectedPool);

    void end_one_shot_command(const InstanceSetup &setup, const VkCommandPool &selectedPool, const VkQueue &selectedQueue, VkCommandBuffer *osCommandBuffer);

    void transition_image_layout(const InstanceSetup &setup, WrappedTexture *texture, const VkFormat &format, const VkImageLayout &oldLayout, const VkImageLayout &newLayout, uint32_t mipLevels);

    void copy_buffer_to_image(const InstanceSetup &setup, const WrappedBuffer &dataSource, VkImage *image, uint32_t width, uint32_t height);
   
    VkSampleCountFlagBits get_max_multisampling_level(const InstanceSetup &setup);
    
    ViewableImage create_color_image(const InstanceSetup &setup);

    void draw_frame(InstanceSetup *setup, GLFWwindow *window, size_t *currentFrame);

    void cleanup_swap_chain(const InstanceSetup &setup);

    void recreate_swap_chain(InstanceSetup *setup, GLFWwindow *window);

    uint32_t find_memory_type(const VkPhysicalDevice &device, uint32_t typeFilter, const VkMemoryPropertyFlags &properties);

    LoadedModel load_model(const std::string &filename);

    void clean_setup(const InstanceSetup &setup);
}
