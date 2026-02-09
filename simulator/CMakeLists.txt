cmake_minimum_required(VERSION 3.14)
project(lvgl_simulator C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

# Screen resolution (match microcontroller)
set(DISP_HOR_RES 320)
set(DISP_VER_RES 240)

add_compile_definitions(
    LV_CONF_INCLUDE_SIMPLE
    SIMULATOR=1
    DISP_HOR_RES=${DISP_HOR_RES}
    DISP_VER_RES=${DISP_VER_RES}
)

# Find SDL2 using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# Add SDL2 library path for linker
link_directories(${SDL2_LIBRARY_DIRS})

# LVGL library path
set(LVGL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib/lvgl)

# Collect LVGL sources
file(GLOB_RECURSE LVGL_SOURCES
    ${LVGL_DIR}/src/*.c
)

# Collect UI sources from the project
file(GLOB UI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/components/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/screens/*.c
)

# Simulator sources
set(SIM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hal_stubs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lv_drv_sdl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings_stub.c
)

add_executable(simulator
    ${LVGL_SOURCES}
    ${UI_SOURCES}
    ${SIM_SOURCES}
)

target_include_directories(simulator PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${LVGL_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui
)

# Link SDL2 properly (use full library path on macOS)
target_link_libraries(simulator PRIVATE
    ${SDL2_LINK_LIBRARIES}
    m
    pthread
)

# Add compiler flags from pkg-config
target_compile_options(simulator PRIVATE ${SDL2_CFLAGS_OTHER})

# Copy lv_conf.h to build directory if needed
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h
    ${CMAKE_BINARY_DIR}/lv_conf.h
    COPYONLY
)
