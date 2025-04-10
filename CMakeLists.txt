CMAKE_MINIMUM_REQUIRED(VERSION 3.23)

PROJECT(finalHope VERSION 0.0.2 LANGUAGES CXX C)

ADD_EXECUTABLE(fhope src/fhope-main.cpp
                     src/setup.cpp
                     src/vertex.cpp
                     src/mvp.cpp)

TARGET_INCLUDE_DIRECTORIES(fhope PUBLIC include)


SET_TARGET_PROPERTIES(fhope  PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)

ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -DWIN32)

ADD_SUBDIRECTORY(${CMAKE_SOURCE_DIR}/submodules/glad/cmake)

glad_add_library(glad_vulkan_12 REPRODUCIBLE LOADER API vulkan=1.2)

ADD_SUBDIRECTORY(${CMAKE_SOURCE_DIR}/submodules/googletest)

ADD_SUBDIRECTORY(${CMAKE_SOURCE_DIR}/submodules/glslang)

ADD_SUBDIRECTORY(${CMAKE_SOURCE_DIR}/submodules/shaderc)

ADD_SUBDIRECTORY(${CMAKE_SOURCE_DIR}/submodules/glfw)

ADD_SUBDIRECTORY(${CMAKE_SOURCE_DIR}/submodules/glm)

ADD_CUSTOM_TARGET(copy-shaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_BINARY_DIR}/shaders
    DEPENDS fhope
)

TARGET_LINK_LIBRARIES(fhope glad_vulkan_12 glfw glm::glm shaderc)
