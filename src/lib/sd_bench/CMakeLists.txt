if(CONFIG_RTFRAME_SD_BENCH)
    zephyr_library_named(rtframe_sd_bench)

    zephyr_library_sources_ifdef(CONFIG_RTFRAME_SD_BENCH
        sd_bench.cpp
    )

    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src/include
    )
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
endif()
