if(CONFIG_RTFRAME_CORE)
    zephyr_library_named(rtframe_core)
    zephyr_library_sources(
        rtframe_core.cpp
        vwork.cpp
        vwork_config.cpp
        work_queue_manager.cpp
    )
    zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    zephyr_library_link_libraries(rtframe_hrt)

    # core 的公共头（vwork.h / vwork_config.h / task_register.h）对所有模块可见
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

    # 任务自注册表的 iterable section 定义
    zephyr_linker_sources(DATA_SECTIONS task_entry.ld)
endif()
