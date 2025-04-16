#include "setup.hpp"

#include <limits>
#include <algorithm>
#include <chrono>

#include <glm/gtc/matrix_transform.hpp>

namespace fhope {
    /*************
     ** METHODS **
     *************/

    bool QueueSetup::is_complete() const {
        return (graphicsIndex.has_value() && presentIndex.has_value() && transferIndex.has_value());
    }

    /***************
     ** CALLBACKS **
     ***************/

    static VKAPI_ATTR VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void *pUserData) {
        std::cerr << "Validation Layer : " << pCallbackData->pMessage << std::endl;
        *((std::ofstream *)pUserData) << "[" << messageSeverity << "] - " << pCallbackData->pMessage << std::endl << std::flush;

        return VK_FALSE;
    }

    /***************
     ** FUNCTIONS **
     ***************/

        /*------------------------------------*
         *- FUNCTIONS: Dependency management -*
         *------------------------------------*/

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

        /*-------------------------------*
         *- FUNCTIONS: Setup generation -*
         *-------------------------------*/

    InstanceSetup create_instance(std::string appName, std::tuple<uint32_t, uint32_t, uint32_t> appVersion) {
        std::cout << std::string(fhope::ENGINE_NAME) << std::endl;
        const char *engineNameC = fhope::ENGINE_NAME;
        
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
        

        //#ifdef DEBUG
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
        //#else
        //    instanceCreateInfo.enabledLayerCount = 0;
        //#endif

        InstanceSetup setup{};

        instanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(enabledExtensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

        if (vkCreateInstance(&instanceCreateInfo, nullptr, &setup.instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create the instance");
        }

        gladLoaderLoadVulkan(setup.instance, NULL, NULL);
        VkDebugUtilsMessengerEXT messenger;

        //#ifdef DEBUG
            VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo{};
            messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            messengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            messengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
            messengerCreateInfo.pfnUserCallback = debug_callback;
            messengerCreateInfo.pUserData = (void*)new std::ofstream("./log.txt");

            vkCreateDebugUtilsMessengerEXT(setup.instance, &messengerCreateInfo, nullptr, &messenger);

            setup.debugMessenger.emplace(messenger);
        //#endif

        return setup;
    }



    InstanceSetup generate_vulkan_setup(GLFWwindow *window, std::string appName, std::tuple<uint32_t, uint32_t, uint32_t> appVersion, const std::string &vertexShaderFilename, const std::string &fragmentShaderFilename, const std::string &textureFilename, const std::string &modelFilename) {
        glfwMakeContextCurrent(window);
        
        InstanceSetup newSetup = create_instance(appName, appVersion);
        
        newSetup.surface.emplace(get_surface_from_window(newSetup, window));

        newSetup.physicalDevice = autopick_physical_device(newSetup);

        newSetup.maxSamplesFlag = get_max_multisampling_level(newSetup);

        if (!newSetup.physicalDevice.has_value()) {
            throw std::runtime_error("Could not find any suitable physical device for new setup.");
        }

        newSetup.queues.emplace(find_queue_families(newSetup, newSetup.physicalDevice.value()));

        newSetup.swapChainSupport.emplace(check_swap_chain_support(newSetup, newSetup.physicalDevice.value()));
        
        newSetup.logicalDevice.emplace(create_logical_device(&newSetup));
        
        VkQueue q{}; // Querying proper vulkan queues

        vkGetDeviceQueue(newSetup.logicalDevice.value(), newSetup.queues.value().graphicsIndex.value(), 0, &q);
        newSetup.graphicsQueue.emplace(q);

        vkGetDeviceQueue(newSetup.logicalDevice.value(), newSetup.queues.value().presentIndex.value(), 0, &q);
        newSetup.presentQueue.emplace(q);

        vkGetDeviceQueue(newSetup.logicalDevice.value(), newSetup.queues.value().transferIndex.value(), 0, &q);
        newSetup.transferQueue.emplace(q);

        // FIXME: Check removing
        // Getting a swap chain
        //newSetup.swapChainSupport.emplace(check_swap_chain_support(newSetup, newSetup.physicalDevice.value()));
        
        newSetup.swapChainConfig.emplace(prepare_swap_chain_config(newSetup, window));
        
        newSetup.swapChain.emplace(create_swap_chain(newSetup, window));

        newSetup.swapChainImages = retrieve_swap_chain_images(newSetup);

        newSetup.swapChainImageViews = create_swap_chain_image_views(newSetup);

        newSetup.uniformLayout.emplace(create_descriptor_set_layout(newSetup));
        
        newSetup.commandPools.emplace(create_command_pool(newSetup));
        
        newSetup.colorImage.emplace(create_color_image(newSetup));

        newSetup.depthBuffer.emplace(create_depth_buffer(newSetup));
        
        newSetup.graphicsPipelineConfig.emplace(create_graphics_pipeline(newSetup, vertexShaderFilename, fragmentShaderFilename));

        newSetup.swapChainFramebuffers = create_framebuffers(newSetup);

        newSetup.texture.emplace(create_texture_from_image(newSetup, textureFilename));
        newSetup.textureView.emplace(create_texture_image_view(newSetup, newSetup.texture.value(), VK_FORMAT_R8G8B8A8_SRGB, newSetup.texture.value().mipLevels.value()));
        
        newSetup.textureSampler.emplace(create_texture_sampler(newSetup, newSetup.texture.value().mipLevels));

        LoadedModel newModel = load_model(modelFilename);

        newSetup.vertexBuffer.emplace(create_vertex_buffer(newSetup, newModel.vertices));

        newSetup.indexBuffer.emplace(create_index_buffer(newSetup, newModel.indices));
        newSetup.indexCount = newModel.indices.size();

        newSetup.uniformBuffers = create_uniform_buffers(newSetup);
        
        newSetup.descriptorPool.emplace(create_descriptor_pool(newSetup));

        newSetup.descriptorSets = create_descriptor_sets(newSetup);

        newSetup.commandBuffers = create_command_buffers(newSetup);

        newSetup.syncObjects.emplace(create_base_sync_objects(newSetup));

        return newSetup;
    }

    

    VkSurfaceKHR get_surface_from_window(const InstanceSetup &setup, GLFWwindow *source) {
        VkSurfaceKHR newSurface;
        glfwCreateWindowSurface(setup.instance, source, nullptr, &newSurface);

        return newSurface;
    }



    std::optional<VkPhysicalDevice> autopick_physical_device(const InstanceSetup &setup) {
        // Enumerating physical devices
        uint32_t physicalDeviceCount;
        vkEnumeratePhysicalDevices(setup.instance, &physicalDeviceCount, nullptr);
        std::vector<VkPhysicalDevice> availablePhysicalDevices(physicalDeviceCount);
        vkEnumeratePhysicalDevices(setup.instance, &physicalDeviceCount, availablePhysicalDevices.data());

        std::optional<VkPhysicalDevice> bestPhysicalDevice = std::nullopt;
        int32_t bestScore(-1);

        for (const VkPhysicalDevice &physicalDevice : availablePhysicalDevices) {
            if (is_physical_device_suitable(setup, physicalDevice)) {
                int32_t deviceScore = score_physical_device(setup, physicalDevice);

                if (deviceScore > bestScore) {
                    bestScore = deviceScore;
                    bestPhysicalDevice.emplace(physicalDevice);
                }
            }
        }
        
        return bestPhysicalDevice;
    }

    

    bool is_physical_device_suitable(const InstanceSetup &setup, const VkPhysicalDevice &physicalDevice) {
        QueueSetup queues     = find_queue_families(setup, physicalDevice);
        bool       extensions = check_physical_device_extension_support(setup, physicalDevice);
        
        bool adequateSwapChain(false);
        if (extensions) {
            SwapChainSupport swapChain = check_swap_chain_support(setup, physicalDevice);

            adequateSwapChain = !swapChain.formats.empty() && !swapChain.presentModes.empty();
        }

        VkPhysicalDeviceFeatures physicalFeatures;
        vkGetPhysicalDeviceFeatures(physicalDevice, &physicalFeatures);

        return queues.is_complete() && extensions && adequateSwapChain && physicalFeatures.samplerAnisotropy;
    }



    QueueSetup find_queue_families(const InstanceSetup &setup, const VkPhysicalDevice &physicalDevice) {
        if (!setup.surface.has_value()) {
            throw std::runtime_error("Tried to find suitable queue families without specifying a surface in setup.");
        }

        QueueSetup queues{};

        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        for (size_t i = 0; i != queueFamilyCount; ++i) {
            const VkQueueFamilyProperties &properties = queueFamilies[i];

            if (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) { // Queue supports graphics
                queues.graphicsIndex = i;
            }

            if ((properties.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                queues.transferIndex = i;
            }

            VkBool32 presentSupport;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, setup.surface.value(), &presentSupport);

            if (presentSupport) {
                queues.presentIndex = i;
            }

            if (queues.is_complete()) {
                break;
            }
        }

        if (!queues.transferIndex.has_value()) {
            queues.transferIndex = queues.graphicsIndex.value();
        }

        return queues;
    }



    bool check_physical_device_extension_support(const InstanceSetup &setup, const VkPhysicalDevice &physicalDevice) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availablePhysicalDeviceExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availablePhysicalDeviceExtensions.data());

        std::set<std::string> requiredExtensions(ENGINE_REQUIRED_DEVICE_EXTENSIONS.begin(), ENGINE_REQUIRED_DEVICE_EXTENSIONS.end());

        for (const VkExtensionProperties &availablePhysicalDeviceExtension : availablePhysicalDeviceExtensions) {
            requiredExtensions.erase(availablePhysicalDeviceExtension.extensionName);
        }

        return requiredExtensions.empty();        
    }



    SwapChainSupport check_swap_chain_support(const InstanceSetup &setup, const VkPhysicalDevice &physicalDevice) {
        if (!setup.surface.has_value()) {
            throw std::runtime_error("Tried to query swap chain support without specifying a surface in the setup.");
        }
        
        SwapChainSupport support{};

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, setup.surface.value(), &support.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, setup.surface.value(), &formatCount, nullptr);
        support.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, setup.surface.value(), &formatCount, support.formats.data());

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, setup.surface.value(), &presentModeCount, nullptr);
        support.presentModes = std::vector<VkPresentModeKHR>(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, setup.surface.value(), &presentModeCount, support.presentModes.data());

        return support;
    }



    int32_t score_physical_device(const InstanceSetup &setup, const VkPhysicalDevice &physicalDevice) {
        int32_t score;

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        return score;
    }



    VkSampleCountFlagBits get_max_multisampling_level(const InstanceSetup &setup) {
        if (!setup.physicalDevice.has_value()) {
            throw std::runtime_error("Tried to get max multisampling level without providing a physical device in the setup.");
        }
        
        VkPhysicalDeviceProperties physicalProperties;
        vkGetPhysicalDeviceProperties(setup.physicalDevice.value(), &physicalProperties);

        VkSampleCountFlags counts = physicalProperties.limits.framebufferColorSampleCounts & physicalProperties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT)  { return VK_SAMPLE_COUNT_8_BIT;  }
        if (counts & VK_SAMPLE_COUNT_4_BIT)  { return VK_SAMPLE_COUNT_4_BIT;  }
        if (counts & VK_SAMPLE_COUNT_2_BIT)  { return VK_SAMPLE_COUNT_2_BIT;  }

        return VK_SAMPLE_COUNT_1_BIT;
    }



    VkDevice create_logical_device(InstanceSetup *setup) {
        if (!setup->physicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a logical device without specifying any physical device in the setup.");
        }

        if (!setup->queues.has_value()) {
            throw std::runtime_error("tried to create a logical device without specifying any queues in the setup.");
        }

        std::set<uint32_t> uniqueQueues = { setup->queues.value().graphicsIndex.value(), setup->queues.value().presentIndex.value(), setup->queues.value().transferIndex.value() };
        setup->queues.value().priorities.resize(uniqueQueues.size(), 1.0f);

        std::vector<VkDeviceQueueCreateInfo> queuesToCreate;
        for (uint32_t queueIndex : uniqueQueues) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueIndex;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &setup->queues.value().priorities[queueIndex];

            queuesToCreate.emplace_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures physicalDeviceFeatures{};
        physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
        physicalDeviceFeatures.sampleRateShading = VK_TRUE;

        VkDeviceCreateInfo logicalDeviceCreateInfo{};
        logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        
        logicalDeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(uniqueQueues.size());
        logicalDeviceCreateInfo.pQueueCreateInfos    = queuesToCreate.data();
        
        logicalDeviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

        logicalDeviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(ENGINE_REQUIRED_DEVICE_EXTENSIONS.size());
        logicalDeviceCreateInfo.ppEnabledExtensionNames = ENGINE_REQUIRED_DEVICE_EXTENSIONS.data();

        //#ifdef DEBUG
            logicalDeviceCreateInfo.enabledLayerCount   = static_cast<uint32_t>(ENGINE_REQUIRED_VALIDATION_LAYERS.size());
            logicalDeviceCreateInfo.ppEnabledLayerNames = ENGINE_REQUIRED_VALIDATION_LAYERS.data();
        //#else
        //    logicalDeviceCreateInfo.enabledLayerCount = 0;
        //#endif // DEBUG
        
        VkDevice newDevice;
        if (vkCreateDevice(setup->physicalDevice.value(), &logicalDeviceCreateInfo, nullptr, &newDevice) != VK_SUCCESS) {
            throw std::runtime_error("Could not create logical device");
        }

        gladLoaderLoadVulkan(setup->instance, setup->physicalDevice.value(), newDevice);

        return newDevice;
    }



    SwapChainConfig prepare_swap_chain_config(const InstanceSetup &setup, GLFWwindow *window) {
        if (!setup.swapChainSupport.has_value()) {
            throw std::runtime_error("Tried to prepare a swap chain config without providing any swap chain support summary in the setup.");
        }
        
        SwapChainConfig config{};

        config.surfaceFormat = setup.swapChainSupport.value().formats[0];
        for (const VkSurfaceFormatKHR &surfaceFormat : setup.swapChainSupport.value().formats) {
            if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                config.surfaceFormat = surfaceFormat;
                break;
            }
        }

        config.presentMode = VK_PRESENT_MODE_FIFO_KHR; // Only mode guaranteed to be available
        if (std::find(setup.swapChainSupport.value().presentModes.begin(), setup.swapChainSupport.value().presentModes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != setup.swapChainSupport.value().presentModes.end()) {
            config.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }

        if (setup.swapChainSupport.value().capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            config.extent = setup.swapChainSupport.value().capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            config.extent.width  = std::clamp(static_cast<uint32_t>(width),  setup.swapChainSupport.value().capabilities.minImageExtent.width,  setup.swapChainSupport.value().capabilities.maxImageExtent.width);
            config.extent.height = std::clamp(static_cast<uint32_t>(height), setup.swapChainSupport.value().capabilities.minImageExtent.height, setup.swapChainSupport.value().capabilities.maxImageExtent.height);
        }

        config.imageCount = setup.swapChainSupport.value().capabilities.minImageCount + 1;
        if (setup.swapChainSupport.value().capabilities.maxImageCount > 0 && config.imageCount > setup.swapChainSupport.value().capabilities.maxImageCount) {
            config.imageCount = setup.swapChainSupport.value().capabilities.maxImageCount;
        }

        return config;
    }
    
    
    
    VkSwapchainKHR create_swap_chain(const InstanceSetup &setup, GLFWwindow *window) {
        if (!setup.swapChainConfig.has_value()) {
            throw std::runtime_error("Tried to create a swapchain without putting any swap chain config in the setup.");
        }
        
        if (!setup.swapChainSupport.has_value()) {
            throw std::runtime_error("Tried to create a swapchain without putting any swap chain support info in the setup.");
        }
        
        if (!setup.queues.has_value()) {
            throw std::runtime_error("Tried to create a swapchain without putting any queue indices in the setup.");
        }
        
        if (!setup.queues.value().graphicsIndex.has_value()) {
            throw std::runtime_error("Tried to create a swapchain without putting any graphics queue family index in the setup.");
        }
        
        if (!setup.queues.value().presentIndex.has_value()) {
            throw std::runtime_error("Tried to create a swapchain without putting any present queue family index in the setup.");
        }
        
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a swapchain without putting any logical device in the setup.");
        }
        
        VkSwapchainCreateInfoKHR swapChainCreateInfo{};
        swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapChainCreateInfo.surface = setup.surface.value();
        
        swapChainCreateInfo.minImageCount   = setup.swapChainConfig.value().imageCount;
        swapChainCreateInfo.imageFormat     = setup.swapChainConfig.value().surfaceFormat.format;
        swapChainCreateInfo.imageColorSpace = setup.swapChainConfig.value().surfaceFormat.colorSpace;
        swapChainCreateInfo.imageExtent     = setup.swapChainConfig.value().extent;
        
        swapChainCreateInfo.imageArrayLayers = 1;
        swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        
        const std::set<uint32_t> qs { setup.queues.value().graphicsIndex.value(), setup.queues.value().presentIndex.value(), setup.queues.value().transferIndex.value() };
        const std::vector<uint32_t> qsv(qs.begin(), qs.end());
        if (qs.size() > 1) {
            swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapChainCreateInfo.queueFamilyIndexCount = qsv.size();
            swapChainCreateInfo.pQueueFamilyIndices = qsv.data();
        } else {
            swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapChainCreateInfo.queueFamilyIndexCount = 0; // Optional
            swapChainCreateInfo.pQueueFamilyIndices = nullptr; // Optional
        }
        
        swapChainCreateInfo.preTransform = setup.swapChainSupport.value().capabilities.currentTransform;
        
        swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        
        swapChainCreateInfo.presentMode = setup.swapChainConfig.value().presentMode;
        swapChainCreateInfo.clipped     = VK_TRUE;
        
        swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
        
        VkSwapchainKHR swapChain;
        if (vkCreateSwapchainKHR(setup.logicalDevice.value(), &swapChainCreateInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't create swap chain.");
        }
        
        return swapChain;
    }


    
    std::vector<VkImage> retrieve_swap_chain_images(const InstanceSetup &setup) {
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to retrieve swap chain images without providing a logical device in the setup.");
        }
        
        if (!setup.swapChain.has_value()) {
            throw std::runtime_error("Tried to retrieve swap chain images without providing a swap chain in the setup.");
        }
        
        uint32_t imageCount;
        vkGetSwapchainImagesKHR(setup.logicalDevice.value(), setup.swapChain.value(), &imageCount, nullptr);
        std::vector<VkImage> images(imageCount);
        vkGetSwapchainImagesKHR(setup.logicalDevice.value(), setup.swapChain.value(), &imageCount, images.data());
        
        return images;
    }
    

    
    std::vector<VkImageView> create_swap_chain_image_views(const InstanceSetup &setup) {
        if (!setup.swapChainConfig.has_value()) {
            throw std::runtime_error("Tried to create swap chain image views without providing a Swap chain config in the setup.");
        }
        
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create swap chain image views without providing a logical device in the setup.");
        }
        
        std::vector<VkImageView> views(setup.swapChainImages.size());
        
        for (size_t i = 0; i != views.size(); ++i) {
            const VkImage &image = setup.swapChainImages[i];
            
            VkImageViewCreateInfo newImageViewCreateInfo{};
            
            newImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            newImageViewCreateInfo.image = image;
            newImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            newImageViewCreateInfo.format = setup.swapChainConfig.value().surfaceFormat.format;
            
            newImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            newImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            newImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            newImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            
            newImageViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            newImageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
            newImageViewCreateInfo.subresourceRange.levelCount     = 1;
            newImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            newImageViewCreateInfo.subresourceRange.layerCount     = 1;
            
            if (vkCreateImageView(setup.logicalDevice.value(), &newImageViewCreateInfo, nullptr, &views[i]) != VK_SUCCESS) {
                throw std::runtime_error("Could not create image view.");
            }
        }
        
        return views;
    }



    VkDescriptorSetLayout create_descriptor_set_layout(const InstanceSetup &setup) {
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a descriptor set layout without providingg a logical device in the setup.");
        }
        
        VkDescriptorSetLayoutBinding uboBinding{};
        uboBinding.binding = 0;
        uboBinding.descriptorCount = 1;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 1;
        samplerBinding.descriptorCount = 1;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerBinding.pImmutableSamplers = nullptr;

        std::array<VkDescriptorSetLayoutBinding, 2> descriptorBindings = { uboBinding, samplerBinding };

        VkDescriptorSetLayoutCreateInfo descriptorCreateInfo{};
        descriptorCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorCreateInfo.bindingCount = static_cast<uint32_t>(descriptorBindings.size());
        descriptorCreateInfo.pBindings = descriptorBindings.data();

        VkDescriptorSetLayout newDescriptorSetLayout;

        if (vkCreateDescriptorSetLayout(setup.logicalDevice.value(), &descriptorCreateInfo, nullptr, &newDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Could not create descriptor set layout.");
        }

        return newDescriptorSetLayout;
    }



    CommandPools create_command_pool(const InstanceSetup &setup) {
        if (!setup.queues.has_value()) {
            throw std::runtime_error("Tried to create a command pool without providing queues in the setup.");
        }

        if (!setup.queues.value().graphicsIndex.has_value()) {
            throw std::runtime_error("Tried to create a command pool without providing a graphics queue family index in the setup.");
        }

        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a command pool without providing a logical device in the setup.");
        }

        CommandPools newCommandPools;

        VkCommandPoolCreateInfo graphicsCommandPoolCreateInfo{};
        graphicsCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        graphicsCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        graphicsCommandPoolCreateInfo.queueFamilyIndex = setup.queues.value().graphicsIndex.value();

        if (vkCreateCommandPool(setup.logicalDevice.value(), &graphicsCommandPoolCreateInfo, nullptr, &newCommandPools.graphics) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't create graphics command pool.");
        }

        VkCommandPoolCreateInfo transferCommandPoolCreateInfo{};
        transferCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        transferCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        transferCommandPoolCreateInfo.queueFamilyIndex = setup.queues.value().transferIndex.value();

        if (vkCreateCommandPool(setup.logicalDevice.value(), &transferCommandPoolCreateInfo, nullptr, &newCommandPools.transfer) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't create transfer command pool.");
        }

        return newCommandPools;
    }



    ViewableImage create_color_image(const InstanceSetup &setup) {
        if (!setup.swapChainConfig.has_value()) {
            throw std::runtime_error("Tried to create color image without providing a swap chain config in the setup.");
        }

        if (!setup.maxSamplesFlag.has_value()) {
            throw std::runtime_error("Tried to create color image without providing a max sample flag in the setup.");
        }
        
        ViewableImage colorImage;

        VkFormat format = setup.swapChainConfig.value().surfaceFormat.format;

        colorImage.image = create_texture(setup, setup.swapChainConfig.value().extent.width, setup.swapChainConfig.value().extent.height, setup.maxSamplesFlag.value(), 1, format, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

        colorImage.imageView = create_texture_image_view(setup, colorImage.image, format, 1);

        return colorImage;
    }



    WrappedTexture create_texture(const InstanceSetup &setup, int width, int height, VkSampleCountFlagBits flags, uint32_t mipLevels, VkFormat depthFormat, VkImageUsageFlags usage) {
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a texture without providing a logical device in the setup.");
        }

        if (!setup.queues.has_value()) {
            throw std::runtime_error("Tried to create a texture without providing queue family indices in the setup.");
        }

        if (!setup.physicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a physical device without providing a physical device in the setup.");
        }
        
        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = width;
        imageCreateInfo.extent.height = height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = mipLevels;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.format = depthFormat;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage = usage;
        imageCreateInfo.samples = flags;
        
        const std::set<uint32_t> qs { setup.queues.value().graphicsIndex.value(), setup.queues.value().presentIndex.value(), setup.queues.value().transferIndex.value() };
        const std::vector<uint32_t> qsv(qs.begin(), qs.end());
        if (qs.size() > 1) {
            imageCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            imageCreateInfo.queueFamilyIndexCount = qsv.size();
            imageCreateInfo.pQueueFamilyIndices = qsv.data();
        } else {
            imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCreateInfo.queueFamilyIndexCount = 0; // Optional
            imageCreateInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        WrappedTexture newTexture;
        if (vkCreateImage(setup.logicalDevice.value(), &imageCreateInfo, nullptr, &newTexture.texture) != VK_SUCCESS) {
            throw std::runtime_error("Could not create Texture image.");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(setup.logicalDevice.value(), newTexture.texture, &memoryRequirements);

        VkMemoryAllocateInfo memoryAllocateInfo{};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = find_memory_type(setup.physicalDevice.value(), memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(setup.logicalDevice.value(), &memoryAllocateInfo, nullptr, &newTexture.memory) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't allocate memory for texture");
        }

        vkBindImageMemory(setup.logicalDevice.value(), newTexture.texture, newTexture.memory, 0);

        return newTexture;
    }



    VkImageView create_texture_image_view(const InstanceSetup &setup, const WrappedTexture &texture, const VkFormat &format, uint32_t mipLevels) {
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a texture image view without providing a logical device in the setup.");
        }
        
        VkImageViewCreateInfo texViewCreateInfo{};
        texViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        texViewCreateInfo.image = texture.texture;
        texViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        texViewCreateInfo.format = format;
        texViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        texViewCreateInfo.subresourceRange.baseMipLevel = 0;
        texViewCreateInfo.subresourceRange.levelCount = mipLevels;
        texViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        texViewCreateInfo.subresourceRange.layerCount = 1;

        VkImageView newImageView;
        if (vkCreateImageView(setup.logicalDevice.value(), &texViewCreateInfo, nullptr, &newImageView) != VK_SUCCESS) {
            throw std::runtime_error("Could not create image view for texture.");
        }

        return newImageView;
    }



    DepthBuffer create_depth_buffer(const InstanceSetup &setup) {
        if (!setup.swapChainConfig.has_value()) {
            throw std::runtime_error("Tried to create a depth buffer without providing a swap chain config in the setup.");
        }

        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a depth buffer without providing a logical device in the setup.");
        }

        if (!setup.maxSamplesFlag.has_value()) {
            throw std::runtime_error("Tried to create a depth buffer without providing a max sample flag in the setup.");
        }
        
        DepthBuffer newDepthBuffer;
        
        std::vector<VkFormat> availableFormats = find_supported_formats(setup, { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        
        if (availableFormats.size() == 0) {
            throw std::runtime_error("Could not find any available depth buffer format.");
        }

        newDepthBuffer.format = availableFormats[0];

        newDepthBuffer.hasStencil = newDepthBuffer.format==VK_FORMAT_D32_SFLOAT_S8_UINT || newDepthBuffer.format==VK_FORMAT_D24_UNORM_S8_UINT;

        newDepthBuffer.image = create_texture(setup, setup.swapChainConfig.value().extent.width, setup.swapChainConfig.value().extent.height, setup.maxSamplesFlag.value(), 1, newDepthBuffer.format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

        VkImageViewCreateInfo newImageViewCreateInfo{};
        newImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        newImageViewCreateInfo.image = newDepthBuffer.image.texture;
        newImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        newImageViewCreateInfo.format = newDepthBuffer.format;
        
        newImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        newImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        newImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        newImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        
        newImageViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        newImageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
        newImageViewCreateInfo.subresourceRange.levelCount     = 1;
        newImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        newImageViewCreateInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(setup.logicalDevice.value(), &newImageViewCreateInfo, nullptr, &newDepthBuffer.view) != VK_SUCCESS) {
            throw std::runtime_error("Could not create depth buffer image view.");
        }

        transition_image_layout(setup, &newDepthBuffer.image, newDepthBuffer.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

        return newDepthBuffer;
    }



    std::vector<VkFormat> find_supported_formats(const InstanceSetup &setup, const std::vector<VkFormat> &candidates, const VkImageTiling &tiling, const VkFormatFeatureFlags &features) {
        if (!setup.physicalDevice.has_value()) {
            throw std::runtime_error("Tried to find supported formats without providing a physical device to the setup.");
        }

        std::vector<VkFormat> suitableFormats;

        for (const VkFormat &format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(setup.physicalDevice.value(), format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features)==features) {
                suitableFormats.push_back(format);
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features)==features) {
                suitableFormats.push_back(format);
            }
        }

        return suitableFormats;
    }



    GraphicsPipelineConfig create_graphics_pipeline(const InstanceSetup &setup, const std::string &vertexShaderFilename, const std::string &fragmentShaderFilename) {
        shaderc::SpvCompilationResult vertexCompiled   = compile_shader(vertexShaderFilename,   shaderc_shader_kind::shaderc_vertex_shader);
        shaderc::SpvCompilationResult fragmentCompiled = compile_shader(fragmentShaderFilename, shaderc_shader_kind::shaderc_fragment_shader);

        VkShaderModule vertexModule   = create_shader_module(setup, vertexCompiled);
        VkShaderModule fragmentModule = create_shader_module(setup, fragmentCompiled);
        
        VkPipelineShaderStageCreateInfo vertexStageCreateInfo{};
        vertexStageCreateInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexStageCreateInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
        vertexStageCreateInfo.module = vertexModule;
        vertexStageCreateInfo.pName  = "main";

        VkPipelineShaderStageCreateInfo fragmentStageCreateInfo{};
        fragmentStageCreateInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentStageCreateInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentStageCreateInfo.module = fragmentModule;
        fragmentStageCreateInfo.pName  = "main";

        VkPipelineShaderStageCreateInfo stageCreateInfos[] = { vertexStageCreateInfo, fragmentStageCreateInfo };
        
        std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo dynamicStatesCreateInfo{};
        dynamicStatesCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStatesCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicStatesCreateInfo.pDynamicStates = dynamicStates.data();

        VkVertexInputBindingDescription                  bindingDesc   = Vertex3D::get_binding_description();
        std::array<VkVertexInputAttributeDescription, 3> attributeDesc = Vertex3D::get_attribute_description();

        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
        vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCreateInfo.vertexBindingDescriptionCount   = 1;
        vertexInputStateCreateInfo.vertexAttributeDescriptionCount = attributeDesc.size();
        vertexInputStateCreateInfo.pVertexBindingDescriptions   = &bindingDesc;
        vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDesc.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
        inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

        if (!setup.swapChainConfig.has_value()) {
            throw std::runtime_error("Tried to create a graphics pipeline without providing a swapchain congif in the setup.");
        }

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width  = (float)setup.swapChainConfig.value().extent.width;
        viewport.height = (float)setup.swapChainConfig.value().extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = setup.swapChainConfig.value().extent;

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.pViewports = &viewport;
        viewportStateCreateInfo.scissorCount = 1;
        viewportStateCreateInfo.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
        rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
        rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationStateCreateInfo.lineWidth = 1.0f;
        rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
        multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCreateInfo.sampleShadingEnable = VK_TRUE;
        multisampleStateCreateInfo.rasterizationSamples = setup.maxSamplesFlag.value();
        multisampleStateCreateInfo.minSampleShading = .2f;
        
        VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
        colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachmentState.blendEnable = VK_TRUE;
        colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
        colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
        colorBlendStateCreateInfo.attachmentCount = 1;
        colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

        VkPipelineLayout pipelineLayout;

        if (!setup.uniformLayout.has_value()) {
            throw std::runtime_error("Tried to create a pipeline layout without providing a descriptor set layout in the setup.");
        }
        
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &setup.uniformLayout.value();

        if (!setup.logicalDevice.value()) {
            throw std::runtime_error("Tried to create a pipeline layout without providing a logical device in the setup.");
        }

        if (vkCreatePipelineLayout(setup.logicalDevice.value(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't create graphics pipeline layout.");
        }

        VkRenderPass renderPass = create_render_pass(setup);

        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
        depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCreateInfo.depthTestEnable = true;
        depthStencilStateCreateInfo.depthWriteEnable = true;
        depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencilStateCreateInfo.depthBoundsTestEnable = false;
        depthStencilStateCreateInfo.minDepthBounds = 0.0f;
        depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
        depthStencilStateCreateInfo.stencilTestEnable = false;


        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = &stageCreateInfos[0];
        
        pipelineCreateInfo.pVertexInputState   = &vertexInputStateCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
        pipelineCreateInfo.pViewportState      = &viewportStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
        pipelineCreateInfo.pMultisampleState   = &multisampleStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState  = nullptr;
        pipelineCreateInfo.pColorBlendState    = &colorBlendStateCreateInfo;
        pipelineCreateInfo.pDynamicState       = &dynamicStatesCreateInfo;

        pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;

        pipelineCreateInfo.layout = pipelineLayout;

        pipelineCreateInfo.renderPass = renderPass;
        pipelineCreateInfo.subpass    = 0;

        VkPipeline graphicsPipeline;
        if (vkCreateGraphicsPipelines(setup.logicalDevice.value(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't create graphics pipeline.");
        }

        GraphicsPipelineConfig newPipelineConfig{};
        newPipelineConfig.renderPass = renderPass;
        newPipelineConfig.vertexShaderFilename   = vertexShaderFilename;
        newPipelineConfig.fragmentShaderFilename = fragmentShaderFilename;
        newPipelineConfig.pipelineLayout = pipelineLayout;
        newPipelineConfig.pipeline = graphicsPipeline;

        vkDestroyShaderModule(setup.logicalDevice.value(), vertexModule,   nullptr);
        vkDestroyShaderModule(setup.logicalDevice.value(), fragmentModule, nullptr);

        return newPipelineConfig;
    }



    VkShaderModule create_shader_module(const InstanceSetup &setup, const shaderc::SpvCompilationResult &compiledShader) {
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a shader module without providing a logical device in the setup.");
        }
        
        VkShaderModuleCreateInfo newShaderModuleCreateInfo{};

        newShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        newShaderModuleCreateInfo.pCode = compiledShader.begin();
        newShaderModuleCreateInfo.codeSize = (compiledShader.end() - compiledShader.begin()) * sizeof(uint32_t);

        VkShaderModule newShaderModule;
        if (vkCreateShaderModule(setup.logicalDevice.value(), &newShaderModuleCreateInfo, nullptr, &newShaderModule) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't compile shader");
        }

        return newShaderModule;
    }



    VkRenderPass create_render_pass(const InstanceSetup &setup) {
        if (!setup.swapChainConfig.has_value()) {
            throw std::runtime_error("Tried to create a render pass without providing a swap chain config in the setup.");
        }
        
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a render pass without providing a logical device in the setup.");
        }

        if (!setup.depthBuffer.has_value()) {
            throw std::runtime_error("Tried to create a render pass without providing a depth buffer in the setup.");
        }

        if (!setup.maxSamplesFlag.has_value()) {
            throw std::runtime_error("Tried to create a render pass without providing a max sample in the setup.");
        }
        
        // COLOR
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = setup.swapChainConfig.value().surfaceFormat.format;
        colorAttachment.samples = setup.maxSamplesFlag.value();
        colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentReference{};
        colorAttachmentReference.attachment = 0;
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // DEPTH
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = setup.depthBuffer.value().format;
        depthAttachment.samples = setup.maxSamplesFlag.value();
        depthAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentReference{};
        depthAttachmentReference.attachment = 1;
        depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // RESOLVE
        VkAttachmentDescription resolveAttachment{};
        resolveAttachment.format = setup.swapChainConfig.value().surfaceFormat.format;
        resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference resolveAttachmentReference{};
        resolveAttachmentReference.attachment = 2;
        resolveAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // SUBPASS...
        VkSubpassDescription subpassDescription{};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorAttachmentReference;
        subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
        subpassDescription.pResolveAttachments = &resolveAttachmentReference;

        VkSubpassDependency subpassDependency{};
        subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        subpassDependency.srcAccessMask = 0;
        subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 3> attachmentDescs { colorAttachment, depthAttachment, resolveAttachment };
        VkRenderPass renderPass{};
        VkRenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
        renderPassCreateInfo.pAttachments = attachmentDescs.data();
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpassDescription;
        renderPassCreateInfo.dependencyCount = 1;
        renderPassCreateInfo.pDependencies = &subpassDependency;

        if (vkCreateRenderPass(setup.logicalDevice.value(), &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't create render pass.");
        }

        return renderPass;
    }



    std::vector<VkFramebuffer> create_framebuffers(const InstanceSetup &setup) {
        if (!setup.graphicsPipelineConfig.has_value()) {
            throw std::runtime_error("Tried to create framebuffers without proving a graphics pipeline in the setup.");
        }

        if (!setup.swapChainConfig.has_value()) {
            throw std::runtime_error("Tried to create framebuffers without providing a swap chain config in the setup.");
        }

        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create framebuffers without providing a logical device in the setup.");
        }

        if (!setup.depthBuffer.has_value()) {
            throw std::runtime_error("Tried to create framebufffers without providing a depth buffer in the setup.");
        }

        std::vector<VkFramebuffer> newFramebuffers(setup.swapChainImageViews.size());

        for (size_t i = 0; i != setup.swapChainImageViews.size(); ++i) {
            VkImageView attachments[] = { setup.colorImage.value().imageView, setup.depthBuffer.value().view, setup.swapChainImageViews[i] };

            VkFramebufferCreateInfo framebufferCreateInfo{};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.renderPass = setup.graphicsPipelineConfig.value().renderPass;
            framebufferCreateInfo.attachmentCount = 3;
            framebufferCreateInfo.pAttachments = &attachments[0];
            framebufferCreateInfo.width = setup.swapChainConfig.value().extent.width;
            framebufferCreateInfo.height = setup.swapChainConfig.value().extent.height;
            framebufferCreateInfo.layers = 1;

            if (vkCreateFramebuffer(setup.logicalDevice.value(), &framebufferCreateInfo, nullptr, &newFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Couldn't create framebuffers.");
            }
        }
        
        return newFramebuffers;
    }



    WrappedTexture create_texture_from_image(const InstanceSetup &setup, const std::string &textureFilename) {
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a texture from an image without providing a logical device in the setup.");
        }
        
        int imageWidth;
        int imageHeight;
        int imageChannels;

        stbi_uc *imageData = stbi_load(textureFilename.c_str(), &imageWidth, &imageHeight, &imageChannels, STBI_rgb_alpha);
        

        if (!imageData) {
            throw std::runtime_error("Could not load image data for texture creation.");
        }

        VkDeviceSize imageSizeInBytes = imageWidth*imageHeight*imageChannels*sizeof(uint8_t);

        WrappedBuffer stagingTextureBuffer = create_buffer(setup, imageSizeInBytes, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void *stagingTextureBufferMapping;
        vkMapMemory(setup.logicalDevice.value(), stagingTextureBuffer.memory, 0, stagingTextureBuffer.sizeInBytes, 0, &stagingTextureBufferMapping);
        memcpy_s(stagingTextureBufferMapping, stagingTextureBuffer.sizeInBytes,imageData, imageSizeInBytes);
        vkUnmapMemory(setup.logicalDevice.value(), stagingTextureBuffer.memory);
        
        uint32_t availableMips = static_cast<uint32_t>(std::floor(std::log2(std::max(imageHeight, imageWidth))));

        WrappedTexture newTexture = create_texture(setup, imageWidth, imageHeight, VK_SAMPLE_COUNT_1_BIT, availableMips, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        newTexture.mipLevels.emplace(availableMips);

        transition_image_layout(setup, &newTexture, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, availableMips);

        copy_buffer_to_image(setup, stagingTextureBuffer, &newTexture.texture, imageWidth, imageHeight);

        generate_mipmaps(setup, newTexture.texture, VK_FORMAT_R8G8B8A8_SRGB, imageWidth, imageHeight, availableMips);
        // TODO : See about removing ? should be done in mipmaps instead
        //transition_image_layout(setup, &newTexture, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, availableMips);

        vkDestroyBuffer(setup.logicalDevice.value(), stagingTextureBuffer.buffer, nullptr);
        vkFreeMemory(setup.logicalDevice.value(), stagingTextureBuffer.memory, nullptr);

        stbi_image_free(imageData);

        return newTexture;
    }



    void copy_buffer(const InstanceSetup &setup, const WrappedBuffer &source, WrappedBuffer *dest) {
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to copy a buffer without providing a logical device in the setup.");
        }

        if (!setup.commandPools.has_value()) {
            throw std::runtime_error("Tried to copy a buffer without providing command pools in the setup.");
        }

        if (!setup.transferQueue.has_value()) {
            throw std::runtime_error("Tried to copy a buffer without providing a transfer queue in the setup.");
        }

        VkCommandBufferAllocateInfo bufferAllocateInfo{};
        bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        bufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        bufferAllocateInfo.commandPool = setup.commandPools.value().transfer;
        bufferAllocateInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(setup.logicalDevice.value(), &bufferAllocateInfo, &commandBuffer);

        VkCommandBufferBeginInfo bufferBeginInfo{};
        bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &bufferBeginInfo);

        VkBufferCopy bufferCopy{};
        bufferCopy.srcOffset = 0;
        bufferCopy.dstOffset = 0;
        bufferCopy.size = std::min(source.sizeInBytes, dest->sizeInBytes);
        vkCmdCopyBuffer(commandBuffer, source.buffer, dest->buffer, 1, &bufferCopy);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(setup.transferQueue.value(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(setup.transferQueue.value());

        vkFreeCommandBuffers(setup.logicalDevice.value(), setup.commandPools.value().transfer, 1, &commandBuffer);
    }



    WrappedBuffer create_buffer(const InstanceSetup &setup, VkDeviceSize sizeInBytes, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
        WrappedBuffer newBuffer;
        
        VkBufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = sizeInBytes;
        bufferCreateInfo.usage = usage;
        
        const std::set<uint32_t> qs { setup.queues.value().graphicsIndex.value(), setup.queues.value().presentIndex.value(), setup.queues.value().transferIndex.value() };
        const std::vector<uint32_t> qsv(qs.begin(), qs.end());
        
        if (qs.size() != 1) {
            bufferCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferCreateInfo.queueFamilyIndexCount = qsv.size();
            bufferCreateInfo.pQueueFamilyIndices = qsv.data();
        } else{
            bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        if (vkCreateBuffer(setup.logicalDevice.value(), &bufferCreateInfo, nullptr, &newBuffer.buffer) != VK_SUCCESS) {
            throw std::runtime_error("Could not create vertex buffer.");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(setup.logicalDevice.value(), newBuffer.buffer, &memoryRequirements);

        VkMemoryAllocateInfo bufferAllocateInfo{};
        bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        bufferAllocateInfo.allocationSize = memoryRequirements.size;
        bufferAllocateInfo.memoryTypeIndex = find_memory_type(setup.physicalDevice.value(), memoryRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(setup.logicalDevice.value(), &bufferAllocateInfo, nullptr, &newBuffer.memory) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't allocate buffer memory.");
        }

        vkBindBufferMemory(setup.logicalDevice.value(), newBuffer.buffer, newBuffer.memory, 0);

        newBuffer.sizeInBytes = sizeInBytes;

        return newBuffer;
    }



    void transition_image_layout(const InstanceSetup &setup, WrappedTexture *texture, const VkFormat &format, const VkImageLayout &oldLayout, const VkImageLayout &newLayout, uint32_t mipLevels) {
        if (!setup.commandPools.has_value()) {
            throw std::runtime_error("Tried to transition an image layout without providing command pools in the setup.");
        }

        if (!setup.graphicsQueue.has_value()) {
            throw std::runtime_error("Tried to transition an image layout without providing a graphics queue in the setup.");
        }
        
        VkCommandBuffer transitionCommand = begin_one_shot_command(setup, setup.commandPools.value().graphics);

        VkImageMemoryBarrier transitionBarrier{};
        transitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        transitionBarrier.oldLayout = oldLayout;
        transitionBarrier.newLayout = newLayout;
        transitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transitionBarrier.image = texture->texture;

        transitionBarrier.subresourceRange.baseMipLevel = 0;
        transitionBarrier.subresourceRange.levelCount = mipLevels;
        transitionBarrier.subresourceRange.baseArrayLayer = 0;
        transitionBarrier.subresourceRange.layerCount = 1;

        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
            transitionBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        } else {
            transitionBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            transitionBarrier.srcAccessMask = 0;
            transitionBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destStage   = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            transitionBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            transitionBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destStage   = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)  {
            transitionBarrier.srcAccessMask = 0;
            transitionBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destStage   = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        } else {
            throw std::runtime_error("Could not transition image layout between specified layouts.");
        }

    
        vkCmdPipelineBarrier(
            transitionCommand,
            sourceStage, destStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &transitionBarrier
        );

        end_one_shot_command(setup, setup.commandPools.value().graphics, setup.graphicsQueue.value(), &transitionCommand);
    }



    VkCommandBuffer begin_one_shot_command(const InstanceSetup &setup, const VkCommandPool &selectedPool) {
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to begin a one-shot command without providing a logical device in the setup.");
        }

        VkCommandBufferAllocateInfo osCommandBufferAllocateInfo{};
        osCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        osCommandBufferAllocateInfo.commandBufferCount = 1;
        osCommandBufferAllocateInfo.commandPool = selectedPool;
        osCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkCommandBuffer osCommandBuffer;
        vkAllocateCommandBuffers(setup.logicalDevice.value(), &osCommandBufferAllocateInfo, &osCommandBuffer);

        VkCommandBufferBeginInfo osCommandBufferBeginInfo{};
        osCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        osCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(osCommandBuffer, &osCommandBufferBeginInfo);

        return osCommandBuffer;
    }



    void end_one_shot_command(const InstanceSetup &setup, const VkCommandPool &selectedPool, const VkQueue &selectedQueue, VkCommandBuffer *osCommandBuffer) {
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to end a one-shot command without providing a logical device in the setup.");
        }

        vkEndCommandBuffer(*osCommandBuffer);

        VkSubmitInfo osSubmitInfo{};
        osSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        osSubmitInfo.commandBufferCount = 1;
        osSubmitInfo.pCommandBuffers = osCommandBuffer;
        
        vkQueueSubmit(selectedQueue, 1, &osSubmitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(selectedQueue);

        vkFreeCommandBuffers(setup.logicalDevice.value(), selectedPool, 1, osCommandBuffer);
    }



    VkSampler create_texture_sampler(const InstanceSetup &setup, std::optional<uint32_t> mipLevel) {
        if (!setup.physicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a texture sampler without providing a physical device in the setup.");
        }

        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a texture sampler without providing a logical device in the setup.");
        }

        VkSamplerCreateInfo samplerCreateInfo{};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;

        // TODO: Maybe change thos out once "novelty fun" fades lmao
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

        samplerCreateInfo.anisotropyEnable = VK_TRUE;

        VkPhysicalDeviceProperties physicalProperties;
        vkGetPhysicalDeviceProperties(setup.physicalDevice.value(), &physicalProperties);

        samplerCreateInfo.maxAnisotropy = physicalProperties.limits.maxSamplerAnisotropy;

        samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
        samplerCreateInfo.compareEnable = VK_FALSE;
        samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.mipLodBias = 0.0f;
        samplerCreateInfo.minLod = 0.0f;
        samplerCreateInfo.maxLod = 0.0f;

        if (mipLevel.has_value()) {
            samplerCreateInfo.maxLod = static_cast<float>(mipLevel.value());
        }

        VkSampler newSampler;
        if (vkCreateSampler(setup.logicalDevice.value(), &samplerCreateInfo, nullptr, &newSampler) != VK_SUCCESS) {
            throw std::runtime_error("Could not create texture sampler.");
        }

        return newSampler;
    }



    void copy_buffer_to_image(const InstanceSetup &setup, const WrappedBuffer &dataSource, VkImage *image, uint32_t imageWidth, uint32_t imageHeight) {
        if (!setup.commandPools.has_value()) {
            throw std::runtime_error("Tried to copy a buffer to an image without providing command pools in the setup.");
        }

        if (!setup.graphicsQueue.has_value()) {
            throw std::runtime_error("Tried to copy a buffer to an image without providing a graphics queue in the setup.");
        }

        VkCommandBuffer copyToImageCommand = begin_one_shot_command(setup, setup.commandPools.value().graphics);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.layerCount = 1;
        region.imageSubresource.baseArrayLayer = 0;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { imageWidth, imageHeight, 1 };

        vkCmdCopyBufferToImage(copyToImageCommand, dataSource.buffer, *image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        end_one_shot_command(setup, setup.commandPools.value().graphics, setup.graphicsQueue.value(), &copyToImageCommand);
    }



    void generate_mipmaps(const InstanceSetup &setup, const VkImage &image, const VkFormat &format, int width, int height, uint32_t mipLevels) {
        if (!setup.commandPools.has_value()) {
            throw std::runtime_error("Tried to generate mipmaps without providing command pools in the setup.");
        }

        if (!setup.graphicsQueue.has_value()) {
            throw std::runtime_error("Tried to generate mipmaps without providing a graphics queue in the setup.");
        }

        if (!setup.physicalDevice.has_value()) {
            throw std::runtime_error("Tried to generate mipmaps without providing a physical device in the setup.");
        }

        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(setup.physicalDevice.value(), format, &props);

        if (! (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) ) {
            throw std::runtime_error("Mipmaps can't be blitted because physical device can't handle their format with linear filtering.");
        }
        
        VkCommandBuffer command = begin_one_shot_command(setup, setup.commandPools.value().graphics);
        
        VkImageMemoryBarrier mipmapBarrier{};
        mipmapBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        mipmapBarrier.image = image;
        mipmapBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        mipmapBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        mipmapBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        mipmapBarrier.subresourceRange.baseArrayLayer = 0;
        mipmapBarrier.subresourceRange.layerCount = 1;
        mipmapBarrier.subresourceRange.levelCount = 1;

        int mipWidth = width;
        int mipHeight = height;
        
        for (uint32_t i = 1; i != mipLevels; ++i) {
            mipmapBarrier.subresourceRange.baseMipLevel = i-1;
            mipmapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            mipmapBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            mipmapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            mipmapBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            
            vkCmdPipelineBarrier(command,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &mipmapBarrier
            );
            
            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i-1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { (mipWidth>1)? mipWidth/2 : 1, (mipHeight>1)? mipHeight/2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;
            
            vkCmdBlitImage(command,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR
            );

            mipmapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            mipmapBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            mipmapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            mipmapBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(command,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &mipmapBarrier
            );
            
            if (mipWidth  > 1) { mipWidth  /= 2; }
            if (mipHeight > 1) { mipHeight /= 2; }
        }

        mipmapBarrier.subresourceRange.baseMipLevel = mipLevels - 1;
        mipmapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        mipmapBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        mipmapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        mipmapBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;


        vkCmdPipelineBarrier(command,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &mipmapBarrier
        );
        
        end_one_shot_command(setup, setup.commandPools.value().graphics, setup.graphicsQueue.value(), &command);
    }



    WrappedBuffer create_vertex_buffer(const InstanceSetup &setup, const std::vector<Vertex3D> &vertices) {
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a vertex buffer without providing a logical device in the setup");
        }

        WrappedBuffer stagingBuffer = create_buffer(setup, vertices.size()*sizeof(Vertex3D), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        void *data;
        vkMapMemory(setup.logicalDevice.value(), stagingBuffer.memory, 0, stagingBuffer.sizeInBytes, 0, &data);
            memcpy_s(data, stagingBuffer.sizeInBytes, vertices.data(), stagingBuffer.sizeInBytes);
        vkUnmapMemory(setup.logicalDevice.value(), stagingBuffer.memory);

        WrappedBuffer newBuffer = create_buffer(setup, stagingBuffer.sizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        copy_buffer(setup, stagingBuffer, &newBuffer);

        vkDestroyBuffer(setup.logicalDevice.value(), stagingBuffer.buffer, nullptr);
        vkFreeMemory(setup.logicalDevice.value(), stagingBuffer.memory, nullptr);
        
        return newBuffer;
    }



    WrappedBuffer create_index_buffer(const InstanceSetup &setup, const std::vector<uint32_t> &indices) {
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create an index buffer without providing a logical device in the setup.");
        }

        WrappedBuffer stagingBuffer = create_buffer(setup, indices.size()*sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        void *data;
        vkMapMemory(setup.logicalDevice.value(), stagingBuffer.memory, 0, stagingBuffer.sizeInBytes, 0, &data);
            memcpy_s(data, stagingBuffer.sizeInBytes, indices.data(), stagingBuffer.sizeInBytes);
        vkUnmapMemory(setup.logicalDevice.value(), stagingBuffer.memory);

        WrappedBuffer newBuffer = create_buffer(setup, stagingBuffer.sizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        copy_buffer(setup, stagingBuffer, &newBuffer);

        vkDestroyBuffer(setup.logicalDevice.value(), stagingBuffer.buffer, nullptr);
        vkFreeMemory(setup.logicalDevice.value(), stagingBuffer.memory, nullptr);
        
        return newBuffer;
    }



    std::vector<WrappedBuffer> create_uniform_buffers(const InstanceSetup &setup) {
        std::vector<WrappedBuffer> newUniformBuffers(MAX_FRAMES_IN_FLIGHT);

        VkDeviceSize bufferSizeInBytes = sizeof(UniformBufferObject);

        for (size_t i = 0; i != newUniformBuffers.size(); ++i) {
            newUniformBuffers[i] = create_buffer(setup, bufferSizeInBytes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            void *bufferMappingLocation;
            vkMapMemory(setup.logicalDevice.value(), newUniformBuffers[i].memory, 0, newUniformBuffers[i].sizeInBytes, 0, &bufferMappingLocation);

            newUniformBuffers[i].mapping.emplace(bufferMappingLocation);
        }

        return newUniformBuffers;
    }



    VkDescriptorPool create_descriptor_pool(const InstanceSetup &setup) {
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a descriptor pool without providing a logical device in the setup.");
        }
        
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

        VkDescriptorPoolCreateInfo poolCreateInfo{};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolCreateInfo.pPoolSizes = poolSizes.data();
        poolCreateInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPool newDescriptorPool;
        if (vkCreateDescriptorPool(setup.logicalDevice.value(), &poolCreateInfo, nullptr, &newDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Could not create descriptor pool.");
        }

        return newDescriptorPool;
    }



    std::vector<VkDescriptorSet> create_descriptor_sets(const InstanceSetup &setup) {
        std::vector<VkDescriptorSetLayout> newLayouts(MAX_FRAMES_IN_FLIGHT, setup.uniformLayout.value());
        
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool = setup.descriptorPool.value();
        descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        descriptorSetAllocateInfo.pSetLayouts = newLayouts.data();
        
        std::vector<VkDescriptorSet> newDescriptorSets(MAX_FRAMES_IN_FLIGHT);

        if (vkAllocateDescriptorSets(setup.logicalDevice.value(), &descriptorSetAllocateInfo, newDescriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't allocate descriptor sets.");
        }

        for (size_t i = 0; i != newDescriptorSets.size(); ++i) {
            VkDescriptorBufferInfo descriptorBufferInfo{};
            descriptorBufferInfo.buffer = setup.uniformBuffers[i].buffer;
            descriptorBufferInfo.offset = 0;
            descriptorBufferInfo.range  = sizeof(UniformBufferObject);

            VkDescriptorImageInfo descriptorImageInfo{};
            descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorImageInfo.imageView = setup.textureView.value();
            descriptorImageInfo.sampler = setup.textureSampler.value();

            std::array<VkWriteDescriptorSet, 2> writeInfos{};

            writeInfos[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeInfos[0].dstSet = newDescriptorSets[i];
            writeInfos[0].dstBinding = 0;
            writeInfos[0].dstArrayElement = 0;
            writeInfos[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeInfos[0].descriptorCount = 1;
            writeInfos[0].pBufferInfo = &descriptorBufferInfo;

            writeInfos[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeInfos[1].dstSet = newDescriptorSets[i];
            writeInfos[1].dstBinding = 1;
            writeInfos[1].dstArrayElement = 0;
            writeInfos[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeInfos[1].descriptorCount = 1;
            writeInfos[1].pImageInfo = &descriptorImageInfo;
            
            vkUpdateDescriptorSets(setup.logicalDevice.value(), static_cast<uint32_t>(writeInfos.size()), writeInfos.data(), 0, nullptr);
        }

        return newDescriptorSets;
    }


    
    std::vector<VkCommandBuffer> create_command_buffers(const InstanceSetup &setup) {
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a command buffer without providing a logical device in the setup.");
        }

        if (!setup.commandPools.has_value()) {
            throw std::runtime_error("Tried to create a command buffer without providing command pools in the buffer.");
        }

        std::vector<VkCommandBuffer> newCommandBuffers(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo commandBufferAllocationInfo{};
        commandBufferAllocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocationInfo.commandPool = setup.commandPools.value().graphics;
        commandBufferAllocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocationInfo.commandBufferCount = static_cast<uint32_t>(newCommandBuffers.size());

        if (vkAllocateCommandBuffers(setup.logicalDevice.value(), &commandBufferAllocationInfo, newCommandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't allocate command buffers");
        }

        return newCommandBuffers;
    }



    BaseSyncObjects create_base_sync_objects(const InstanceSetup &setup) {
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create base sync objects without providing a logical device in the setup");
        }

        BaseSyncObjects newSyncObjects{};

        newSyncObjects.imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        newSyncObjects.renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        newSyncObjects.inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        
        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i != MAX_FRAMES_IN_FLIGHT; ++i) {
            if (vkCreateSemaphore(setup.logicalDevice.value(), &semaphoreCreateInfo, nullptr, &newSyncObjects.imageAvailableSemaphores[i]) != VK_SUCCESS) {
                throw std::runtime_error("Couldn't create `image available` semaphore.");
            }

            if (vkCreateSemaphore(setup.logicalDevice.value(), &semaphoreCreateInfo, nullptr, &newSyncObjects.renderFinishedSemaphores[i]) != VK_SUCCESS) {
                throw std::runtime_error("Couldn't create `render fisnished` semaphore.");
            }

            if (vkCreateFence(setup.logicalDevice.value(), &fenceCreateInfo, nullptr, &newSyncObjects.inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("Couldn't create `in flight images` fence.");
            }
        }

        return newSyncObjects;
    }

    /*----------------------------*
     *- FUNCTIONS: Setup cleanup -*
     *----------------------------*/

    void clean_setup(const InstanceSetup &setup) {

        for (const VkSemaphore &semaphore : setup.syncObjects.value().imageAvailableSemaphores) {
            vkDestroySemaphore(setup.logicalDevice.value(), semaphore, nullptr);
        }

        for (const VkSemaphore &semaphore : setup.syncObjects.value().renderFinishedSemaphores) {
            vkDestroySemaphore(setup.logicalDevice.value(), semaphore, nullptr);
        }
        
        for (const VkFence &fence : setup.syncObjects.value().inFlightFences) {
            vkDestroyFence(setup.logicalDevice.value(), fence, nullptr);
        }

        vkDestroyCommandPool(setup.logicalDevice.value(), setup.commandPools.value().graphics, nullptr);
        vkDestroyCommandPool(setup.logicalDevice.value(), setup.commandPools.value().transfer, nullptr);

        cleanup_swap_chain(setup);

        vkDestroySampler(setup.logicalDevice.value(), setup.textureSampler.value(), nullptr);
        vkDestroyImageView(setup.logicalDevice.value(), setup.textureView.value(), nullptr);

        vkDestroyImage(setup.logicalDevice.value(), setup.texture.value().texture, nullptr);
        vkFreeMemory(setup.logicalDevice.value(), setup.texture.value().memory, nullptr);
        
        for (const WrappedBuffer &uniformBuffer : setup.uniformBuffers) {
            vkDestroyBuffer(setup.logicalDevice.value(), uniformBuffer.buffer, nullptr);
            vkFreeMemory(setup.logicalDevice.value(), uniformBuffer.memory, nullptr);
        }

        vkDestroyDescriptorPool(setup.logicalDevice.value(), setup.descriptorPool.value(), nullptr);

        vkDestroyDescriptorSetLayout(setup.logicalDevice.value(), setup.uniformLayout.value(), nullptr);

        vkDestroyBuffer(setup.logicalDevice.value(), setup.indexBuffer.value().buffer, nullptr);
        vkFreeMemory(setup.logicalDevice.value(), setup.indexBuffer.value().memory, nullptr);

        vkDestroyBuffer(setup.logicalDevice.value(), setup.vertexBuffer.value().buffer, nullptr);
        vkFreeMemory(setup.logicalDevice.value(), setup.vertexBuffer.value().memory, nullptr);

        vkDestroyPipeline(setup.logicalDevice.value(), setup.graphicsPipelineConfig.value().pipeline, nullptr);
        vkDestroyPipelineLayout(setup.logicalDevice.value(), setup.graphicsPipelineConfig.value().pipelineLayout, nullptr);
        
        vkDestroyRenderPass(setup.logicalDevice.value(), setup.graphicsPipelineConfig.value().renderPass, nullptr);
        
        vkDestroyDevice(setup.logicalDevice.value(), nullptr);

        vkDestroySurfaceKHR(setup.instance, setup.surface.value(), nullptr);

        if (setup.debugMessenger.has_value()) {
            vkDestroyDebugUtilsMessengerEXT(setup.instance, setup.debugMessenger.value(), nullptr);
        }

        vkDestroyInstance(setup.instance, nullptr);
    }



    void cleanup_swap_chain(const InstanceSetup &setup) {
        for (const VkFramebuffer &framebuffer : setup.swapChainFramebuffers) {
            vkDestroyFramebuffer(setup.logicalDevice.value(), framebuffer, nullptr);
        }

        for (const VkImageView &imageView : setup.swapChainImageViews) {
            vkDestroyImageView(setup.logicalDevice.value(), imageView, nullptr);
        }

        vkDestroyImageView(setup.logicalDevice.value(), setup.depthBuffer.value().view, nullptr);
        vkDestroyImage(setup.logicalDevice.value(), setup.depthBuffer.value().image.texture, nullptr);
        vkFreeMemory(setup.logicalDevice.value(), setup.depthBuffer.value().image.memory, nullptr);

        vkDestroyImageView(setup.logicalDevice.value(), setup.colorImage.value().imageView, nullptr);
        vkDestroyImage(setup.logicalDevice.value(), setup.colorImage.value().image.texture, nullptr);
        vkFreeMemory(setup.logicalDevice.value(), setup.colorImage.value().image.memory, nullptr);

        vkDestroySwapchainKHR(setup.logicalDevice.value(), setup.swapChain.value(), nullptr);
    }

    /*----------------------------*
     *- FUNCTIONS: Setup drawing -*
     *----------------------------*/

    void draw_frame(InstanceSetup *setup, GLFWwindow *window, size_t *currentFrame) {
        if (!setup->logicalDevice.has_value()) {
            throw std::runtime_error("Tried to draw a frame without providing a logical device in the setup.");
        }

        if (!setup->syncObjects.has_value()) {
            throw std::runtime_error("Tried to draw a frame without providing sync objects in the setup.");
        }

        if (!setup->swapChain.has_value()) {
            throw std::runtime_error("Tried to draw a frame without providing a swap chain in the setup.");
        }
        
        if (!setup->graphicsQueue.has_value()) {
            throw std::runtime_error("Tried to draw a frame without providing a graphics queue in the setup.");
        }

        vkWaitForFences(setup->logicalDevice.value(), 1, &setup->syncObjects.value().inFlightFences[*currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
        
        
        uint32_t imageIndex;
        VkResult swapChainStatus = vkAcquireNextImageKHR(setup->logicalDevice.value(), setup->swapChain.value(), std::numeric_limits<uint64_t>::max(), setup->syncObjects.value().imageAvailableSemaphores[*currentFrame], VK_NULL_HANDLE, &imageIndex);
        
        if (swapChainStatus == VK_ERROR_OUT_OF_DATE_KHR || swapChainStatus == VK_SUBOPTIMAL_KHR) {
            recreate_swap_chain(setup, window);
            return;
        } else if (swapChainStatus != VK_SUCCESS) {
            throw std::runtime_error("Failed to acquire next swapchain image.");
        }

        update_uniform_buffer(*setup, *currentFrame);
        
        vkResetFences(setup->logicalDevice.value(), 1, &setup->syncObjects.value().inFlightFences[*currentFrame]);

        vkResetCommandBuffer(setup->commandBuffers[*currentFrame], NULL);

        record_command_buffer(*setup, setup->commandBuffers[*currentFrame], imageIndex, *currentFrame);
        
        
        VkSemaphore          waitSemaphores[] = { setup->syncObjects.value().imageAvailableSemaphores[*currentFrame] };
        VkPipelineStageFlags waitStages[]     = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        
        VkSemaphore signalSemaphore[] = { setup->syncObjects.value().renderFinishedSemaphores[*currentFrame] };
        
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphores[0];
        submitInfo.pWaitDstStageMask = &waitStages[0];
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &setup->commandBuffers[*currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore[0];

        if (vkQueueSubmit(setup->graphicsQueue.value(), 1, &submitInfo, setup->syncObjects.value().inFlightFences[*currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't submit sync objects while drawing frame.");
        }
        
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &signalSemaphore[0];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &setup->swapChain.value();
        presentInfo.pImageIndices = &imageIndex;
        
        VkResult queueStatus = vkQueuePresentKHR(setup->graphicsQueue.value(), &presentInfo);

        if (queueStatus == VK_ERROR_OUT_OF_DATE_KHR ||queueStatus == VK_SUBOPTIMAL_KHR) {
            recreate_swap_chain(setup, window);
        } else if (queueStatus != VK_SUCCESS) {
            throw std::runtime_error("Failed to acquire next swapchain image.");
        }

        *currentFrame = (*currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }



    void recreate_swap_chain(InstanceSetup *setup, GLFWwindow *window) {
        vkDeviceWaitIdle(setup->logicalDevice.value());

        std::cerr << "Recreating swap chain" << std::endl;

        cleanup_swap_chain(*setup);

        setup->swapChainSupport.emplace(check_swap_chain_support(*setup, setup->physicalDevice.value()));
        setup->swapChainConfig.emplace(prepare_swap_chain_config(*setup, window));
        setup->swapChain.emplace(create_swap_chain(*setup, window));
        setup->swapChainImages = retrieve_swap_chain_images(*setup);
        setup->swapChainImageViews = create_swap_chain_image_views(*setup);
        setup->depthBuffer = create_depth_buffer(*setup);
        setup->colorImage = create_color_image(*setup);
        setup->swapChainFramebuffers = create_framebuffers(*setup);
    }



    void update_uniform_buffer(const InstanceSetup &setup, size_t frame) {
        if (!setup.swapChainConfig.has_value()) {
            throw std::runtime_error("Tried to update a uniform buffer without providing a swapchain config in the setup.");
        }

        if (setup.uniformBuffers.size() <= frame) {
            throw std::runtime_error("Tried to update a uniform buffer too far in the array provided in the setup");
        }

        if (!setup.uniformBuffers[frame].mapping.has_value()) {
            throw std::runtime_error("Tried to update a uniform buffer without providing it's memory mapping in the setup.");
        }

        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();

        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view  = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.projection = glm::perspective(glm::radians(45.0f), setup.swapChainConfig.value().extent.width / (float)setup.swapChainConfig.value().extent.height, 0.01f, 99999.9f);

        ubo.projection[1][1] *= -1;

        memcpy_s(setup.uniformBuffers[frame].mapping.value(), setup.uniformBuffers[frame].sizeInBytes, &ubo, sizeof(ubo));
    }



    void record_command_buffer(const InstanceSetup &setup, const VkCommandBuffer &commandBuffer, uint32_t imageIndex, size_t currentFrame) {
        if (!setup.graphicsPipelineConfig.has_value()) {
            throw std::runtime_error("Tried to record a command buffer without providing a graphics pipeline to the setup.");
        }

        if (!setup.swapChainConfig.has_value()) {
            throw std::runtime_error("Tried to record a command buffer without providing a swap chain config in the setup.");
        }

        if (!setup.vertexBuffer.has_value()) {
            throw std::runtime_error("Tried to record a command buffer without providing a vertex buffer in the setup.");
        }

        if (!setup.indexBuffer.has_value()) {
            throw std::runtime_error("Tried to record a command buffer without providing an index buffer in the setup.");
        }

        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't record command buffer (beginning).");
        }

        std::array<VkClearValue, 2> clearColors;
        clearColors[0].color = {{0.8f, 0.0f, 0.8f, 1.0f}};
        clearColors[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = setup.graphicsPipelineConfig.value().renderPass;
        renderPassBeginInfo.framebuffer = setup.swapChainFramebuffers[imageIndex];
        renderPassBeginInfo.renderArea.extent = setup.swapChainConfig.value().extent;
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearColors.size());
        renderPassBeginInfo.pClearValues = clearColors.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, setup.graphicsPipelineConfig.value().pipeline);

        VkBuffer     vertexBuffers[] = { setup.vertexBuffer.value().buffer };
        VkDeviceSize offsets[]       = { 0 };

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffers[0], &offsets[0]);

        vkCmdBindIndexBuffer(commandBuffer, setup.indexBuffer.value().buffer, 0, VK_INDEX_TYPE_UINT32);

        // TODO: avoid rpeating it again by storing them somewhere ?
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(setup.swapChainConfig.value().extent.width);
            viewport.height = static_cast<float>(setup.swapChainConfig.value().extent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = setup.swapChainConfig.value().extent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        // END TODO

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, setup.graphicsPipelineConfig.value().pipelineLayout, 0, 1, &setup.descriptorSets[currentFrame], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, setup.indexCount.value(), 1, 0, 0, 0);
    
        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer (end)");
        }
    }

    /*---------------------*
     *- FUNCTIONS: helper -*
     *---------------------*/

    LoadedModel load_model(const std::string &filename) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;

        std::string warning;
        std::string error;

        if (tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, filename.c_str())) {
            std::cout << "[TINYOBJ]: " + warning + error;
        }

        LoadedModel newModel{};

        std::unordered_map<Vertex3D, uint32_t> uniqueVertices{};

        for (tinyobj::shape_t shape : shapes) {
            for (tinyobj::index_t index : shape.mesh.indices) {
                Vertex3D newVertex {
                    .position={
                        attrib.vertices[3*index.vertex_index + 0],
                        attrib.vertices[3*index.vertex_index + 1],
                        attrib.vertices[3*index.vertex_index + 2]
                    },
                    .color={
                        1.0f,
                        1.0f,
                        1.0f
                    },
                    .uv={
                        attrib.texcoords[2*index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2*index.texcoord_index + 1]
                    }
                };

                if (uniqueVertices.count(newVertex) == 0) {
                    uniqueVertices[newVertex] = static_cast<uint32_t>(newModel.vertices.size());
                    newModel.vertices.push_back(newVertex);
                }

                newModel.indices.push_back(uniqueVertices[newVertex]);
            }
        }

        return newModel;
    }



    uint32_t find_memory_type(const VkPhysicalDevice &device, uint32_t typeFilter, const VkMemoryPropertyFlags &properties) {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);
        
        for (uint32_t i = 0; i != memoryProperties.memoryTypeCount; ++i) {
            if ( (typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find a suitable memory type.");
    }



    shaderc::SpvCompilationResult compile_shader(const std::string &filename, const shaderc_shader_kind &shaderKind) {
        shaderc::Compiler compiler;

        shaderc::CompileOptions options;

        options.SetTargetEnvironment(shaderc_target_env::shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
        options.SetSourceLanguage(shaderc_source_language_glsl);

        std::ifstream shaderFile(filename, std::ifstream::binary);

        if (!shaderFile.is_open()) {
            throw std::runtime_error("Could not open shader file : '" + filename + "'.");
        }

        std::string fileContent;

        shaderFile.seekg(0, std::ios::end);
        fileContent.reserve(shaderFile.tellg());
        shaderFile.seekg(0, std::ios::beg);

        fileContent.assign((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());

        shaderc::SpvCompilationResult compiled = compiler.CompileGlslToSpv(fileContent, shaderKind, filename.c_str(), options);

        if (compiled.GetCompilationStatus() != shaderc_compilation_status::shaderc_compilation_status_success) {
            throw std::runtime_error("Couldn't compile shader `" + filename + "` : [" + compiled.GetErrorMessage() + "]");
        }

        std::vector<uint32_t> code(compiled.begin(), compiled.end());
        return compiled;
    }
}
