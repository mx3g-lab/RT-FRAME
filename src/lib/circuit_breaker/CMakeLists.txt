if(CONFIG_RTFRAME_CIRCUIT_BREAKER)
    zephyr_library_named(rtframe_circuit_breaker)

    zephyr_library_sources_ifdef(CONFIG_RTFRAME_CIRCUIT_BREAKER
        circuit_breaker.cpp
    )

    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src/include
    )
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

    zephyr_library_link_libraries(rtframe_parameters)
endif()
