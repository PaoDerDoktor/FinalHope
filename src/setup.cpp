#include "setup.hpp"

#include <stdexcept>

namespace fhope {
    static VKAPI_ATTR VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void *pUserData) {
        std::cerr << "Validation Layer : " << pCallbackData->pMessage << std::endl;
    
        return VK_FALSE;
    }



    void initialize_dependencies() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        int gladVkVersion = gladLoaderLoadVulkan(NULL, NULL, NULL);
        if (!gladVkVersion) {
            std::runtime_error("Unable to load Vulkan symbols from glad");
        }
    }


    void terminate_dependencies() {
        glfwTerminate();
    }



    VulkanInstanceSetup init_instance(std::string appName, std::tuple<uint32_t, uint32_t, uint32_t> appVersion, bool debug) {
        std::cout << std::string(fhope::ENGINE_NAME) << std::endl;
        const char *engineNameC = fhope::ENGINE_NAME.data();
        
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(std::get<0>(appVersion), std::get<1>(appVersion), std::get<2>(appVersion));
        appInfo.pEngineName = engineNameC;
        appInfo.engineVersion = VK_MAKE_VERSION(std::get<0>(fhope::ENGINE_VERSION), std::get<1>(fhope::ENGINE_VERSION), std::get<2>(fhope::ENGINE_VERSION));
        appInfo.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo instanceCreateInfo{};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount(0);
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        
        std::vector<const char*> enabledExtensions(glfwExtensions, glfwExtensions+glfwExtensionCount);

        uint32_t instanceExtensionCount(0);
        vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
        std::vector<VkExtensionProperties> instanceExtensions(instanceExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensions.data());
        
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        const std::vector<const char *> validationLayers = { "VK_LAYER_KHRONOS_validation" };
        
        if (debug) {
            enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

            bool layerFound;
            for (const char *layerName : validationLayers) {
                for (const VkLayerProperties &layerProperties : availableLayers) {
                    if (strcmp(layerName, layerProperties.layerName)) {
                        layerFound = true;
                        break;
                    }
                }
            }

            if (!layerFound) {
                std::runtime_error("Could not find any of requested validation layer");
            }

            instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            instanceCreateInfo.enabledLayerCount = 0;
        }

        VulkanInstanceSetup setup{};

        instanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(enabledExtensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

        if (vkCreateInstance(&instanceCreateInfo, nullptr, &setup.instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create the instance");
        }

        gladLoaderLoadVulkan(setup.instance, NULL, NULL);
        VkDebugUtilsMessengerEXT messenger;

        if (debug) {
            VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo{};
            messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            messengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            messengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            messengerCreateInfo.pfnUserCallback = debug_callback;
            messengerCreateInfo.pUserData = nullptr;
            

            vkCreateDebugUtilsMessengerEXT(setup.instance, &messengerCreateInfo, nullptr, &messenger);

            setup.debugMessenger.emplace(messenger);
        }

        return setup;
    }



    bool is_physical_device_suitable(VkPhysicalDevice device, std::vector<VkPhysicalDeviceType> possibleTypes) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        bool typeCheck = std::find(possibleTypes.begin(), possibleTypes.end(), properties.deviceType) != possibleTypes.end();

        const std::vector<const char *> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const VkExtensionProperties extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        bool extensionCheck = requiredExtensions.empty();



        return typeCheck && extensionCheck;
    }



    int64_t score_physical_device(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures   features;

        vkGetPhysicalDeviceProperties(device, &properties);
        vkGetPhysicalDeviceFeatures(device, &features);

        // TODO : Implement better scoring
        int64_t score(0);

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        return score;
    }



    std::optional<VkPhysicalDevice> autopick_physical_device(VulkanInstanceSetup *setup) {
        uint32_t physicalDeviceCount;
        vkEnumeratePhysicalDevices(setup.instance, &physicalDeviceCount, nullptr);

        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        vkEnumeratePhysicalDevices(setup.instance, &physicalDeviceCount, physicalDevices.data());

        int64_t bestScore(-1);
        std::optional<VkPhysicalDevice> bestDevice(std::nullopt);
        
        for (VkPhysicalDevice device : physicalDevices) {
            int64_t deviceScore = score_physical_device(device);

            if (deviceScore > bestScore) {
                bestScore = deviceScore;
                bestDevice.emplace(device);
            }
        }

        return bestDevice;
    }



    void find_queues(VulkanInstanceSetup *setup) {
        uint32_t queueFamilyCount(0);
        vkGetPhysicalDeviceQueueFamilyProperties(setup->physicalDevice.value(), &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(setup->physicalDevice.value(), &queueFamilyCount, queueFamilies.data());
        
        for (size_t i(0); i != queueFamilyCount; ++i) {
            VkQueueFamilyProperties queueFamily = queueFamilies[i];

            if (!setup->queues.graphicsQueueIndex.has_value() && queueFamily.queueFlags&VK_QUEUE_GRAPHICS_BIT) {
                setup->queues.graphicsQueueIndex.emplace(i);
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(setup->physicalDevice.value(), i, setup->surface.value(), &presentSupport);

            if (presentSupport) {
                setup->queues.presentQueueIndex.emplace(i);
            }
        }
    }



    void populate_swap_chain_support(VulkanInstanceSetup *setup) {
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(setup->physicalDevice.value(), setup->surface.value(), &capabilities);

        setup->swapChain.swapChainCapabilities.emplace(capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(setup->physicalDevice.value(), setup->surface.value(), &formatCount, nullptr);
        setup->swapChain.swapChainFormats.reserve(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(setup->physicalDevice.value(), setup->surface.value(), &formatCount, setup->swapChain.swapChainFormats.data());

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(setup->physicalDevice.value(), setup->surface.value(), &presentModeCount, nullptr);
        setup->swapChain.swapChainFormats.reserve(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(setup->physicalDevice.value(), setup->surface.value(), &presentModeCount, setup->swapChain.swapChainPresentModes.data());
    }



    VkDevice create_logical_device(const VulkanInstanceSetup &setup) {
        VkDevice logicalDevice;


        std::cout << "starting queue search" << std::endl;

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::vector<std::pair<uint32_t, const float*>> queuesToCreate;
    
        if (setup.queues.graphicsQueueIndex.has_value()) {
            queuesToCreate.push_back(std::make_pair(setup.queues.graphicsQueueIndex.value(), &setup.queues.graphicsPriority));
        }

        if (setup.queues.presentQueueIndex.has_value() && setup.queues.presentQueueIndex.value() != setup.queues.graphicsQueueIndex.value_or(setup.queues.presentQueueIndex.value()+1)) {
            queuesToCreate.push_back(std::make_pair(setup.queues.presentQueueIndex.value(), &setup.queues.presentPriority));
        }

        for (std::pair<uint32_t, const float *> queueToCreate : queuesToCreate) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueToCreate.first;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = queueToCreate.second;

            queueCreateInfos.push_back(queueCreateInfo);
        }
        
        VkPhysicalDeviceFeatures physicalFeatures{};

        VkDeviceCreateInfo deviceCreateInfo{};

        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queuesToCreate.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures = &physicalFeatures;

        if (vkCreateDevice(setup.physicalDevice.value(), &deviceCreateInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't create Logical Device");
        }

        std::cout << "end of queues search" << std::endl;

        return logicalDevice;
    }



    void clean_setup(VulkanInstanceSetup setup) {
        vkDestroyDevice(setup.logicalDevice.value(), nullptr);

        vkDestroySurfaceKHR(setup.instance, setup.surface.value(), nullptr);

        if (setup.debugMessenger.has_value()) {
            vkDestroyDebugUtilsMessengerEXT(setup.instance, setup.debugMessenger.value(), nullptr);
        }

        vkDestroyInstance(setup.instance, nullptr);
    }
}
