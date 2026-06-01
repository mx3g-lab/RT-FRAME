# 计划：PX4 logger 模块移植到 Zephyr rtframe

## Context

`src/modules/logger/` 从 PX4 直接拷贝，依赖大量 PX4 专有 API。目标：在 Zephyr 上完整编译并运行，写 ULog 到 SD 卡 `/SD:/log/`，功能不删减。

**策略：直接改源码 include 路径，不建 shim 目录。**

---

## 已有资源（直接复用）

| logger 依赖 | 项目对应路径 |
|------------|------------|
| `drivers/drv_hrt.h` | `src/lib/hrt/hrt.h` |
| `perf/perf_counter.h` | `src/lib/perf/perf_counter.h` |
| `px4_platform_common/log.h` | `src/lib/log/px4_log.h` |
| `px4_platform_common/defines.h` | `src/include/defines.h` |
| `px4_platform_common/atomic.h` | `src/include/px4_atomic.h` |
| `px4_platform_common/module.h` | `src/include/module.h`（已拷贝） |
| `px4_platform_common/module_params.h` | `src/include/module_params.h`（已拷贝） |
| `containers/Array.hpp` 等 | `src/include/containers/` |
| `mathlib/math/Limits.hpp` | `src/lib/mathlib/` |
| `parameters/param.h` | `src/include/param/param.h` |
| `DEFINE_PARAMETERS(...)` | `src/include/param/param.h`（完整实现） |
| `px4_platform_common/console_buffer.h` | `src/lib/console_buffer/console_buffer.h`（已拷贝） |
| uORB topics | `msg/`（全部存在） |
| `uORB/uORBMessageFields.hpp` | `src/middleware/uorb/uORB/` |
| POSIX 文件 I/O | Zephyr POSIX（已启用） |

---

## 需要直接处理的依赖

| 依赖 | 处理方式 |
|------|---------|
| `px4_platform_common/tasks.h` (`px4_task_spawn`) | 不用：`ModuleBase` 的 `task_spawn` 改为 rtframe `vwork::Thread` 调用 |
| `px4_platform_common/shutdown.h` | 直接在源码里注释掉相关调用或加 `#if 0` |
| `px4_platform_common/getopt.h` | 改为 `<getopt.h>`（标准 POSIX） |
| `px4_platform_common/printload.h` | 在源码里定义最小 `print_load_s` struct + 空 `print_load_buffer` |
| `px4_platform_common/events.h` | 在源码里将 `events::send(...)` 调用注释掉或空 lambda |
| `px4_platform_common/posix.h` | 改为 `<fcntl.h>`；`px4_sem_*` → `sem_*`；`PX4_O_RDONLY` → `O_RDONLY` |
| `px4_platform_common/sem.h` | `px4_sem_t` 改为 `sem_t` |
| `px4_platform_common/time.h` | 改为 `hrt/hrt.h`（已有 `hrt_abstime`） |
| `px4_platform/cpuload.h` + `nuttx/sched.h` | watchdog 已在 `#ifdef __PX4_NUTTX` 里，Zephyr 不编译；shim 只需空头 |
| `replay/definitions.hpp` | 在源码加条件：`#ifndef CONFIG_RTFRAME` ... |
| `version/version.h` | 在 logger.cpp 里直接写死版本字符串 |
| `component_information/checksums.h` | 注释掉相关代码 |
| `systemlib/mavlink_log.h` | 改为 uORB `mavlink_log` topic 的 include 路径 |
| `hrt_call_every` / `hrt_call` | 改为 `k_timer`（Zephyr 内核定时器），在 ISR 里 `k_sem_give` |
| `px4_lockstep_register_component` | 改为空调用（返回 -1） |

---

## 关键架构改动

### 1. Logger 任务模型：vwork::Thread

- 新建 `src/modules/logger/logger_task.h` / `logger_task.cpp`
- `LoggerTask : public vwork::Thread`，持有 `Logger*` 对象
- `init()` 构造 Logger，`run()` 调用 `logger->run()`（覆盖 Thread 的 run()）
- 去掉 `Logger` 继承 `ModuleBase`（直接由 LoggerTask 管理生命周期）
- `RTFRAME_TASK_REGISTER(LoggerTask, vwork::configs::logger, INIT_LEVEL_APP, 0)`

