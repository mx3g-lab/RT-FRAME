zephyr_library_named(rtframe_param_loader)
zephyr_library_sources(param_loader.cpp)
zephyr_library_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${RTFRAME_ROOT}/src/core
)
zephyr_library_link_libraries(rtframe_core rtframe_parameters)
