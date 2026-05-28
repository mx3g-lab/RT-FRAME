cmake_minimum_required(VERSION 3.20)

get_filename_component(RTFRAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

include("${RTFRAME_ROOT}/cmake/toolchain.cmake")

set(BOARD vmu_rt1170/mimxrt1176/cm7)
set(BOARD_DIR "${RTFRAME_ROOT}/boards/nxp/vmu_rt1170" CACHE PATH "" FORCE)

include("${RTFRAME_ROOT}/cmake/zephyr.cmake")

project(rtframe_cm7 LANGUAGES C CXX ASM)

# Core and module sources
add_subdirectory("${RTFRAME_ROOT}/core"   "${CMAKE_BINARY_DIR}/core")
add_subdirectory("${RTFRAME_ROOT}/drivers" "${CMAKE_BINARY_DIR}/drivers")
add_subdirectory("${RTFRAME_ROOT}/lib"    "${CMAKE_BINARY_DIR}/lib")
add_subdirectory("${RTFRAME_ROOT}/modules" "${CMAKE_BINARY_DIR}/modules")

target_sources(app PRIVATE
  "${RTFRAME_ROOT}/src/main.cpp"
)

target_include_directories(app PRIVATE
  "${RTFRAME_ROOT}/src"
  "${RTFRAME_ROOT}/core/include"
)
