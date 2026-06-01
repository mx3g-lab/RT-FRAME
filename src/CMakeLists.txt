zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory_ifdef(CONFIG_RTFRAME_CORE       core)
add_subdirectory_ifdef(CONFIG_RTFRAME_LIB        lib)
add_subdirectory_ifdef(CONFIG_RTFRAME_DRIVERS    drivers)
add_subdirectory_ifdef(CONFIG_RTFRAME_MODULES    modules)
add_subdirectory_ifdef(CONFIG_RTFRAME_MIDDLEWARE middleware)
