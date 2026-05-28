# Resolve ZEPHYR_BASE
# Priority: cmake var -> env var -> relative path os/zephyr inside this repo

if(NOT DEFINED ZEPHYR_BASE AND DEFINED ENV{ZEPHYR_BASE})
  set(ZEPHYR_BASE $ENV{ZEPHYR_BASE})
endif()

if(NOT DEFINED ZEPHYR_BASE)
  get_filename_component(_repo_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
  set(_local_zephyr "${_repo_root}/os/zephyr")
  if(EXISTS "${_local_zephyr}/CMakeLists.txt")
    set(ZEPHYR_BASE "${_local_zephyr}")
    message(STATUS "Using bundled Zephyr: ${ZEPHYR_BASE}")
  else()
    message(FATAL_ERROR
      "ZEPHYR_BASE not set and os/zephyr not present.\n"
      "Either set the environment variable ZEPHYR_BASE or place zephyr under os/zephyr/."
    )
  endif()
endif()

set(ZEPHYR_BASE "${ZEPHYR_BASE}" CACHE PATH "Zephyr base directory")

# Resolve ZEPHYR_MODULES: caller may pass extra paths, we always inject our boards dir
get_filename_component(_repo_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

list(APPEND ZEPHYR_EXTRA_MODULES "${_repo_root}")

# If ZEPHYR_MODULES env var is set, honour it (colon-separated on Linux)
if(DEFINED ENV{ZEPHYR_MODULES})
  string(REPLACE ":" ";" _env_modules "$ENV{ZEPHYR_MODULES}")
  list(APPEND ZEPHYR_EXTRA_MODULES ${_env_modules})
endif()

# Allow caller to pass additional module paths via -DRTFRAME_EXTRA_MODULES
if(DEFINED RTFRAME_EXTRA_MODULES)
  list(APPEND ZEPHYR_EXTRA_MODULES ${RTFRAME_EXTRA_MODULES})
endif()

list(REMOVE_DUPLICATES ZEPHYR_EXTRA_MODULES)
set(ZEPHYR_EXTRA_MODULES "${ZEPHYR_EXTRA_MODULES}" CACHE STRING "" FORCE)

find_package(Zephyr REQUIRED HINTS "${ZEPHYR_BASE}")
