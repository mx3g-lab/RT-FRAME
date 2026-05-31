zephyr_library_named(rtframe_parameters)

# --- 参数代码生成：yaml -> C宏 -> XML -> hpp ---
if(NOT DEFINED RTFRAME_ROOT)
    get_filename_component(RTFRAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()

set(PARAM_YAML_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/events_params.yaml
)

set(generated_params_dir  ${CMAKE_CURRENT_BINARY_DIR}/generated_params)
set(generated_params_c    ${generated_params_dir}/all_params.c)
set(generated_params_xml  ${generated_params_dir}/parameters.xml)
set(generated_params_hpp  ${generated_params_dir}/px4_parameters.hpp)

zephyr_library_sources_ifdef(CONFIG_RTFRAME_PARAM
    parameters.cpp
    # param_translation.cpp  # TODO: 后期加回，依赖 Device.hpp / drv_sensor.h
    param_translation_stub.cpp
    atomic_transaction.cpp
    autosave.cpp
)

zephyr_library_sources_ifdef(CONFIG_PARAM_PRIMARY
    parameters_primary.cpp
)

zephyr_library_sources_ifdef(CONFIG_PARAM_REMOTE
    parameters_remote.cpp
)

# 生成目录必须在源码目录之前，确保生成的 px4_parameters.hpp 优先被找到
zephyr_library_include_directories(
    ${generated_params_dir}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# 让全局代码可以 #include <parameters/param.h>
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

zephyr_library_link_libraries(
    rtframe_tinybson
    rtframe_perf
    rtframe_hrt
    rtframe_crc
)

# Step 1: yaml -> C宏
add_custom_command(
    OUTPUT  ${generated_params_c}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${generated_params_dir}
    COMMAND ${Python3_EXECUTABLE}
        ${RTFRAME_ROOT}/tools/module_config/generate_params.py
        --config-files ${PARAM_YAML_FILES}
        --params-file  ${generated_params_c}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/generated
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${generated_params_c}
        ${CMAKE_CURRENT_SOURCE_DIR}/generated/all_params.c
    DEPENDS ${PARAM_YAML_FILES}
            ${RTFRAME_ROOT}/tools/module_config/generate_params.py
    COMMENT "Generating param C macros from YAML"
)

# Step 2: C宏 -> XML
add_custom_command(
    OUTPUT  ${generated_params_xml}
    COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/px_process_params.py
        -s ${generated_params_dir}
        --xml ${generated_params_xml}
    DEPENDS ${generated_params_c}
            ${CMAKE_CURRENT_SOURCE_DIR}/px_process_params.py
    COMMENT "Generating parameters.xml"
)

# Step 3: XML -> hpp
set(generated_params_hpp_copy ${CMAKE_CURRENT_SOURCE_DIR}/generated/px4_parameters.hpp)

add_custom_command(
    OUTPUT  ${generated_params_hpp}
    COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/px_generate_params.py
        --xml  ${generated_params_xml}
        --dest ${generated_params_dir}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/generated
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${generated_params_hpp}
        ${generated_params_hpp_copy}
    DEPENDS ${generated_params_xml}
            ${CMAKE_CURRENT_SOURCE_DIR}/px_generate_params.py
            ${CMAKE_CURRENT_SOURCE_DIR}/templates/px4_parameters.hpp.jinja
    COMMENT "Generating px4_parameters.hpp"
)

add_custom_target(generate_px4_parameters DEPENDS ${generated_params_hpp})
zephyr_library_add_dependencies(generate_px4_parameters)
