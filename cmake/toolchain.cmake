# Toolchain detection for rtframe
# Priority:
#   1. ZEPHYR_SDK_INSTALL_DIR (cmake var or env) — preferred, Zephyr-native
#   2. GNUARMEMB_TOOLCHAIN_PATH (cmake var or env) — generic gnuarmemb
#   3. Well-known Zephyr SDK locations
#   4. System PATH arm-none-eabi-gcc

# --- Zephyr SDK path resolution ---
if(NOT DEFINED ZEPHYR_SDK_INSTALL_DIR AND DEFINED ENV{ZEPHYR_SDK_INSTALL_DIR})
  set(ZEPHYR_SDK_INSTALL_DIR $ENV{ZEPHYR_SDK_INSTALL_DIR})
endif()

if(NOT DEFINED ZEPHYR_SDK_INSTALL_DIR)
  foreach(_candidate
      "/home/mx3g/work_space/rtos/zephyr/GCC"
      "/opt/zephyr-sdk"
      "$ENV{HOME}/zephyr-sdk"
  )
    if(EXISTS "${_candidate}/sdk_version")
      set(ZEPHYR_SDK_INSTALL_DIR "${_candidate}")
      break()
    endif()
  endforeach()
endif()

if(DEFINED ZEPHYR_SDK_INSTALL_DIR AND EXISTS "${ZEPHYR_SDK_INSTALL_DIR}/sdk_version")
  set(ZEPHYR_SDK_INSTALL_DIR "${ZEPHYR_SDK_INSTALL_DIR}" CACHE PATH "")
  set(ZEPHYR_TOOLCHAIN_VARIANT zephyr CACHE STRING "")
  message(STATUS "Toolchain: Zephyr SDK at ${ZEPHYR_SDK_INSTALL_DIR}")
  return()
endif()

# --- Fallback: gnuarmemb ---
if(NOT DEFINED GNUARMEMB_TOOLCHAIN_PATH AND DEFINED ENV{GNUARMEMB_TOOLCHAIN_PATH})
  set(GNUARMEMB_TOOLCHAIN_PATH $ENV{GNUARMEMB_TOOLCHAIN_PATH})
endif()

if(NOT DEFINED GNUARMEMB_TOOLCHAIN_PATH)
  find_program(_gcc_path arm-none-eabi-gcc)
  if(_gcc_path)
    get_filename_component(_gcc_dir "${_gcc_path}" DIRECTORY)
    get_filename_component(GNUARMEMB_TOOLCHAIN_PATH "${_gcc_dir}/.." ABSOLUTE)
    message(STATUS "Toolchain: system arm-none-eabi at ${GNUARMEMB_TOOLCHAIN_PATH}")
  else()
    message(FATAL_ERROR
      "No ARM toolchain found.\n"
      "Options:\n"
      "  1. Set ZEPHYR_SDK_INSTALL_DIR to the Zephyr SDK root\n"
      "  2. Set GNUARMEMB_TOOLCHAIN_PATH to a gnuarmemb root\n"
      "  3. Ensure arm-none-eabi-gcc is in PATH"
    )
  endif()
endif()

set(ZEPHYR_TOOLCHAIN_VARIANT gnuarmemb CACHE STRING "")
set(GNUARMEMB_TOOLCHAIN_PATH "${GNUARMEMB_TOOLCHAIN_PATH}" CACHE PATH "")
