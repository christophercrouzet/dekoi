### TODO:
# - add shaders
###

cmake_minimum_required(VERSION 3.1)

project(Dekoi
    VERSION 0.1.0
    LANGUAGES C)

string(TOLOWER ${PROJECT_NAME} DK_PROJECT_NAME)

# ------------------------------------------------------------------------------

include(CMakePackageConfigHelpers)
include(CMakeParseArguments)
include(GNUInstallDirs)

# ------------------------------------------------------------------------------

option(DEKOI_ENABLE_DEMOS "Enable demo builds" ON)

# ------------------------------------------------------------------------------

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU"
    OR "${CMAKE_C_COMPILER_ID}" STREQUAL "CLANG")
    add_compile_options(
        -Wpedantic -Wall -Wextra -Waggregate-return -Wcast-align -Wcast-qual
        -Wconversion -Wfloat-equal -Wpointer-arith -Wshadow -Wstrict-overflow=5
        -Wswitch -Wswitch-default -Wundef -Wunreachable-code -Wwrite-strings)
endif()

# ------------------------------------------------------------------------------

find_package(Vulkan REQUIRED)

if(DEKOI_ENABLE_DEMOS)
    find_package(glfw3 REQUIRED)
endif()

# ------------------------------------------------------------------------------

add_subdirectory(deps/zero)

# ------------------------------------------------------------------------------

set(DK_MODULE_TARGETS)

macro(dk_get_module_public_headers target)
    set(${target}_DK_PUBLIC_HEADERS)
    file(GLOB DK_GET_MODULE_PUBLIC_HEADERS_REFS
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/include/${DK_PROJECT_NAME}
        include/${DK_PROJECT_NAME}/${target}/*)
    foreach(file ${DK_GET_MODULE_PUBLIC_HEADERS_REFS})
        list(APPEND ${target}_DK_PUBLIC_HEADERS src/${file})
    endforeach()
    unset(DK_GET_MODULE_PUBLIC_HEADERS_REFS)
endmacro()

macro(dk_add_module target)
    set(DK_ADD_MODULE_OPTIONS)
    set(DK_ADD_MODULE_SINGLE_VALUE_ARGS)
    set(DK_ADD_MODULE_MULTI_VALUE_ARGS FILES)
    cmake_parse_arguments(
        DK_ADD_MODULE
        "${DK_ADD_MODULE_OPTIONS}"
        "${DK_ADD_MODULE_SINGLE_VALUE_ARGS}"
        "${DK_ADD_MODULE_MULTI_VALUE_ARGS}"
        ${ARGN})

    dk_get_module_public_headers(${target})

    add_library(${target} ${DK_ADD_MODULE_FILES})
    set_target_properties(${target}
        PROPERTIES
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}
            ARCHIVE_OUTPUT_DIRECTORY lib
            LIBRARY_OUTPUT_DIRECTORY lib
            PUBLIC_HEADER "${${target}_DK_PUBLIC_HEADERS}")
    target_include_directories(${target}
        PUBLIC
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
    install(
        TARGETS ${target}
        EXPORT ${PROJECT_NAME}Targets
        PUBLIC_HEADER
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${DK_PROJECT_NAME}/${target}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${DK_PROJECT_NAME}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${DK_PROJECT_NAME})
    add_library(${PROJECT_NAME}::${target} ALIAS ${target})
    list(APPEND DK_MODULE_TARGETS ${target})
    unset(${target}_DK_PUBLIC_HEADERS)
endmacro()

dk_add_module(common
    FILES
        src/common/private/allocator.c
        src/common/private/allocator.h
        src/common/private/assert.h
        src/common/private/common.h
        src/common/private/logger.c
        src/common/private/logger.h
        src/common/allocator.h
        src/common/common.c
        src/common/common.h
        src/common/logger.c
        src/common/logger.h)
target_link_libraries(common
    PRIVATE
        Zero::allocator
        Zero::logger)

dk_add_module(graphics
    FILES
        src/graphics/renderer.c
        src/graphics/renderer.h)
target_link_libraries(graphics
    PUBLIC common
    PRIVATE Vulkan::Vulkan)

add_custom_target(modules DEPENDS ${DK_MODULE_TARGETS})

# ------------------------------------------------------------------------------

set(DK_DEMO_TARGETS)

macro(dk_add_demo target)
    set(DK_ADD_DEMO_OPTIONS)
    set(DK_ADD_DEMO_SINGLE_VALUE_ARGS)
    set(DK_ADD_DEMO_MULTI_VALUE_ARGS FILES)
    cmake_parse_arguments(
        DK_ADD_DEMO
        "${DK_ADD_DEMO_OPTIONS}"
        "${DK_ADD_DEMO_SINGLE_VALUE_ARGS}"
        "${DK_ADD_DEMO_MULTI_VALUE_ARGS}"
        ${ARGN})

    add_executable(demo-${target} ${DK_ADD_DEMO_FILES})
    set_target_properties(demo-${target}
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY bin/demos
            OUTPUT_NAME ${target})
    target_link_libraries(demo-${target}
        PRIVATE demo-common)
    list(APPEND DK_DEMO_TARGETS demo-${target})
endmacro()

if(DEKOI_ENABLE_DEMOS)
    add_library(demo-common
        demos/common/allocator.c
        demos/common/allocator.h
        demos/common/application.c
        demos/common/application.h
        demos/common/bootstrap.c
        demos/common/bootstrap.h
        demos/common/common.h
        demos/common/io.c
        demos/common/io.h
        demos/common/logger.c
        demos/common/logger.h
        demos/common/renderer.c
        demos/common/renderer.h
        demos/common/window.c
        demos/common/window.h)
    set_target_properties(demo-common
        PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY lib/demos
            LIBRARY_OUTPUT_DIRECTORY lib/demos
            OUTPUT_NAME common)
    target_link_libraries(demo-common
        PUBLIC ${DK_MODULE_TARGETS}
        PRIVATE
            Zero::allocator
            Zero::logger
            glfw)

    dk_add_demo(customallocator
        FILES
            demos/customallocator/allocator.c
            demos/customallocator/allocator.h
            demos/customallocator/main.c)

    dk_add_demo(customlogger
        FILES
            demos/customlogger/logger.c
            demos/customlogger/logger.h
            demos/customlogger/main.c)

    dk_add_demo(indexbuffer
        FILES demos/indexbuffer/main.c)

    dk_add_demo(triangle
        FILES demos/triangle/main.c)

    dk_add_demo(vertexbuffer
        FILES demos/vertexbuffer/main.c)

    add_custom_target(demos DEPENDS ${DK_DEMO_TARGETS})
endif()

# ------------------------------------------------------------------------------

set(DK_CMAKE_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

configure_package_config_file(
    cmake/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${DK_CMAKE_INSTALL_DIR})

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${DK_CMAKE_INSTALL_DIR})

install(
    EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${DK_CMAKE_INSTALL_DIR})
