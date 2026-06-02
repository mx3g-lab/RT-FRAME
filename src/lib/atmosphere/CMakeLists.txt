if(CONFIG_RTFRAME_ATMOSPHERE)
    zephyr_library_named(rtframe_atmosphere)

    zephyr_library_sources_ifdef(CONFIG_RTFRAME_ATMOSPHERE
        atmosphere.cpp
    )

    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src/include
    )
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
endif()
