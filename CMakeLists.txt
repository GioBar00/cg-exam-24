cmake_minimum_required(VERSION 3.28)
project(cg-exam-24)

set(CMAKE_CXX_STANDARD 20)

# Headers (+ glm)
include_directories(headers/)
# Add GLFW3 precompiled library
find_package(glfw3 REQUIRED)
# Vulkan
find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME} App.cpp)
target_link_libraries(${PROJECT_NAME} glfw Vulkan::Vulkan)

# Find GLSLC
find_program(GLSLC_EXECUTABLE NAMES glslc HINTS Vulkan::glslc)

# If GLSLC is not found, copy the shader files to the cmake-build-debug folder
if (NOT GLSLC_EXECUTABLE)
    message(WARNING "GLSLC not found, copying compiled shader files if they exist")
else ()
    message(STATUS "GLSLC found at ${GLSLC_EXECUTABLE}")
    message(STATUS "Compiling shaders")

    # Compile shaders
    function(compile_shader GLSL_FILE)
        add_custom_command(
                OUTPUT ${GLSL_FILE}.spv
                COMMAND ${GLSLC_EXECUTABLE} ${GLSL_FILE} -o ${GLSL_FILE}.spv
                MAIN_DEPENDENCY ${GLSL_FILE}
        )
        target_sources(${PROJECT_NAME} PRIVATE ${GLSL_FILE}.spv)
    endfunction()
    message(STATUS "Compiling shaders")
    file(GLOB SHADERS "shaders/*.vert" "shaders/*.frag")
    foreach(SHADER ${SHADERS})
        compile_shader(${SHADER})
    endforeach()
    message(STATUS "Shaders compiled")
endif ()

message(STATUS "Copying textures, models and shaders")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/textures
        ${CMAKE_CURRENT_BINARY_DIR}/textures
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/models
        ${CMAKE_CURRENT_BINARY_DIR}/models
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.spv
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/
)

# Set ADDITIONAL_CLEAN_FILES to the list of files to be cleaned adding the copied files
set_target_properties(${PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES "${CMAKE_CURRENT_BINARY_DIR}/shaders/;${CMAKE_CURRENT_BINARY_DIR}/textures/;${CMAKE_CURRENT_BINARY_DIR}/models/")
