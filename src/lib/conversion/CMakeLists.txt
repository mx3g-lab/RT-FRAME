if(CONFIG_RTFRAME_CONVERSION)
    zephyr_library_named(rtframe_conversion)

    zephyr_library_sources_ifdef(CONFIG_RTFRAME_CONVERSION
        rotation.cpp
    )

    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src/include
    )
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
endif()
