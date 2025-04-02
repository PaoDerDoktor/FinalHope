#pragma once

#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <set>

#include <glad/vulkan.h>
#include <GLFW/glfw3.h>



namespace fhope {
    /***********************
     ** GLOBALS CONSTANTS **
     ***********************/

    inline constexpr std::string_view ENGINE_NAME = "FinalHope"; // Name of the engine
    inline constexpr std::tuple<uint32_t, uint32_t, uint32_t> ENGINE_VERSION = {0, 0, 1}; // Current engine version

    /****************
     ** STRUCTURES **
     ****************/
    
    struct VulkanQueueSetup {
        std::optional<uint32_t> graphicsQueueIndex = std::nullopt;
        std::optional<uint32_t> presentQueueIndex  = std::nullopt;

        float graphicsPriority = 1.0f;
        float presentPriority  = 1.0f;
    };

    struct VulkanSwapChainSetup {
        std::optional<VkSurfaceCapabilitiesKHR> swapChainCapabilities;
        std::vector<VkSurfaceFormatKHR> swapChainFormats;
        std::vector<VkPresentModeKHR> swapChainPresentModes;
    };
    
    /**
     * @brief Group of values used as a setup fore a vulkan instance
     */
    struct VulkanInstanceSetup {
        VkInstance instance = VK_NULL_HANDLE; // Vulkan instance
        std::optional<VkDebugUtilsMessengerEXT> debugMessenger = std::nullopt; // Debugger of the instance

        std::optional<VkPhysicalDevice> physicalDevice = std::nullopt;

        std::optional<VkDevice> logicalDevice = std::nullopt;

        std::optional<VkSurfaceKHR> surface = std::nullopt;

        VulkanQueueSetup queues = {};
        VulkanSwapChainSetup swapChain = {};
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

    /**
     * @brief Prepares and returns an instance and it's setup
     * 
     * @param appName Name of the application to run on the instance
     * @param appVersion Version of the application to run on the instance
     * @param debug Wether ot not debug mode is on
     * @return VulkanInstanceSetup The vulkan instances and required companion values
     */
    VulkanInstanceSetup init_instance(std::string appName, std::tuple<uint32_t, uint32_t, uint32_t> appVersion, bool debug=true);

    //TODO: Implement more checks for physical device suitability

    /**
     * @brief Checks wether or not a given physical device is suitable to use considering given requirements
     * 
     * @param device The physical device to check
     * @param possibleTypes The types of physical device we can accept as suitable
     * @return true if the physical device is suitable
     * @return false if the physical device isn't suitable
     */
    bool is_physical_device_suitable(VkPhysicalDevice device, std::vector<VkPhysicalDeviceType> possibleTypes);

    /**
     * @brief Scores a given physical device
     * 
     * @param device The device to score
     * @return int64_t An integer representing the expected performances of the given physical device
     */
    int64_t score_physical_device(VkPhysicalDevice device);

    /**
     * @brief Automatically chooses a physical device from every one a given instance has access to
     * 
     * @param instance The instance to automatically choose a physical device for
     * @return std::optional<VkPhysicalDevice> The best physical device available for the given instance, if any has been deemed suitable
     */
    std::optional<VkPhysicalDevice> autopick_physical_device(VulkanInstanceSetup setup);
    
    /**
     * @brief Find queues for a given vulkan instance setup in-place
     * 
     * @param setup The setup to populate queues for
     */
    void find_queues(VulkanInstanceSetup *setup);

    /**
     * @brief Queries the swap chain support of a given setup and populates it with the info
     * 
     * @param setup The stup to query/populate swap chain support info for
     */
    void populate_swap_chain_support(VulkanInstanceSetup *setup);

    /**
     * @brief Create a logical device object considering a given instance setup
     * @param setup the instance setup
     * 
     * @return VkDevice The created logical device
     */
    VkDevice create_logical_device(const VulkanInstanceSetup &setup);

    /**
     * @brief Cleans a setup by shutting down it's components
     * 
     * @param setup The setup to shut down
     */
    void clean_setup(VulkanInstanceSetup setup);
}
