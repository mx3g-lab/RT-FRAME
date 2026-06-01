zephyr_library_named(rtframe_test_perf)
zephyr_library_sources_ifdef(CONFIG_RTFRAME_TEST_PERF
    test_perf.cpp
)
zephyr_library_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${RTFRAME_ROOT}/src/core
    ${RTFRAME_ROOT}/src/lib/perf
)
zephyr_library_link_libraries(rtframe_core rtframe_hrt rtframe_perf)
