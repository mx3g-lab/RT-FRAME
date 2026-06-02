if(CONFIG_RTFRAME_DATAMAN_CLIENT)
    zephyr_library_named(rtframe_dataman_client)

    zephyr_library_sources_ifdef(CONFIG_RTFRAME_DATAMAN_CLIENT
        DatamanClient.cpp
    )

    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src/include
    )
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

    zephyr_library_link_libraries(rtframe_hrt)
endif()
