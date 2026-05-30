# =============================================================================
# msg/CMakeLists.txt — uORB 消息生成（rtframe 精简版）
#
# 对每个 .msg 调用 PX4 的 px_generate_uorb_topic_files.py 生成 C/C++ 头文件。
# 去掉了 PX4 原版的 JSON / CDR / Zenoh / ucdr / 压缩字段生成，只保留 --headers。
#
# 产出：
#   ${CMAKE_CURRENT_BINARY_DIR}/uORB/topics/<snake>.h   每个消息一个头
#   ${CMAKE_CURRENT_BINARY_DIR}/uORB/topics/uORBTopics.hpp
#
# 用法：依赖 INTERFACE 库 uorb_msgs，它把生成目录加进 include path，
#       使用方即可 #include <uORB/topics/sensor_combined.h>
#
# 依赖：venv 里的 empy==3.3.4 + pyros-genmsg（由 tools/setup_env.sh 安装）
# =============================================================================

if(NOT DEFINED RTFRAME_ROOT)
    get_filename_component(RTFRAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
endif()

set(MSG_GEN_SCRIPT  "${RTFRAME_ROOT}/tools/msg/px_generate_uorb_topic_files.py")
set(MSG_TEMPLATE_DIR "${RTFRAME_ROOT}/tools/msg/templates/uorb")
set(MSG_OUT_DIR     "${CMAKE_CURRENT_BINARY_DIR}/uORB/topics")

# ── 收集 .msg 文件（主目录 + versioned/）──────────────────────────────────────
file(GLOB msg_files CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.msg"
    "${CMAKE_CURRENT_SOURCE_DIR}/versioned/*.msg"
)
list(SORT msg_files)

# ── 由 .msg 文件名推导生成的 .h 路径（PascalCase → snake_case）────────────────
set(uorb_headers)
foreach(msg_file ${msg_files})
    get_filename_component(msg_name ${msg_file} NAME_WE)
    # PascalCase → snake_case：在大写字母前插下划线，再转小写
    string(REGEX REPLACE "(.)([A-Z][a-z]+)" "\\1_\\2" snake "${msg_name}")
    string(REGEX REPLACE "([a-z0-9])([A-Z])" "\\1_\\2" snake "${snake}")
    string(TOLOWER "${snake}" snake)
    list(APPEND uorb_headers "${MSG_OUT_DIR}/${snake}.h")
endforeach()

# ── 生成命令：一次性把所有 .msg 传给脚本 ──────────────────────────────────────
add_custom_command(
    OUTPUT ${uorb_headers} "${MSG_OUT_DIR}/uORBTopics.hpp"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MSG_OUT_DIR}"
    COMMAND ${PYTHON_EXECUTABLE} "${MSG_GEN_SCRIPT}"
        --headers
        -f ${msg_files}
        -i "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/versioned"
        -o "${MSG_OUT_DIR}"
        -e "${MSG_TEMPLATE_DIR}"
    DEPENDS
        ${msg_files}
        "${MSG_GEN_SCRIPT}"
        "${RTFRAME_ROOT}/tools/msg/px_generate_uorb_topic_helper.py"
        "${MSG_TEMPLATE_DIR}/msg.h.em"
        "${MSG_TEMPLATE_DIR}/uORBTopics.hpp.em"
    COMMENT "Generating uORB topic headers (${CMAKE_CURRENT_BINARY_DIR}/uORB/topics)"
    VERBATIM
)

add_custom_target(uorb_headers_gen DEPENDS ${uorb_headers})

# ── INTERFACE 库：把生成目录加进 include path ─────────────────────────────────
# 使用方 #include <uORB/topics/xxx.h>，所以 include 根是 <binary>（含 uORB/ 子目录）
add_library(uorb_msgs INTERFACE)
target_include_directories(uorb_msgs INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")
add_dependencies(uorb_msgs uorb_headers_gen)
