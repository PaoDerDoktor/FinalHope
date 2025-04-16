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

    inline constexpr const char *ENGINE_NAME = "FinalHope"; ///< Name of the engine
    inline constexpr std::tuple<uint32_t, uint32_t, uint32_t> ENGINE_VERSION = {0, 0, 1}; ///< Current engine version

    inline constexpr std::array<const char *, 1> ENGINE_REQUIRED_DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME }; ///< List of extensions the engine needs to run
    inline constexpr std::array<const char *, 1> ENGINE_REQUIRED_VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" }; ///< List of validation layers the engine needs to run

    inline constexpr int MAX_FRAMES_IN_FLIGHT = 2; ///< Maximum amount of in-flight frames (double buffering, triple buffering, etc)

    /****************
     ** STRUCTURES **
     ****************/
    
    
    /**
     * @brief Set of queues
     */
    struct QueueSetup {
        std::optional<uint32_t> graphicsIndex = std::nullopt; ///< Graphics queue family index
        std::optional<uint32_t> presentIndex  = std::nullopt; ///< Presentation queue family index
        std::optional<uint32_t> transferIndex = std::nullopt; ///< Transfer (non-graphic) queue family index
        
        std::vector<float> priorities; ///< list of prioriities for the queues
        
        /**
         * @brief Checks wether or not every optional queue family index is present in the instantiated structure
         * 
         * @return true If every queue has a value
         * @return false If at least one queue has no value
         */
        bool is_complete() const;
    };


    /**
     * @brief Informations about a device's swap chain's support
     */
    struct SwapChainSupport {
        VkSurfaceCapabilitiesKHR capabilities; ///< Capabilities of the surface an hypothetical swap chain would present to
        std::vector<VkSurfaceFormatKHR> formats; ///< List of available formats an hypothetical swap chain could use
        std::vector<VkPresentModeKHR> presentModes; ///< List of available presentation modes an hypothetical swap chain could use
    };
    
    
    /**
     * @brief Swap chain configuration set
     */
    struct SwapChainConfig {
        VkSurfaceFormatKHR surfaceFormat; ///< Swap chain's destination surface's format
        VkPresentModeKHR   presentMode; ///< Swap chain's presentation mode
        VkExtent2D         extent; ///< Extent (dimensions) of the swap chain's intended presented image
        
        uint32_t imageCount; ///< Number of images in the swap chain (in-flight images)
    };
    

    /**
     * @brief Set of command pools
     */
    struct CommandPools {
        VkCommandPool graphics; ///< Graphics command pool
        VkCommandPool transfer; ///< Transfer (non-graphic) command pool
    };
    
    
    /**
     * @brief Wrapped of a vulkan image intended to be used as a texture or a destination image
     */
    struct WrappedTexture {
        VkImage texture; ///< Proper wrapped vulkan image of the texture
        VkDeviceMemory memory; ///< Physical memory of the image
        std::optional<uint32_t> mipLevels; ///< Amount of mipmap of the texture
    };


    /**
     * @brief Double wrapping of a wrapped vulkan image and a corresponding image view
     * 
     */
    struct ViewableImage {
        WrappedTexture image; ///< Proper double wrapped image
        VkImageView imageView; ///< view to the wrapped image
    };


    /**
     * @brief Wrapped depth buffer with useful informations
     */
    struct DepthBuffer {
        WrappedTexture image; ///< Wrapped vulkan image corresponding to the depth buffer
        VkImageView view; ///< View to the depth buffer's corresponding vulkan image
        VkFormat format; ///< Format of the depth buffer
        bool hasStencil; ///< Wether or not the depth buffer also has stencil test
    };
    
    
    /**
     * @brief Wrapped vulkan graphics pipeline with configuration information
     */
    struct GraphicsPipelineConfig {
        std::string vertexShaderFilename; ///< Used vertex shader's source's filename
        std::string fragmentShaderFilename; ///< Used fragment shader's source's filename
        
        VkPipelineLayout pipelineLayout; ///< Layout of the pipeline's mutable states
        VkRenderPass renderPass; ///< Used render pass
        
        VkPipeline pipeline; ///< Proper Wrapped pipeline
    };


    /**
     * @brief Wrapped vulkan data buffer with useful informations for clean and safe utilization
     */
    struct WrappedBuffer {
        VkBuffer buffer; ///< Proper wrapped vulkan buffer
        VkDeviceMemory memory; ///< Physical memory of the buffer
        VkDeviceSize sizeInBytes; ///< Number of bytes of the buffer's memory
        std::optional<void*> mapping; ///< Memory mapping (CPU <-> GPU) if required
    };


    /**
     * @brief Objects used for synchronization of GPU calls
     */
    struct BaseSyncObjects {
        std::vector<VkSemaphore> imageAvailableSemaphores; ///< Semaphores for image availability synchronization (1 per in-flight frame)
        std::vector<VkSemaphore> renderFinishedSemaphores; ///< Semaphores for image rendering synchronization (1 per in-flight frame)
        std::vector<VkFence>     inFlightFences;           ///< Fences for in-flight frame draw call synchronization (1 per in-flight frame)
    };


    /**
     * @brief Memory-optimized representation of a 3d model, almost ready to be sent to the GPU as buffers
     */
    struct LoadedModel {
        std::vector<Vertex3D> vertices; ///< Memory-optimized vertices of a model
        std::vector<uint32_t> indices;  ///< Indices of a model's vertices, allowing for their reutilization and non-demultiplication
    };
    
    
    /**
     * @brief Buffer containing values to be sent as an uniform to a shader program
     */
    struct UniformBufferObject {
        glm::mat4 model;      ///< Model matrix (location/rotation of a model)
        glm::mat4 view;       ///< View matrix (eye and depth)
        glm::mat4 projection; ///< Projection matrix (perspective)
    };


    /**
     * @brief Modular structure intended to represent a vulkan rendering setup
     */
    struct InstanceSetup {
        VkInstance instance = VK_NULL_HANDLE; ///< Vulkan instance
        
        std::optional<VkDebugUtilsMessengerEXT> debugMessenger = std::nullopt; ///< Debugger of the instance
        
        std::optional<VkSurfaceKHR> surface = std::nullopt; ///< Surfface the swap chain would be able to present to

        std::optional<VkPhysicalDevice> physicalDevice = std::nullopt; ///< Physical device chosen for the instance
        
        std::optional<VkSampleCountFlagBits> maxSamplesFlag; ///< maximum swap amount supported by the physical devic

        std::optional<QueueSetup> queues = std::nullopt; ///< Queues of the physical device

        std::optional<SwapChainSupport> swapChainSupport = std::nullopt; ///< Swap chain support informations

        std::optional<VkDevice> logicalDevice = std::nullopt; ///< Logical device derived from the physical device

        std::optional<VkQueue> graphicsQueue; ///< vulkan graphics queue if the devices
        std::optional<VkQueue> presentQueue;  ///< vulkan presentation queue if the devices
        std::optional<VkQueue> transferQueue; ///< vulkan transfer (non-graphics) queue if the devices

        std::optional<SwapChainConfig> swapChainConfig; ///< Swap chain effective configuration

        std::optional<VkSwapchainKHR> swapChain;           ///< Swap chain
        std::vector<VkImage>          swapChainImages;     ///< Images of the swap chain
        std::vector<VkImageView>      swapChainImageViews; ///< Views to the swap chain's images

        std::optional<VkDescriptorSetLayout> uniformLayout; ///< uniform layout
        
        std::optional<CommandPools> commandPools; ///< Command pools to use queues
        
        std::optional<ViewableImage> colorImage; ///< Swapchain-presentable color image (must be 1-sampled)

        std::optional<DepthBuffer> depthBuffer; ///< Depth buffer

        std::optional<GraphicsPipelineConfig> graphicsPipelineConfig; ///< Drawing graphics pipeline

        std::vector<VkFramebuffer> swapChainFramebuffers; ///< List of framebuffers (1 per in-flight frame)

        //TODO: should be modular and multiple (per-model)
        std::optional<WrappedTexture> texture; ///< Texture
        std::optional<VkImageView> textureView; ///< View to the texture

        //TODO: should be modular and multiple (per-model)
        std::optional<VkSampler> textureSampler; ///< Texture sampler

        //TODO: should be modular and multiple (per-model)
        std::optional<WrappedBuffer> vertexBuffer; ///< Vertex buffer

        //TODO: should be modular and multiple (per-model)
        std::optional<WrappedBuffer> indexBuffer; ///< Index buffer for memory-size optimization of vertices
        std::optional<size_t> indexCount; ///< Number of indices in the buffer

        std::vector<WrappedBuffer> uniformBuffers; ///< Uniform Buffer Objects (1 per in-flight frame)

        std::optional<VkDescriptorPool> descriptorPool; ///< Descriptor pools to integrate descriptor sets
        std::vector<VkDescriptorSet>    descriptorSets; ///< Descriptor sets to bind non-vertice-related data

        std::vector<VkCommandBuffer> commandBuffers; ///< Draw-purposed command buffers (1 per in-flight frame)

        std::optional<BaseSyncObjects> syncObjects; ///< Synchronization objects

        uint32_t currentFrame = 0; ///< Current frame counter
    };

    /***************
     ** CALLBACKS **
     ***************/

    // Default dumb error logger for validation layers
    static VKAPI_ATTR VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void *pUserData);

    /***************
     ** FUNCTIONS **
     ***************/

        /*------------------------------------*
         *- FUNCTIONS: Dependency management -* 
         *------------------------------------*/

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

        /*-------------------------------*
         *- FUNCTIONS: Setup generation -*
         *-------------------------------*/

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

    /**
     * @brief Gets a vulkan surface from a GLFW window handle
     * 
     * @param setup A setup containing at least an instance
     * @param source The GLFW window handle to retrieve a vulkan surface from
     * @return VkSurfaceKHR The vulkan surface corresponding to the given window handle and vulkan instance
     */
    VkSurfaceKHR get_surface_from_window(const InstanceSetup &setup, GLFWwindow *source);

    /**
     * @brief Automatically chooses, creates and returns a suitable vulkan physical device (considering requirements and a scoring system)
     * 
     * @param setup A setup containing at least an instance
     * @return std::optional<VkPhysicalDevice> If it has been found, the autopicked physical device
     */
    std::optional<VkPhysicalDevice> autopick_physical_device(const InstanceSetup &setup);

    /**
     * @brief Checks wether or not a given physical device is suitable for the engine's needs
     * 
     * @param setup A setup containing at least an instance
     * @param physicalDevice The vulkan physical device to examine
     * @return true If given physical device is suitable for the engine
     * @return false If given physical device is not suitable for the engine
     */
    bool is_physical_device_suitable(const InstanceSetup &setup, const VkPhysicalDevice &physicalDevice);

    /**
     * @brief Finds and returns a set of queue families indices
     * 
     * @param setup A setup containing at least a surface (and it's requirements)
     * @param physicalDevice The physical device for which to find it's queue families
     * @return QueueSetup The found set of queue families indices
     */
    QueueSetup find_queue_families(const InstanceSetup &setup, const VkPhysicalDevice &physicalDevice);

    /**
     * @brief Checks wether ot not a given physical device supports required extensions
     * 
     * @param setup A setup containing at least an instance
     * @param physicalDevice The physical device for which to check it's extensions' availability
     * @return true If all required extensions are supported
     * @return false If one or more extension is unsupported
     */
    bool check_physical_device_extension_support(const InstanceSetup &setup, const VkPhysicalDevice &physicalDevice);

    /**
     * @brief Checks and returns information about the swapchain support of a given physical device with a setup
     * 
     * @param setup A setup containing at least a surface (and it's requirements)
     * @param physicalDevice The physical device for which to check it's swapchain support
     * @return SwapChainSupport The informations about swap chain support found for the physical device
     */
    SwapChainSupport check_swap_chain_support(const InstanceSetup &setup, const VkPhysicalDevice &physicalDevice);

    /**
     * @brief Evaluate a physical device 
     * 
     * @param setup A setup containing at least an instance
     * @param physicalDevice The physical device to score
     * @return int32_t The score of the physical device
     */
    int32_t score_physical_device(const InstanceSetup &setup, const VkPhysicalDevice &physicalDevice);

    /**
     * @brief Gets the max multisampling level of a setup
     * 
     * @param setup A setup containing at least a physical device (and it's requirements)
     * @return VkSampleCountFlagBits The maximum multisampling level of the given setup
     */
    VkSampleCountFlagBits get_max_multisampling_level(const InstanceSetup &setup);


    // TODO: find a way to avoid populating the setup in this function
    /**
     * @brief Creates a logical device for a setup and populates the considered setup with queue priorities
     * 
     * @param setup A pointer to a setup containing at least a physical device and queues family indices (and their requirements)
     * @return VkDevice A logical device created to fit in the given setup
     */
    VkDevice create_logical_device(InstanceSetup *setup);

    /**
     * @brief Prepare a swap chain configuration given a setup and a GLFW window handle
     * 
     * @param setup A setup containing at least swap chain support informations (and it's requirements)
     * @param window A handle to the GLFW window the swap chain would be presenting to
     * @return SwapChainConfig A configuration for a swap chain
     */
    SwapChainConfig prepare_swap_chain_config(const InstanceSetup &setup, GLFWwindow *window);

    /**
     * @brief Create a swap chain considering a setup and a GLFW window handle
     * 
     * @param setup A setup containing at least a swap chain config, swap chain support informations, queu family indices for graphics and present queues, and a logical device (and their requirements)
     * @param window The GLFW window handle the swapchain will present to
     * @return VkSwapchainKHR The created swapchain
     */
    VkSwapchainKHR create_swap_chain(const InstanceSetup &setup, GLFWwindow *window);

    /**
     * @brief Retrieves a swap chain's images
     * 
     * @param setup A setup containing at least a logical device and a swap chain (and their requirements)
     * @return std::vector<VkImage> A vector of vulkan images corresponding to the setup's swap chain's images
     */
    std::vector<VkImage> retrieve_swap_chain_images(const InstanceSetup &setup);

    /**
     * @brief Creates vulkan image views for the swap chain's images of a setup
     * 
     * @param setup A setup containing at least a swap chain configuration and a logical device (and their requirements)
     * @return std::vector<VkImageView> A vector of vulkan image views corresponding to the setup's swap chain's images
     */
    std::vector<VkImageView> create_swap_chain_image_views(const InstanceSetup &setup);

    /**
     * @brief Create a vulkan descriptor set layout to bind an UniformBufferObject and a sampler
     * 
     * @param setup A setup containing at least a logical device (and it's requirements)
     * @return VkDescriptorSetLayout The created descriptor set layout
     */
    VkDescriptorSetLayout create_descriptor_set_layout(const InstanceSetup &setup);

    /**
     * @brief Creates a set of vulkan command pools do
     * 
     * @param setup A setup containing a least graphics and transfer queue family indices and a logical device (and their requirements)
     * @return CommandPools The created set of command pools
     */
    CommandPools create_command_pool(const InstanceSetup &setup);

    /**
     * @brief Creates an image intended for being the swap chain's presented image (multisampling result)
     * 
     * @param setup A setup containing at least a swap chain configuration and a max sample flag (and their requirements)
     * @return ViewableImage The created image
     */
    ViewableImage create_color_image(const InstanceSetup &setup);

    /**
     * @brief Creates a texture
     * 
     * @param setup A setup containing at lest a logical device, a physical device, and queue families indices (and their requirements)
     * @param width The width of the texture to create
     * @param height The height of the texture to create
     * @param flags The sample count flag of the texture to create
     * @param mipLevels The mipmap amount of the texture to create
     * @param depthFormat The "color" format of the texture to create (RGB, greyscale...)
     * @param usage The vulkan usage flags of the texture to create
     * @return WrappedTexture The created texture
     */
    WrappedTexture create_texture(const InstanceSetup &setup, int width, int height, VkSampleCountFlagBits flags, uint32_t mipLevels, VkFormat depthFormat, VkImageUsageFlags usage);
    
    /**
     * @brief Create an image view for a given texture
     * 
     * @param setup A setup containing at least a logical device (and it's requirements)
     * @param texture The texture for which to create an image view
     * @param format The format of the image view to create
     * @param mipLevels The mipmap amount of the image view to create
     * @return VkImageView The created image view
     */
    VkImageView create_texture_image_view(const InstanceSetup &setup, const WrappedTexture &texture, const VkFormat &format, uint32_t mipLevels);
    
    /**
     * @brief Creates a depth buffer for a given setup
     * 
     * @param setup A setup containing at least a swap chain configuration, a logical device, and a max samples flag (and their requirements)
     * @return DepthBuffer The created depth buffer
     */
    DepthBuffer create_depth_buffer(const InstanceSetup &setup);
    
    /**
     * @brief Finds and returns formats supported for a setup among a list of candidate formats, considering a tiling mode and a list of required features
     * 
     * @param setup A setup containing at least a physical device (and it's requirements)
     * @param candidates A vector containing the candidates
     * @param tiling The tiling mode for which to fetch format compatibility info
     * @param features The required features
     * @return std::vector<VkFormat> A list of all candidates fulfilling the requirements
     */
    std::vector<VkFormat> find_supported_formats(const InstanceSetup &setup, const std::vector<VkFormat> &candidates, const VkImageTiling &tiling, const VkFormatFeatureFlags &features);
    
    /**
     * @brief Create a graphics pipeline for a setup, compiling a shading program on the fly
     * 
     * @param setup A setup containing at least a swap chain congiguration, a max samples flag, a descriptor set layout, and a logical device (and their requirements)
     * @param vertexShaderFilename The vertex stage's source's filename for the pipeline's shader
     * @param fragmentShaderFilename The fragment stage's source's filename for the pipline's shader
     * @return GraphicsPipelineConfig The created graphics pipeline
     */
    GraphicsPipelineConfig create_graphics_pipeline(const InstanceSetup &setup, const std::string &vertexShaderFilename, const std::string &fragmentShaderFilename);
    
    /**
     * @brief Creates a shader module given a compiled shader
     * 
     * @param setup A setup containing at least a logical device (and it's requirements)
     * @param compiledShader A shader compiled to SPIR-V and linked
     * @return VkShaderModule The created shader module
     */
    VkShaderModule create_shader_module(const InstanceSetup &setup, const shaderc::SpvCompilationResult &compiledShader);
    
    /**
     * @brief Creates a render pass for a setup, binding vertex data for the pipeline
     * 
     * @param setup A setup containing at least a swap chain configuration, a logical deive, a depth buffer, and a max samples flag (and their requirements)
     * @return VkRenderPass The created render pass
     */
    VkRenderPass create_render_pass(const InstanceSetup &setup);
    
    /**
     * @brief Creates framebuffers for a setup
     * 
     * @param setup A setup containing at least a graphics pipeline, a swap chain configuration, a logical device and a depth buffer (and their requirements)
     * @return std::vector<VkFramebuffer> A list of created framebuffers
     */
    std::vector<VkFramebuffer> create_framebuffers(const InstanceSetup &setup);
    
    /**
     * @brief Creates a texture from an (RGBA)image file, considering a setup
     * 
     * @param setup A setup containing at least a logical device (and it's requirements)
     * @param textureFilename The image's filename
     * @return WrappedTexture The created texture
     */
    WrappedTexture create_texture_from_image(const InstanceSetup &setup, const std::string &textureFilename);
    
    /**
     * @brief Memory-safely copies a source wrapped vulkan buffer's content to another's, considering a setup, using memory mapping
     * 
     * @param setup A setup containing at least a logical device, a transfer command pool, and a transfer queue (and their requirements)
     * @param source The source wrapped vulkan buffer
     * @param dest The destination wrapped vulkan buffer
     */
    void copy_buffer(const InstanceSetup &setup, const WrappedBuffer &source, WrappedBuffer *dest);
    
    /**
     * @brief Creates a wrapped vulkan data buffer for a setup, considering size, usage and required memory properties
     * 
     * @param setup A setup containing at least a logical device and a graphics queue (and their requirements)
     * @param sizeInBytes The number of bytes the buffer should be able to contain
     * @param usage The usage flags for the buffer
     * @param properties The memory properties required for the buffer
     * @return WrappedBuffer The create wrapped vulkan data buffer
     */
    WrappedBuffer create_buffer(const InstanceSetup &setup, VkDeviceSize sizeInBytes, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    
    /**
     * @brief Transitions an image (in-place) from a specified old layout to a specified new layout, considering a setup and preserving mipmaps
     * 
     * @param setup A setup containig at least command pools and a graphics queue (and their requirements)
     * @param texture The texture to transition
     * @param format The texture's format
     * @param oldLayout The texture's "current" (/undesired) layout
     * @param newLayout The texture's "next" (/desired) layout
     * @param mipLevels The texture's mipmap level
     */
    void transition_image_layout(const InstanceSetup &setup, WrappedTexture *texture, const VkFormat &format, const VkImageLayout &oldLayout, const VkImageLayout &newLayout, uint32_t mipLevels);
    
    /**
     * @brief Creates, begins and returns a one-shot command buffer, considering a setup and a selected command pool (usually graphics or transfer)
     * 
     * @param setup A setup containing a logical device (and it's requirements)
     * @param selectedPool The selected command pool to create the one-shot command buffer for
     * @return VkCommandBuffer The created one-shot command buffer
     */
    VkCommandBuffer begin_one_shot_command(const InstanceSetup &setup, const VkCommandPool &selectedPool);
    
    /**
     * @brief Ends, submits and syncs a given one-shot command buffer, considering it 's setup and selected pool
     * 
     * @param setup A setup containing at least a logical device (and it's requirements)
     * @param selectedPool The command pool specified for the one-shot command buffer
     * @param selectedQueue The queue specified for the one-shot command buffer
     * @param osCommandBuffer A pointer to the one-shot command buffer
     */
    void end_one_shot_command(const InstanceSetup &setup, const VkCommandPool &selectedPool, const VkQueue &selectedQueue, VkCommandBuffer *osCommandBuffer);
    
    /**
     * @brief Creates a texture sampler, considering a setup and a mipmap level
     * 
     * @param setup A setup containing at least a physical device and a logical device (and their requirements)
     * @param mipLevels The sampler's mipmap level
     * @return VkSampler The created sampler
     */
    VkSampler create_texture_sampler(const InstanceSetup &setup, std::optional<uint32_t> mipLevels);
    
    /**
     * @brief Copies a general purpose vulkan data buffer's content to an image's data buffer
     * 
     * @param setup A setup containig at least a graphics command pool and a graphics queue (and their requirements)
     * @param dataSource The general purpose vulkan data buffer to use as a source
     * @param image The image to use as a destination
     * @param width The image's width
     * @param height The image's height
     */
    void copy_buffer_to_image(const InstanceSetup &setup, const WrappedBuffer &dataSource, VkImage *image, uint32_t width, uint32_t height);
    
    /**
     * @brief Generates mipmaps (in-place) for a specified texture, considering a setup and parameters
     * 
     * @param setup A setup containing at least a graphics command pool, a graphics queue, and a physical device (and their requirements)
     * @param image The image for which to generate mipmaps
     * @param format The image's and mipmap's format
     * @param width The image's base width
     * @param height The image's base height
     * @param mipLevels The amount of mipmap to generate
     */
    void generate_mipmaps(const InstanceSetup &setup, const VkImage &image, const VkFormat &format, int width, int height, uint32_t mipLevels);
    
    /**
     * @brief Creates and fills a wrapped vulkan buffer intended to be used as a vertex buffer for a specified setup, using a staging buffer
     * 
     * @param setup A setup containing at least a logical device (and it's requirements)
     * @param vertices The vertices to fill the vertex buffer with
     * @return WrappedBuffer The created and filled wrapped vertex buffer
     */
    WrappedBuffer create_vertex_buffer(const InstanceSetup &setup, const std::vector<Vertex3D> &vertices);
    
    /**
     * @brief Creates and fills a wrapped vulkan buffer intended to be used as an index buffer for a specified setup, using a staging buffer
     * 
     * @param setup A setup containing at least a logical device (and it's requirements)
     * @param indices The indices to fill the index buffer with
     * @return WrappedBuffer The created and filled wrapped index buffer
     */
    WrappedBuffer create_index_buffer(const InstanceSetup &setup, const std::vector<uint32_t> &indices);
    
    /**
     * @brief Creates wrapped vulkan buffers intended to be used as uniform buffer objects for a specified setup
     * 
     * @param setup A setup containing at least a logical device (and it's requirements)
     * @return std::vector<WrappedBuffer> A list containing all created wrapped uniform buffers
     */
    std::vector<WrappedBuffer> create_uniform_buffers(const InstanceSetup &setup);
    
    /**
     * @brief Creates a descriptor pool considering a setup, binding uniform buffers, samplers.
     * 
     * @param setup A setup containing at least a logical device (and it's requrements)
     * @return VkDescriptorPool The created descriptor pool
     */
    VkDescriptorPool create_descriptor_pool(const InstanceSetup &setup);
    
    /**
     * @brief Creates a list of descriptor sets for a descriptor pool
     * 
     * @param setup A setup containing at least a descriptor pool, a logical device, uniform buffers, texture views and texture samplers (and their requirements)
     * @return std::vector<VkDescriptorSet> A list containing all created descriptor sets
     */
    std::vector<VkDescriptorSet> create_descriptor_sets(const InstanceSetup &setup);
    
    /**
     * @brief Creates draw-purposed command buffers for a setup's graphics queue
     * 
     * @param setup A setup containing at least a logical device and a graphics command pool (and their requirements)
     * @return std::vector<VkCommandBuffer> A list containing all created draw-purposed command bufffers
     */
    std::vector<VkCommandBuffer> create_command_buffers(const InstanceSetup &setup);
    
    /**
     * @brief Create a basic set of synchroniation helper objects for a setup
     * 
     * @param setup A setup containing at least a logical device (and it's requirements)
     * @return BaseSyncObjects The set of created sync objects
     */
    BaseSyncObjects create_base_sync_objects(const InstanceSetup &setup);
    
        /*----------------------------*
        *- FUNCTIONS: Setup cleanup -*
        *----------------------------*/
   
    /**
     * @brief Explicitely destroys every contained object in the setup, in the right order
     * 
     * @param setup A complete setup
     */
    void clean_setup(const InstanceSetup &setup);
    
    /**
     * @brief Explicitely destroys a setup's swap chain
     * 
     * @param setup A setup with a complete swap chain
     */
    void cleanup_swap_chain(const InstanceSetup &setup);
   
        /*----------------------------*
         *- FUNCTIONS: Setup drawing -*
         *----------------------------*/
    
    /**
     * @brief Draws a frame considering a setup, the destination window handle, and a current frame ID
     * 
     * @param setup A pointer to a complete setup
     * @param window A pointer to the destination window
     * @param currentFrame A pointer to the current frame's ID (which will be incremented+modulo'd just before the draw ends)
     */
    void draw_frame(InstanceSetup *setup, GLFWwindow *window, size_t *currentFrame);
    
    /**
     * @brief Explicitely destroys, then recreates a setup's swap chain
     * 
     * @param setup A setup containing at least a complete swap chain (and it's requirements)
     * @param window A window handle
     */
    void recreate_swap_chain(InstanceSetup *setup, GLFWwindow *window);
    
    /**
     * @brief Updates a setup's uniform buffer object considering a frame number
     * 
     * @param setup A setup containing at least uniform buffers
     * @param frame A frame ID
     */
    void update_uniform_buffer(const InstanceSetup &setup, size_t frame);
    
    /**
     * @brief Records a command buffer for rendering
     * 
     * @param setup A complete setup
     * @param commandBuffer A command buffer to record
     * @param imageIndex An image index
     * @param currentFrame A frame ID
     */
    void record_command_buffer(const InstanceSetup &setup, const VkCommandBuffer &commandBuffer, uint32_t imageIndex, size_t currentFrame);
  
        /*---------------------*
         *- FUNCTIONS: helper -*
         *---------------------*/
    
    /**
     * @brief Loads a model considering it's filename
     * 
     * @param filename Name of the file to load as a model
     * @return LoadedModel The loaded as loaded in the memory
     */
    LoadedModel load_model(const std::string &filename);
    
    /**
     * @brief Finds suitable memory type considering type filters and required properties, for a specified physical device
     * 
     * @param device The physical device to fetch memory info from
     * @param typeFilter The suitable types
     * @param properties The requred properties
     * @return uint32_t The found suitable memory type
     */
    uint32_t find_memory_type(const VkPhysicalDevice &device, uint32_t typeFilter, const VkMemoryPropertyFlags &properties);
    
    /**
     * @brief Compiles a shader in-memory from it's source's filename to SPIR-V bytecode
     * 
     * @param filename The source's filename
     * @param shaderKind The shader stage (vertex, fragment, compute...)
     * @return shaderc::SpvCompilationResult The resultat SPIR-V bytecode's wrapper
     */
    shaderc::SpvCompilationResult compile_shader(const std::string &filename, const shaderc_shader_kind &shaderKind);
}
