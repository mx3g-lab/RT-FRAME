zephyr_library_named(rtframe_test_sdcard)

zephyr_library_sources_ifdef(CONFIG_RTFRAME_TEST_SDCARD
    test_sdcard.cpp
)

zephyr_library_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${RTFRAME_ROOT}/src/core
)
zephyr_library_link_libraries(rtframe_core)
