cmake_minimum_required(VERSION 3.20)

get_filename_component(RTFRAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

include("${RTFRAME_ROOT}/cmake/toolchain.cmake")

set(BOARD native_sim/native/64)

include("${RTFRAME_ROOT}/cmake/zephyr.cmake")

project(rtframe_native LANGUAGES C CXX ASM)

target_sources(app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
target_include_directories(app PRIVATE "${RTFRAME_ROOT}/src")
