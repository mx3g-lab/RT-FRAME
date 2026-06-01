zephyr_library_named(rtframe_test_ringbuffer)
zephyr_library_sources_ifdef(CONFIG_RTFRAME_TEST_RINGBUFFER
    test_ringbuffer.cpp
)
zephyr_library_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${RTFRAME_ROOT}/src/core
    ${RTFRAME_ROOT}/src/lib/ringbuffer
)
zephyr_library_link_libraries(rtframe_core rtframe_ringbuffer)
