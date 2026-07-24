cmake_minimum_required(VERSION 3.14)
project(lvgl_simulator C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

# Screen resolution (match microcontroller)
set(DISP_HOR_RES 320)
set(DISP_VER_RES 240)

# Use LV_CONF_PATH to specify the exact config file path
add_compile_definitions(
    LV_CONF_PATH="${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h"
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/elrs/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/telemetry/*.c
)

# Simulator sources
set(SIM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hal_stubs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lv_drv_sdl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings_stub.c
    ${CMAKE_CURRENT_SOURCE_DIR}/elrs_sim.c
)

add_executable(simulator
    ${LVGL_SOURCES}
    ${UI_SOURCES}
    ${SIM_SOURCES}
)

add_executable(test_elrs_service
    ${CMAKE_CURRENT_SOURCE_DIR}/test_elrs_service.c
    ${CMAKE_CURRENT_SOURCE_DIR}/elrs_sim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/elrs/elrs_client.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/elrs/elrs_service.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/elrs/crsf_protocol.c
)

add_executable(test_hexapod_telemetry
    ${CMAKE_CURRENT_SOURCE_DIR}/test_hexapod_telemetry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/telemetry/hexapod_telemetry.c
)

# IMPORTANT: Simulator directory must come BEFORE LVGL_DIR so our lv_conf.h is found first!
target_include_directories(simulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${LVGL_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/elrs
)

target_include_directories(test_elrs_service PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/elrs
)

target_include_directories(test_hexapod_telemetry PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Link SDL2 properly (use full library path on macOS)
target_link_libraries(simulator PRIVATE
    ${SDL2_LINK_LIBRARIES}
    m
    pthread
)

enable_testing()
add_test(NAME test_elrs_service COMMAND test_elrs_service)
add_test(NAME test_hexapod_telemetry COMMAND test_hexapod_telemetry)
add_test(NAME test_ui_smoke COMMAND simulator --smoke-test)

# Add compiler flags from pkg-config
target_compile_options(simulator PRIVATE ${SDL2_CFLAGS_OTHER} -g)

# Copy lv_conf.h to build directory if needed
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h
    ${CMAKE_BINARY_DIR}/lv_conf.h
    COPYONLY
)
