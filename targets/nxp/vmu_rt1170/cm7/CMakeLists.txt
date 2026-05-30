cmake_minimum_required(VERSION 3.20)

get_filename_component(RTFRAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)

include("${RTFRAME_ROOT}/cmake/toolchain.cmake")

set(BOARD vmu_rt1170/mimxrt1176/cm7)

include("${RTFRAME_ROOT}/cmake/zephyr.cmake")

project(rtframe_cm7 LANGUAGES C CXX ASM)

target_sources(app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
target_include_directories(app PRIVATE "${RTFRAME_ROOT}/src")

set(LSS_FILE ${CMAKE_BINARY_DIR}/zephyr/zephyr.lss)
set(ELF_FILE ${CMAKE_BINARY_DIR}/zephyr/zephyr.elf)

add_custom_target(gen_lss ALL
  COMMAND ${CMAKE_OBJDUMP} -S -d ${ELF_FILE} > ${LSS_FILE}
  DEPENDS ${ELF_FILE}
  COMMENT "Generating zephyr.lss"
  VERBATIM
)
add_dependencies(gen_lss zephyr_final)
