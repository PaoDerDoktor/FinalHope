#include "fhope-main.hpp"

#include <cstdint>
#include <stdexcept>
#include <vector>
#include <iostream>

#define GLAD_VULKAN_IMPLEMENTATION
#include <glad/vulkan.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "setu.hpp"

static VKAPI_ATTR VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void *pUserData) {
    std::cerr << "Validation Layer : " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

int main(int argc, char const *argv[]) {
    fhope::initialize_dependencies();

    GLFWwindow *window = glfwCreateWindow(800, 600, "hope", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    fhope::InstanceSetup setup;
    try {
        setup = fhope::generate_vulkan_setup(window, "Test", {0, 0, 1}, "shaders/base.v.glsl", "shaders/base.f.glsl");
    } catch(const std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        fhope::draw_frame(setup);
    }

    vkDeviceWaitIdle(setup.logicalDevice.value());

    fhope::clean_setup(setup);

    glfwDestroyWindow(window);

    
    std::cout << glm::vec2(1.0f).length() << std::endl;
    fhope::terminate_dependencies();
    return 0;
}
