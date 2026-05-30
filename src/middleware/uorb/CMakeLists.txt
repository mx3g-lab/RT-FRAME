if(CONFIG_RTFRAME_UORB)
    if(NOT DEFINED RTFRAME_ROOT)
        get_filename_component(RTFRAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
    endif()

    # 消息生成：引入仓库根的 msg/，产出 uorb_msgs INTERFACE 库
    add_subdirectory("${RTFRAME_ROOT}/msg" "${CMAKE_BINARY_DIR}/msg")

    # 生成的消息头对所有模块可见（#include <uORB/topics/xxx.h>）
    zephyr_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/msg
    )

    # 让生成的头在编译前就绪
    add_dependencies(app uorb_headers_gen)
endif()