### 2. vwork_config.h 新增线程坑位

```cpp
X(logger, "vwork:logger", 8192, PRIORITY_DEFAULT, Model::THREAD)
```

### 3. timer_callback → k_timer

logger run() 用信号量阻塞，定时器每隔 `_log_interval`（默认 3500µs）触发一次。

```cpp
// 替换 hrt_call_every：
static struct k_timer _logger_timer;
static void logger_timer_fn(struct k_timer *t) {
    k_sem_give(&_timer_sem);
}
k_timer_init(&_logger_timer, logger_timer_fn, nullptr);
k_timer_start(&_logger_timer, K_USEC(_log_interval), K_USEC(_log_interval));
```

---

## 文件修改清单

### 新建
- `src/modules/logger/logger_task.h` — vwork::Thread 包装类
- `src/modules/logger/logger_task.cpp` — 构造 Logger，调用 run()
- `src/modules/logger/CMakeLists.txt` — 重写（Zephyr native API）
- `src/modules/logger/Kconfig` — `CONFIG_RTFRAME_LOGGER`

### 修改（include 路径 + NuttX API 替换）
- `logger.h` — 去掉 `ModuleBase` 继承；替换 include
- `logger.cpp` — 替换 include；去掉 `task_spawn`；k_timer 替换 hrt_call；注释 shutdown_hook / lockstep
- `log_writer_file.h/.cpp` — `defines.h`, `hrt.h`, `perf_counter.h`；`px4_sem_*` → `sem_*`
- `log_writer_mavlink.h/.cpp` — 替换 include
- `watchdog.h/.cpp` — nuttx 头已在 ifdef 内，加空 `nuttx/sched.h` shim 或直接删 include
- `util.h/.cpp` — `hrt.h`, `px4_log.h`
- `util_parse.h/.cpp` — include 路径
- `logged_topics.cpp` — include 路径
- `src/core/vwork_config.h` — 添加 logger 线程坑位
- `src/modules/CMakeLists.txt` — 添加 `add_subdirectory(logger)`

---

## CMakeLists.txt

```cmake
zephyr_library_named(rtframe_logger)

zephyr_library_sources_ifdef(CONFIG_RTFRAME_LOGGER
    logger.cpp
    logger_task.cpp
    logged_topics.cpp
    log_writer.cpp
    log_writer_file.cpp
    log_writer_mavlink.cpp
    util.cpp
    util_parse.cpp
    watchdog.cpp
)

zephyr_library_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${RTFRAME_ROOT}/src/core
    ${RTFRAME_ROOT}/src/include
    ${RTFRAME_ROOT}/src/lib
    ${RTFRAME_ROOT}/src/middleware/uorb
)

zephyr_library_compile_options(-Wno-cast-align -Wno-sign-compare)

zephyr_library_link_libraries(
    rtframe_core rtframe_hrt rtframe_perf rtframe_parameters rtframe_mathlib
)
```

---

## prj.conf 新增

```
CONFIG_RTFRAME_LOGGER=y
```

---

## 实施步骤

1. 添加 logger 线程坑位到 `vwork_config.h`
2. 重写 `CMakeLists.txt` + `Kconfig`
3. 创建 `logger_task.h` / `logger_task.cpp`
4. 修改 `logger.h` — 去掉 `ModuleBase`，替换 include
5. 修改 `logger.cpp` — 替换 include + hrt_call → k_timer + 注释 NuttX 专有调用
6. 修改 `log_writer_file.h/.cpp` — include + px4_sem_* → sem_*
7. 修改其余文件（log_writer_mavlink, util, watchdog, logged_topics）— include 路径
8. 添加进 `src/modules/CMakeLists.txt`
9. 编译修错直到 `make cm7` 零错误

---

## 验收标准

- `make cm7` 零错误
- 板子启动后在 `/SD:/log/` 下生成 `*.ulg` 文件
- `param show SDLOG_PROFILE` 可读取参数
- uORB `logger_status` topic 可被订阅到
