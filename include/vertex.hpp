#pragma once

#include <array>

#include <glm/glm.hpp>
#include <glad/vulkan.h>

namespace fhope {
    struct Vertex2D {
        glm::vec2 position;
        glm::vec3 color;
        glm::vec2 uv;


        static constexpr VkVertexInputBindingDescription get_binding_description() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex2D);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            
            return bindingDescription;
        }


        static constexpr std::array<VkVertexInputAttributeDescription, 3> get_attribute_description() {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescription;
            // POSITION
            attributeDescription[0].binding  = 0;
            attributeDescription[0].location = 0;
            attributeDescription[0].format   = VK_FORMAT_R32G32_SFLOAT;
            attributeDescription[0].offset   = offsetof(Vertex2D, position);

            // COLOR
            attributeDescription[1].binding  = 0;
            attributeDescription[1].location = 1;
            attributeDescription[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescription[1].offset   = offsetof(Vertex2D, color);

            // UV
            attributeDescription[2].binding  = 0;
            attributeDescription[2].location = 2;
            attributeDescription[2].format   = VK_FORMAT_R32G32_SFLOAT;
            attributeDescription[2].offset   = offsetof(Vertex2D, uv);

            return attributeDescription;
        }
    };

    struct Vertex3D {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec2 uv;


        static constexpr VkVertexInputBindingDescription get_binding_description() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex3D);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            
            return bindingDescription;
        }


        static constexpr std::array<VkVertexInputAttributeDescription, 3> get_attribute_description() {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescription;
            // POSITION
            attributeDescription[0].binding  = 0;
            attributeDescription[0].location = 0;
            attributeDescription[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescription[0].offset   = offsetof(Vertex3D, position);

            // COLOR
            attributeDescription[1].binding  = 0;
            attributeDescription[1].location = 1;
            attributeDescription[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescription[1].offset   = offsetof(Vertex3D, color);

            // UV
            attributeDescription[2].binding  = 0;
            attributeDescription[2].location = 2;
            attributeDescription[2].format   = VK_FORMAT_R32G32_SFLOAT;
            attributeDescription[2].offset   = offsetof(Vertex3D, uv);

            return attributeDescription;
        }
    };
}
