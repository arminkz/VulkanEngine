cmake_minimum_required(VERSION 3.10)
project(VulkanEngine)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (WIN32)
    set(VULKAN_SDK $ENV{VULKAN_SDK})
    if (NOT VULKAN_SDK)
        message(FATAL_ERROR "Vulkan SDK not found! Make sure it's installed.")
    endif()
    set(CMAKE_INCLUDE_PATH ${VULKAN_SDK}/Include)
    set(CMAKE_LIBRARY_PATH ${VULKAN_SDK}/Lib)
endif()

find_package(Vulkan REQUIRED)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# Add dependencies
add_subdirectory(external/spdlog)
add_subdirectory(external/glm)
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)
add_subdirectory(external/imgui)

# Libraries
set(LIBRARIES
    Vulkan::Vulkan
    spdlog::spdlog
    glm::glm
    SDL3::SDL3
    imgui
)

# Handle .cpp files
file(GLOB SOURCES
    src/*
    src/font/*
    src/geometry/*
    src/loader/*
    src/utilities/*
    src/imgui-impl/*
)

# Add resources.rc file to the sources if on Windows
if (WIN32)
    list(APPEND SOURCES resources.rc)
endif()

# Handle .h files
set(INCLUDES ${INCLUDES} src)
set(INCLUDES ${INCLUDES} external/stb)
set(INCLUDES ${INCLUDES} external/tinyobjloader)
set(INCLUDES ${INCLUDES} external/imgui)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDES})
message(STATUS "I hate myself so I use cmake!")

# Copy shader files to the build directory
file(GLOB SHADER_FILES spv/*.spv)
foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/spv $<TARGET_FILE_DIR:${PROJECT_NAME}>/spv
        COMMAND ${CMAKE_COMMAND} -E echo "Copying shader file: ${SHADER_NAME}"
    )
endforeach()

# Copy texture folder to the build directory
file(GLOB TEXTURE_FILES textures/*)
foreach(TEXTURE_FILE ${TEXTURE_FILES})
    get_filename_component(TEXTURE_NAME ${TEXTURE_FILE} NAME)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/textures $<TARGET_FILE_DIR:${PROJECT_NAME}>/textures
        COMMAND ${CMAKE_COMMAND} -E echo "Copying texture file: ${TEXTURE_NAME}"
    )
endforeach()

# Copy model folder to the build directory
file(GLOB MODEL_FILES models/*)
foreach(MODEL_FILE ${MODEL_FILES})
    get_filename_component(MODEL_NAME ${MODEL_FILE} NAME)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/models $<TARGET_FILE_DIR:${PROJECT_NAME}>/models
        COMMAND ${CMAKE_COMMAND} -E echo "Copying model file: ${MODEL_NAME}"
    )
endforeach()

# Copy font folder to the build directory
file(GLOB FONT_FILES fonts/*)
foreach(FONT_FILE ${FONT_FILES})
    get_filename_component(FONT_NAME ${FONT_FILE} NAME)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:${PROJECT_NAME}>/fonts
        COMMAND ${CMAKE_COMMAND} -E echo "Copying font file: ${FONT_NAME}"
    )
endforeach()