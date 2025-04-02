#include "setup.hpp"

#include <limits>
#include <algorithm>

namespace fhope {
    bool QueueSetup::is_complete() const {
        return (graphicsIndex.has_value() && presentIndex.has_value());
    }



    static VKAPI_ATTR VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void *pUserData) {
        std::cerr << "Validation Layer : " << pCallbackData->pMessage << std::endl;
        *((std::ofstream *)pUserData) << "[" << messageSeverity << "] - " << pCallbackData->pMessage << std::endl << std::flush;

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

    

    InstanceSetup generate_vulkan_setup(GLFWwindow *window, std::string appName, std::tuple<uint32_t, uint32_t, uint32_t> appVersion, const std::string &vertexShaderFilename, const std::string &fragmentShaderFilename) {
        glfwMakeContextCurrent(window);
        
        InstanceSetup newSetup = create_instance(appName, appVersion);
        
        newSetup.surface.emplace(get_surface_from_window(newSetup, window));

        newSetup.physicalDevice = autopick_physical_device(newSetup);

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

        // Getting a swap chain
        newSetup.swapChainSupport.emplace(check_swap_chain_support(newSetup, newSetup.physicalDevice.value()));
        
        newSetup.swapChainConfig.emplace(prepare_swap_chain_config(newSetup, window));
        
        newSetup.swapChain.emplace(create_swap_chain(newSetup, window));

        newSetup.swapChainImages = retrieve_swap_chain_images(newSetup);

        newSetup.swapChainImageViews = create_swap_chain_image_views(newSetup);

        newSetup.graphicsPipelineConfig.emplace(create_graphics_pipeline(newSetup, vertexShaderFilename, fragmentShaderFilename));

        newSetup.swapChainFramebuffers = create_framebuffers(newSetup);

        newSetup.commandPool.emplace(create_command_pool(newSetup));

        newSetup.vertexBuffer.emplace(create_vertex_buffer(newSetup, std::vector<Vertex2D>(EXAMPLE_VERTICES.begin(), EXAMPLE_VERTICES.end())));

        newSetup.commandBuffers = create_command_buffers(newSetup);

        newSetup.syncObjects.emplace(create_base_sync_objects(newSetup));

        return newSetup;
    }
    
    
    
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

        return queues.is_complete() && extensions && adequateSwapChain;
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



    QueueSetup find_queue_families(const InstanceSetup &setup, const VkPhysicalDevice &physicalDevice) {
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

            if (!setup.surface.has_value()) {
                throw std::runtime_error("Tried to find suitable queue families without specifying a surface in setup.");
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

        return queues;
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



    VkDevice create_logical_device(InstanceSetup *setup) {
        if (!setup->physicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a logical device without specifying any physical device in the setup.");
        }

        if (!setup->queues.has_value()) {
            throw std::runtime_error("tried to create a logical device without specifying any queues in the setup.");
        }

        std::set<uint32_t> uniqueQueues = { setup->queues.value().graphicsIndex.value(), setup->queues.value().presentIndex.value() };
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
        
        const uint32_t qs[] = { setup.queues.value().graphicsIndex.value(), setup.queues.value().presentIndex.value() };
        if (setup.queues.value().graphicsIndex != setup.queues.value().presentIndex) {
            swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapChainCreateInfo.queueFamilyIndexCount = 2;
            swapChainCreateInfo.pQueueFamilyIndices = &qs[0];
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
        VkAttachmentDescription colorAttachment{};

        if (!setup.swapChainConfig.has_value()) {
            throw std::runtime_error("Tried to create a render pass without providing a swap chain config in the setup.");
        }

        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a render pass without providing a logical device in the setup.");
        }

        colorAttachment.format = setup.swapChainConfig.value().surfaceFormat.format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

        colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentReference{};
        colorAttachmentReference.attachment = 0;
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDescription{};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorAttachmentReference;

        VkSubpassDependency subpassDependency{};
        subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency.srcAccessMask = 0;
        subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPass renderPass{};
        VkRenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = 1;
        renderPassCreateInfo.pAttachments = &colorAttachment;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpassDescription;
        renderPassCreateInfo.dependencyCount = 1;
        renderPassCreateInfo.pDependencies = &subpassDependency;

        if (vkCreateRenderPass(setup.logicalDevice.value(), &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't create render pass.");
        }

        return renderPass;
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

        VkVertexInputBindingDescription                  bindingDesc   = Vertex2D::get_binding_description();
        std::array<VkVertexInputAttributeDescription, 2> attributeDesc = Vertex2D::get_attribute_description();

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
        rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
        multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        
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
        
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 0;

        if (!setup.logicalDevice.value()) {
            throw std::runtime_error("Tried to create a pipeline layout without providing a logical device in the setup.");
        }

        if (vkCreatePipelineLayout(setup.logicalDevice.value(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't create graphics pipeline layout.");
        }

        VkRenderPass renderPass = create_render_pass(setup);

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

        std::vector<VkFramebuffer> newFramebuffers(setup.swapChainImageViews.size());

        for (size_t i = 0; i != setup.swapChainImageViews.size(); ++i) {
            VkImageView attachments[] = { setup.swapChainImageViews[i] };

            VkFramebufferCreateInfo framebufferCreateInfo{};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.renderPass = setup.graphicsPipelineConfig.value().renderPass;
            framebufferCreateInfo.attachmentCount = 1;
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



    VkCommandPool create_command_pool(const InstanceSetup &setup) {
        if (!setup.queues.has_value()) {
            throw std::runtime_error("Tried to create a command pool without providing queues in the setup.");
        }

        if (!setup.queues.value().graphicsIndex.has_value()) {
            throw std::runtime_error("Tried to create a command pool without providing a graphics queue family index in the setup.");
        }

        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a command pool without providing a logical device in the setup.");
        }

        VkCommandPoolCreateInfo commandPoolCreateInfo{};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = setup.queues.value().graphicsIndex.value();

        VkCommandPool newCommandPool;
        if (vkCreateCommandPool(setup.logicalDevice.value(), &commandPoolCreateInfo, nullptr, &newCommandPool) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't create command pool.");
        }

        return newCommandPool;
    }



    WrappedBuffer create_vertex_buffer(const InstanceSetup &setup, const std::vector<Vertex2D> &vertices) {
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a vertex buffer without providing a logical device in the setup");
        }

        WrappedBuffer newBuffer;
        
        VkBufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = sizeof(vertices[0]) * vertices.size();
        bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(setup.logicalDevice.value(), &bufferCreateInfo, nullptr, &newBuffer.buffer) != VK_SUCCESS) {
            throw std::runtime_error("Could not create vertex buffer.");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(setup.logicalDevice.value(), newBuffer.buffer, &memoryRequirements);

        VkMemoryAllocateInfo bufferAllocateInfo{};
        bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        bufferAllocateInfo.allocationSize = memoryRequirements.size;
        bufferAllocateInfo.memoryTypeIndex = find_memory_type(setup.physicalDevice.value(), memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(setup.logicalDevice.value(), &bufferAllocateInfo, nullptr, &newBuffer.memory) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't allocate buffer memory.");
        }

        vkBindBufferMemory(setup.logicalDevice.value(), newBuffer.buffer, newBuffer.memory, 0);

        void *data;
        vkMapMemory(setup.logicalDevice.value(), newBuffer.memory, 0, bufferCreateInfo.size, 0, &data);
        memcpy_s(data, bufferCreateInfo.size, vertices.data(), vertices.size()*sizeof(vertices[0]));
        vkUnmapMemory(setup.logicalDevice.value(), newBuffer.memory);

        return newBuffer;
    }


    
    std::vector<VkCommandBuffer> create_command_buffers(const InstanceSetup &setup) {
        if (!setup.logicalDevice.has_value()) {
            throw std::runtime_error("Tried to create a command buffer without providing a logical device in the setup.");
        }

        if (!setup.commandPool.has_value()) {
            throw std::runtime_error("Tried to create a command buffer without providing a command pool in the buffer.");
        }

        std::vector<VkCommandBuffer> newCommandBuffers(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo commandBufferAllocationInfo{};
        commandBufferAllocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocationInfo.commandPool = setup.commandPool.value();
        commandBufferAllocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocationInfo.commandBufferCount = static_cast<uint32_t>(newCommandBuffers.size());

        if (vkAllocateCommandBuffers(setup.logicalDevice.value(), &commandBufferAllocationInfo, newCommandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't allocate command buffers");
        }

        return newCommandBuffers;
    }



    void record_command_buffer(const InstanceSetup &setup, const VkCommandBuffer &commandBuffer, uint32_t imageIndex) {
        if (!setup.graphicsPipelineConfig.has_value()) {
            throw std::runtime_error("Tried to record a command buffer without providing a graphics pipeline to the setup.");
        }

        if (!setup.swapChainConfig.has_value()) {
            throw std::runtime_error("Tried to record a command buffer without providing a swap chain config in the setup.");
        }

        if (!setup.vertexBuffer.has_value()) {
            throw std::runtime_error("Tried to record a command buffer without providing a vertex buffer in the setup.");
        }

        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't record command buffer (beginning).");
        }

        VkClearValue clearColor = {{{0.8f, 0.0f, 0.8f, 1.0f}}};

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = setup.graphicsPipelineConfig.value().renderPass;
        renderPassBeginInfo.framebuffer = setup.swapChainFramebuffers[imageIndex];
        renderPassBeginInfo.renderArea.extent = setup.swapChainConfig.value().extent;
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, setup.graphicsPipelineConfig.value().pipeline);

        VkBuffer     vertexBuffers[] = { setup.vertexBuffer.value().buffer };
        VkDeviceSize offsets[]       = { 0 };

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffers[0], &offsets[0]);

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

        vkCmdDraw(commandBuffer, static_cast<uint32_t>(EXAMPLE_VERTICES.size()), 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer (end)");
        }
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

        vkResetFences(setup->logicalDevice.value(), 1, &setup->syncObjects.value().inFlightFences[*currentFrame]);

        vkResetCommandBuffer(setup->commandBuffers[*currentFrame], NULL);

        record_command_buffer(*setup, setup->commandBuffers[*currentFrame], imageIndex);


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



    void cleanup_swap_chain(const InstanceSetup &setup) {
        for (const VkFramebuffer &framebuffer : setup.swapChainFramebuffers) {
            vkDestroyFramebuffer(setup.logicalDevice.value(), framebuffer, nullptr);
        }

        for (const VkImageView &imageView : setup.swapChainImageViews) {
            vkDestroyImageView(setup.logicalDevice.value(), imageView, nullptr);
        }

        vkDestroySwapchainKHR(setup.logicalDevice.value(), setup.swapChain.value(), nullptr);
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
        setup->swapChainFramebuffers = create_framebuffers(*setup);
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

        vkDestroyCommandPool(setup.logicalDevice.value(), setup.commandPool.value(), nullptr);

        cleanup_swap_chain(setup);

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
}
