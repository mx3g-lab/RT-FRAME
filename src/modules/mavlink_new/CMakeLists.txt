if(CONFIG_RTFRAME_MAVLINK)
    zephyr_library_named(rtframe_mavlink)

    # --- MAVLink C header generation ---
    set(MAVLINK_GIT_DIR "${CMAKE_CURRENT_LIST_DIR}/../mavlink/mavlink")
    set(MAVLINK_OUT_DIR "${CMAKE_BINARY_DIR}/mavlink_gen")
    set(MAVLINK_DIALECT ${CONFIG_RTFRAME_MAVLINK_DIALECT})

    # Generate uAvionix dialect (always)
    set(MAVLINK_UAVIONIX_OUT "${MAVLINK_OUT_DIR}/uAvionix/uAvionix.h")
    add_custom_command(
        OUTPUT ${MAVLINK_UAVIONIX_OUT}
        COMMAND ${PYTHON_EXECUTABLE} ${MAVLINK_GIT_DIR}/pymavlink/tools/mavgen.py
            --lang C --wire-protocol 2.0
            --output ${MAVLINK_OUT_DIR}
            ${MAVLINK_GIT_DIR}/message_definitions/v1.0/uAvionix.xml
        DEPENDS ${MAVLINK_GIT_DIR}/pymavlink/tools/mavgen.py
                ${MAVLINK_GIT_DIR}/message_definitions/v1.0/uAvionix.xml
        COMMENT "Generating MAVLink uAvionix dialect"
    )

    # Generate primary dialect
    set(MAVLINK_DIALECT_OUT "${MAVLINK_OUT_DIR}/${MAVLINK_DIALECT}/${MAVLINK_DIALECT}.h")
    add_custom_command(
        OUTPUT ${MAVLINK_DIALECT_OUT}
        COMMAND ${PYTHON_EXECUTABLE} ${MAVLINK_GIT_DIR}/pymavlink/tools/mavgen.py
            --lang C --wire-protocol 2.0
            --output ${MAVLINK_OUT_DIR}
            ${MAVLINK_GIT_DIR}/message_definitions/v1.0/${MAVLINK_DIALECT}.xml
        DEPENDS ${MAVLINK_GIT_DIR}/pymavlink/tools/mavgen.py
                ${MAVLINK_GIT_DIR}/message_definitions/v1.0/${MAVLINK_DIALECT}.xml
                ${MAVLINK_UAVIONIX_OUT}
        COMMENT "Generating MAVLink ${MAVLINK_DIALECT} dialect"
    )

    # INTERFACE library for generated MAVLink headers
    zephyr_interface_library_named(mavlink_headers)
    target_sources(mavlink_headers INTERFACE
        ${MAVLINK_DIALECT_OUT}
        ${MAVLINK_UAVIONIX_OUT}
    )
    target_include_directories(mavlink_headers INTERFACE
        ${MAVLINK_OUT_DIR}
        ${MAVLINK_OUT_DIR}/${MAVLINK_DIALECT}
        ${MAVLINK_OUT_DIR}/uAvionix
    )
    # Suppress packed-member and cast-align warnings from generated code
    target_compile_options(mavlink_headers INTERFACE -Wno-address-of-packed-member -Wno-cast-align)

    # --- Phase 2: incremental enable ---
    zephyr_library_sources(
        mavlink.c
        mavlink_rate_limiter.cpp
        mavlink_stream.cpp
        mavlink_log_handler.cpp
        mavlink_sign_control.cpp
        mavlink_messages.cpp
        mavlink_parameters.cpp
        mavlink_command_sender.cpp
        mavlink_simple_analyzer.cpp
        mavlink_main.cpp
        mavlink_receiver.cpp
        mavlink_timesync.cpp
        MavlinkStatustextHandler.cpp
        mavlink_shell.cpp
    )

    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src                    # resolves <lib/xxx.h>, <drivers/xxx.h> etc.
        ${RTFRAME_ROOT}/src/core
        ${RTFRAME_ROOT}/src/include
        ${RTFRAME_ROOT}/src/lib
        ${RTFRAME_ROOT}/src/middleware/uorb
    )

    zephyr_library_compile_options(-Wno-enum-compare -Wno-cast-align -Wno-sign-compare)

    zephyr_library_link_libraries(
        rtframe_hrt
        mavlink_headers
    )
endif()
