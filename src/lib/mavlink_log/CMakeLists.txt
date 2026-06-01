if(CONFIG_RTFRAME_MAVLINK_LOG)
    zephyr_library_named(rtframe_mavlink_log)

    zephyr_library_sources_ifdef(CONFIG_RTFRAME_UORB mavlink_log.cpp)

    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src/include
        ${RTFRAME_ROOT}/src/lib
        ${RTFRAME_ROOT}/src/middleware/uorb
    )

    if(CONFIG_RTFRAME_UORB)
        zephyr_library_link_libraries(rtframe_hrt rtframe_uorb)
    endif()
endif()
