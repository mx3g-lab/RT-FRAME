if(CONFIG_RTFRAME_TIMESYNC)
    zephyr_library_named(rtframe_timesync)

    zephyr_library_sources_ifdef(CONFIG_RTFRAME_TIMESYNC
        Timesync.cpp
    )

    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src/include
    )
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

    zephyr_library_link_libraries(rtframe_hrt)
endif()
