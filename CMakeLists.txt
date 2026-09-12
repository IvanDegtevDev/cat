cmake_minimum_required(VERSION 3.28)

project(Vulkan
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
)

set(SLANGC_EXECUTABLE
    "C:/Users/cacet/slang/bin/slangc.exe"
)

set(SHADER_SLANG_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.slang
)



function(add_slang_shader_target TARGET)

    cmake_parse_arguments(
        "SHADER"
        ""
        ""
        "SOURCES"
        ${ARGN}
    )

    set(SHADERS_DIR
        ${CMAKE_CURRENT_LIST_DIR}/shaders
    )

    set(ENTRY_POINTS
        -entry vertMain
        -entry fragMain
    )

    add_custom_command(
        OUTPUT ${SHADERS_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
    )

    add_custom_command(
        OUTPUT ${SHADERS_DIR}/slang.spv

        COMMAND
            ${SLANGC_EXECUTABLE}
            ${SHADER_SOURCES}
            -target spirv
            -profile spirv_1_4
            -emit-spirv-directly
            -fvk-use-entrypoint-name
            ${ENTRY_POINTS}
            -o slang.spv

        WORKING_DIRECTORY ${SHADERS_DIR}

        DEPENDS
            ${SHADERS_DIR}
            ${SHADER_SOURCES}

        COMMENT "Compiling Slang Shaders"

        VERBATIM
    )

    add_custom_target(
        ${TARGET}
        DEPENDS ${SHADERS_DIR}/slang.spv
    )

endfunction()

add_slang_shader_target(
    foo
    SOURCES ${SHADER_SLANG_SOURCES}
)

add_dependencies(
    ${PROJECT_NAME}
    foo
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/textures
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/textures
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Vulkan::Vulkan
        glfw
        glm::glm
)


if (MSVC)

    target_compile_options(${PROJECT_NAME}
        PRIVATE
            /W4
            /permissive-
            /EHsc
        )

else()

    target_compile_options(${PROJECT_NAME}
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )

endif()

if (WIN32)

    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            VK_USE_PLATFORM_WIN32_KHR
            NOMINMAX
            WIN32_LEAN_AND_MEAN
        )

endif()