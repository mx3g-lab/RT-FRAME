if(CONFIG_RTFRAME_VERSION)
    zephyr_library_named(rtframe_version)

    zephyr_library_sources_ifdef(CONFIG_RTFRAME_VERSION
        version.c
    )

    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src/include
    )
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

    # Git version generation (simplified - TODO: enhance later)
    set(BUILD_URI "localhost")
    if(DEFINED ENV{BUILD_URI})
        set(BUILD_URI $ENV{BUILD_URI})
    endif()

    zephyr_library_compile_definitions(
        BUILD_URI=${BUILD_URI}
        PX4_BOARD_NAME="vmu_rt1170"
        PX4_BOARD_LABEL="cm7"
    )
endif()
