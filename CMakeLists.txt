cmake_minimum_required(VERSION 3.10)
project(viture_head_mouse VERSION 1.0.0)

# Option for static build
option(BUILD_STATIC "Build with static linking where possible" OFF)

# Find X11 and XTest for the X11 version
find_package(X11 REQUIRED)
find_path(XTEST_INCLUDE_DIR X11/extensions/XTest.h
          PATHS ${X11_INCLUDE_DIR})

# Check for Wayland (informational only)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(WAYLAND QUIET wayland-client)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${X11_INCLUDE_DIR})
include_directories(${XTEST_INCLUDE_DIR})

# Add libviture_one_sdk
if(BUILD_STATIC AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libs/libviture_one_sdk_static.a)
    add_library(viture_one_sdk STATIC IMPORTED)
    set_target_properties(viture_one_sdk PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libs/libviture_one_sdk_static.a)
else()
    add_library(viture_one_sdk SHARED IMPORTED)
    set_target_properties(viture_one_sdk PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libs/libviture_one_sdk.so)
endif()

# Set static linking flags if requested
if(BUILD_STATIC)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
    # Try to link statically with system libraries where possible
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static")
endif()

# X11 version - works with X11 display server
add_executable(head_mouse_x11 head_mouse.c config.c socket_server.c)
target_compile_definitions(head_mouse_x11 PRIVATE USE_X11)
target_link_libraries(head_mouse_x11
    viture_one_sdk
    ${X11_LIBRARIES}
    ${X11_Xtst_LIB}
    pthread
    m)

# Wayland compatible version using uinput
add_executable(head_mouse_wayland head_mouse_wayland.c config.c socket_server.c)
target_link_libraries(head_mouse_wayland
    viture_one_sdk
    pthread
    m)

# Control client tool
add_executable(viture-mouse-ctl viture-mouse-ctl.c)
if(NOT BUILD_STATIC)
    target_link_libraries(viture-mouse-ctl pthread)
endif()

# Create run scripts to set library path
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/run_head_mouse_x11.sh
"#!/bin/bash
export LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/libs:\$LD_LIBRARY_PATH
./head_mouse_x11 \$@
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/run_head_mouse_wayland.sh
"#!/bin/bash
export LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/libs:\$LD_LIBRARY_PATH

# Check if user is in input group and uinput is accessible
if groups | grep -q input && [ -r /dev/uinput ] && [ -w /dev/uinput ]; then
    echo 'Running without sudo (permissions configured)'
    ./head_mouse_wayland \$@
else
    echo 'Running with sudo (permissions not configured)'
    echo 'Run ./setup-permissions.sh to avoid needing sudo'
    sudo ./head_mouse_wayland \$@
fi
")

# Make the scripts executable
execute_process(COMMAND chmod +x ${CMAKE_CURRENT_BINARY_DIR}/run_head_mouse_x11.sh)
execute_process(COMMAND chmod +x ${CMAKE_CURRENT_BINARY_DIR}/run_head_mouse_wayland.sh)

# Installation
install(TARGETS head_mouse_x11 head_mouse_wayland viture-mouse-ctl DESTINATION bin)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/run_head_mouse_x11.sh
        ${CMAKE_CURRENT_BINARY_DIR}/run_head_mouse_wayland.sh
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

# Create build targets for specific platforms
add_custom_target(wayland DEPENDS head_mouse_wayland viture-mouse-ctl)
add_custom_target(x11 DEPENDS head_mouse_x11 viture-mouse-ctl)

# Print information about the build
message(STATUS "X11 libraries: ${X11_LIBRARIES}")
message(STATUS "XTest library: ${X11_Xtst_LIB}")
message(STATUS "Building both X11 and Wayland-compatible versions")
message(STATUS "")
message(STATUS "Build targets:")
message(STATUS "  make          - Build everything")
message(STATUS "  make wayland  - Build Wayland version only")
message(STATUS "  make x11      - Build X11 version only")