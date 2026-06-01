if(CONFIG_RTFRAME_LOGGER)
    zephyr_library_named(rtframe_logger)

    zephyr_library_sources_ifdef(CONFIG_RTFRAME_LOGGER
        logger.cpp
        logger_task.cpp
        logged_topics.cpp
        log_writer.cpp
        log_writer_file.cpp
        log_writer_mavlink.cpp
        util.cpp
        util_parse.cpp
        watchdog.cpp
    )

    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src/core
        ${RTFRAME_ROOT}/src/include
        ${RTFRAME_ROOT}/src/lib
        ${RTFRAME_ROOT}/src/middleware/uorb
    )

    zephyr_library_compile_options(-Wno-cast-align -Wno-sign-compare)

    zephyr_library_link_libraries(rtframe_core rtframe_hrt rtframe_perf rtframe_parameters rtframe_uorb rtframe_heatshrink)

    if(CONFIG_RTFRAME_MAVLINK_LOG)
        zephyr_library_link_libraries(rtframe_mavlink_log)
    endif()
endif()
