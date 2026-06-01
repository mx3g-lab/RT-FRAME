zephyr_library_named(rtframe_demo_pub)
zephyr_library_sources_ifdef(CONFIG_RTFRAME_DEMO_PUB
    demo_pub.cpp
)
zephyr_library_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${RTFRAME_ROOT}/src/core
    ${RTFRAME_ROOT}/src/middleware/uorb
)
zephyr_library_link_libraries(rtframe_core rtframe_hrt rtframe_uorb)
