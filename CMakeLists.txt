cmake_minimum_required(VERSION 3.28)
project(cg-exam-24)

set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} src/CG.cpp)

# Add GLFW3 precompiled library
find_package(glfw3 REQUIRED)

# Headers (+ glm)
include_directories(src/headers)

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} glfw Vulkan::Vulkan)


# copy the shader files to the cmake-build-debug folder
file(COPY src/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
# copy the texture files to the cmake-build-debug folder
file(COPY src/textures DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
# copy the model files to the cmake-build-debug folder
file(COPY src/models DESTINATION ${CMAKE_CURRENT_BINARY_DIR})