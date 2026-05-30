# Toolchain detection for rtframe
# Priority:
#   1. ZEPHYR_SDK_INSTALL_DIR (cmake var or env)
#   2. toolchain/zephyr-sdk-* inside repo (installed by tools/setup_env.sh)
#   3. GNUARMEMB_TOOLCHAIN_PATH (cmake var or env)
#   4. System PATH arm-none-eabi-gcc

# --- Zephyr SDK path resolution ---
if(NOT DEFINED ZEPHYR_SDK_INSTALL_DIR AND DEFINED ENV{ZEPHYR_SDK_INSTALL_DIR})
  set(ZEPHYR_SDK_INSTALL_DIR $ENV{ZEPHYR_SDK_INSTALL_DIR})
endif()

# repo-local SDK
if(NOT DEFINED ZEPHYR_SDK_INSTALL_DIR)
  get_filename_component(_repo_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
  file(GLOB _sdk_candidates "${_repo_root}/toolchain/zephyr-sdk-*")
  foreach(_candidate ${_sdk_candidates})
    if(EXISTS "${_candidate}/sdk_version")
      set(ZEPHYR_SDK_INSTALL_DIR "${_candidate}")
      break()
    endif()
  endforeach()
endif()

if(DEFINED ZEPHYR_SDK_INSTALL_DIR AND EXISTS "${ZEPHYR_SDK_INSTALL_DIR}/sdk_version")
  set(ZEPHYR_SDK_INSTALL_DIR "${ZEPHYR_SDK_INSTALL_DIR}" CACHE PATH "")
  set(ZEPHYR_TOOLCHAIN_VARIANT zephyr CACHE STRING "")
  # 直接指向 repo 内 SDK 的 cmake 目录，阻止 find_package 搜索系统 SDK
  set(Zephyr-sdk_DIR "${ZEPHYR_SDK_INSTALL_DIR}/cmake" CACHE PATH "")
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
      "  2. Run: bash tools/setup_env.sh\n"
      "  3. Set GNUARMEMB_TOOLCHAIN_PATH to a gnuarmemb root\n"
      "  4. Ensure arm-none-eabi-gcc is in PATH"
    )
  endif()
endif()

set(ZEPHYR_TOOLCHAIN_VARIANT gnuarmemb CACHE STRING "")
set(GNUARMEMB_TOOLCHAIN_PATH "${GNUARMEMB_TOOLCHAIN_PATH}" CACHE PATH "")
