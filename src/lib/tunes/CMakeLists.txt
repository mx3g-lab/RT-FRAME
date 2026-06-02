if(CONFIG_RTFRAME_TUNES)
    zephyr_library_named(rtframe_tunes)

    zephyr_library_sources_ifdef(CONFIG_RTFRAME_TUNES
        tunes.cpp
        default_tunes.cpp
    )

    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src/include
    )
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

    zephyr_library_link_libraries(rtframe_circuit_breaker)
endif()
