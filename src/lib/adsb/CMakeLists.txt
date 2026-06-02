if(CONFIG_RTFRAME_ADSB)
    zephyr_library_named(rtframe_adsb)

    zephyr_library_sources_ifdef(CONFIG_RTFRAME_ADSB
        AdsbConflict.cpp
    )

    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src/include
    )
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

    zephyr_library_link_libraries(rtframe_geo rtframe_hrt)
endif()
