# MAVLink 模块 Zephyr 适配计划

> 创建日期：2026-06-02 | 最后更新：2026-06-02 | 状态：规划中

## 概述

`modules/mavlink/` 是从 PX4 工程拷贝的 MAVLink 2.0 协议模块，包含 92 个 stream 类和多个协议处理器。`src/include/` 提供了丰富的适配层 stubs，`src/lib/` 下各模块适配状态参差不齐——**3 个纯数学库（geo、conversion、airspeed）已完成适配，无需修改；5 个模块（dataman_client、version、timesync、tunes、adsb）仍为原始 PX4 代码，需要适配**。

---

## 一、src/lib/ 下各模块实际适配状态

### 已完成（纯数学，无 OS 依赖）

| 模块 | 路径 | 状态 | 说明 |
|------|------|------|------|
| geo | `lib/geo/` | **完成** | 纯大地测量数学，MapProjection、Haversine、bearing 等，无 OS 依赖 |
| conversion | `lib/conversion/` | **完成** | 纯坐标系旋转矩阵数学（matrix），无 OS 依赖 |
| airspeed | `lib/airspeed/` | **完成** | 纯空气动力学计算，无 OS 依赖 |
| perf | `lib/perf/` | **完成** | perf 计数器，已适配 Zephyr |
| hrt | `lib/hrt/` | **完成** | HRT 高精度定时器，已适配 cycle counter |
| crc | `lib/crc/` | **完成** | CRC 计算，无 OS 依赖 |
| ringbuffer | `lib/ringbuffer/` | **完成** | SPSC 环形缓冲区，已适配 Zephyr |
| log | `lib/log/` | **完成** | 日志系统，已适配 Zephyr `printk` |
| heatshrink | `lib/heatshrink/` | **完成** | 压缩库，已适配 |
| tinybson | `lib/tinybson/` | **完成** | BSON 编解码，已适配 |
| mavlink_log | `lib/mavlink_log/` | **完成** | MAVLink 日志后端，已适配 |

### 待适配（PX4 原始代码）

| 模块 | 路径 | 状态 | 主要 OS 依赖 |
|------|------|------|-------------|
| dataman_client | `lib/dataman_client/` | **RAW** | `px4_poll()`（NuttX），uORB |
| version | `lib/version/` | **RAW** | `__PX4_NUTTX/__PX4_LINUX` 宏，board 回调 |
| timesync | `lib/timesync/` | **RAW** | `hrt_absolute_time()`，uORB |
| tunes | `lib/tunes/` | **RAW** | `circuit_breaker_enabled()`（NuttX），uORB |
| adsb | `lib/adsb/` | **RAW** | `board_get_px4_guid()`（board 回调），uORB，`parameters.yaml` |

---

## 二、src/include/ 适配层 stubs 清单

`src/include/` 已提供的 stubs 可直接使用：

### 2.1 日志与错误

| 文件 | stubs |
|------|-------|
| `log.h` → `lib/log/px4_log.h` | `PX4_INFO/WARN/ERR/DEBUG/PANIC`，`px4_log_raw()` |
| `err.h` | `err()`，`warn()`，`EXIT()` |

### 2.2 参数系统

| 文件 | stubs |
|------|-------|
| `param/param.h` | `DEFINE_PARAMETERS()`，`ParamFloat<Int|Bool>`，`<px4_parameters.hpp>`（生成） |
| `param/param_macros.h` | `APPLY0`~`APPLY56` 编译时参数列表展开宏 |
| `module_params.h` | `ModuleParams` 基类（parent/child 级联通知） |

### 2.3 原子操作与并发

| 文件 | stubs |
|------|-------|
| `px4_atomic.h` | `px4::atomic<T>`，`atomic_load_acquire/store_release/relaxed`，`atomic_fetch_add/sub` |
| `structs/LockGuard.hpp` | `LockGuard`（RAII `k_mutex`） |
| `structs/BlockingList.hpp` | `BlockingList<T>`（`k_mutex` 保护的 IntrusiveSortedList） |
| `structs/BlockingQueue.hpp` | `BlockingQueue<T,N>`（`k_sem` MPMC 队列） |

### 2.4 容器与数据结构

| 文件 | stubs |
|------|-------|
| `structs/List.hpp` | `List<T>`，`ListNode<T>` |
| `structs/IntrusiveSortedList.hpp` | `IntrusiveSortedList<T>` |
| `structs/IntrusiveQueue.hpp` | `IntrusiveQueue<T>` |
| `structs/Array.hpp` | `px4::Array<T,N>`（溢出追踪） |
| `structs/Bitset.hpp` | `px4::Bitset<N>` |
| `structs/AtomicBitset.hpp` | `px4::AtomicBitset<N>`（`atomic<uint32_t>`） |
| `singleton.hpp` | `px4::Singleton<T>`（CRTP） |

### 2.5 核心常量与宏

| 文件 | stubs |
|------|-------|
| `defines.h` | `PX4_OK/ERROR`，`M_PI*`，`M_DEG_TO_RAD*`，`PX4_ISFINITE` |
| `visibility.h` | `__EXPORT`，`__BEGIN_DECLS/__END_DECLS` |
| `macros.h` | `arraySize`，`UNUSED`，`CCASSERT`，`QUICKACCESSCODE`，`PX4_FAST_CODE` |

---

## 三、Phase 0：lib/ 下 5 个待适配模块

### 3.1 lib/version/ 适配

**目标**：替换 PX4 git 版本生成管线为静态 stub。

```cmake
# lib/version/CMakeLists.txt
if(CONFIG_RTFRAME_PARAM)
    zephyr_library_named(rtframe_version)
    zephyr_library_sources(version.c)
    zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

    # 替换 git 生成：直接生成静态版本头
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/version_stub.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/build_git_version.h"
    )
endif()
```

`version_stub.h.in`:
```c
#define PX4_GIT_TAG_STR          "v1.0.0"
#define PX4_GIT_VERSION_STR      "rtframe-1.0"
#define PX4_GIT_BRANCH_NAME      "main"
#define PX4_GIT_VERSION_BINARY   0x01000000ULL
#define NUTTX_GIT_TAG_STR        "NuttX-11.0.0"
#define NUTTX_GIT_VERSION_BINARY 0x0b000000ULL
#define BUILD_URI                "localhost"
```

**version.c 修改**：添加 Zephyr 分支：
```cpp
const char *px4_os_name(void) {
    return "Zephyr";  // 新增 Zephyr 分支
}
const char *px4_os_version_string(void) {
    return KERNEL_VERSION_STRING;  // Zephyr kernel version
}
uint32_t px4_os_version(void) {
    return KERNEL_VERSION_NUMBER;
}
```

### 3.2 lib/timesync/ 适配

**目标**：适配 HRT timer 和 uORB。

```cmake
# lib/timesync/CMakeLists.txt
if(CONFIG_RTFRAME_UORB)
    zephyr_library_named(rtframe_timesync)
    zephyr_library_sources(Timesync.cpp Timesync.hpp)
    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src/include
        ${RTFRAME_ROOT}/src/lib
        ${RTFRAME_ROOT}/src/middleware/uorb
    )
    zephyr_library_link_libraries(rtframe_uorb rtframe_hrt rtframe_perf rtframe_log)
endif()
```

Timesync 源码已有 `hrt_absolute_time()` 和 `uORB::PublicationMulti<timesync_status_s>`，这两个在 rtframe 中已适配（hrt + uORB），**无需修改源码**。

### 3.3 lib/tunes/ 适配

**目标**：stub `circuit_breaker_enabled()`，适配 uORB。

```cmake
# lib/tunes/CMakeLists.txt
if(CONFIG_RTFRAME_TUNES)
    zephyr_library_named(rtframe_tunes)
    zephyr_library_sources(tunes.cpp default_tunes.cpp)
    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src/include
        ${RTFRAME_ROOT}/src/lib
        ${RTFRAME_ROOT}/src/middleware/uorb
    )
    zephyr_library_link_libraries(rtframe_uorb rtframe_hrt rtframe_log)
endif()
```

`tunes.cpp` 中的 `circuit_breaker_enabled()` 需要 stub：
```cpp
// tunes_adaptation.h
static inline bool circuit_breaker_enabled(const char *, int) { return false; }
```

### 3.4 lib/adsb/ 适配

**目标**：stub `board_get_px4_guid()`，适配 uORB 和 parameters。

```cmake
# lib/adsb/CMakeLists.txt
if(CONFIG_RTFRAME_ADSB)
    zephyr_library_named(rtframe_adsb)
    zephyr_library_sources(AdsbConflict.cpp)
    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src/include
        ${RTFRAME_ROOT}/src/lib
        ${RTFRAME_ROOT}/src/middleware/uorb
    )
    zephyr_library_link_libraries(rtframe_uorb rtframe_hrt rtframe_log rtframe_perf rtframe_geo)
endif()
```

`parameters.yaml` 需转为 Kconfig。

### 3.5 lib/dataman_client/ 适配

**目标**：适配 `px4_poll()` → Zephyr `k_poll()`，适配 uORB。

```cmake
# lib/dataman_client/CMakeLists.txt
if(CONFIG_RTFRAME_DATAMAN_CLIENT)
    zephyr_library_named(rtframe_dataman_client)
    zephyr_library_sources(DatamanClient.cpp)
    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src/include
        ${RTFRAME_ROOT}/src/lib
        ${RTFRAME_ROOT}/src/middleware/uorb
    )
    zephyr_library_link_libraries(rtframe_uorb rtframe_hrt rtframe_perf rtframe_log)
endif()
```

`px4_poll()` → Zephyr `k_poll()` 适配层：
```cpp
// dataman_client.cpp 顶部添加
#include <zephyr/kernel.h>

// 替换 px4_poll 为 Zephyr k_poll
static inline int px4_poll(px4_pollfd_struct_t *fds, nfds_t nfds, int timeout) {
    struct k_poll_event events[16];
    // ... convert fds to k_poll_event ...
    return k_poll(events, nfds, K_MSEC(timeout));
}
```

---

## 四、Phase 1：mavlink CMake/Kconfig 构建系统

### 4.1 创建 `modules/mavlink/CMakeLists.txt`

```cmake
if(CONFIG_RTFRAME_MAVLINK)
    if(NOT DEFINED RTFRAME_ROOT)
        get_filename_component(RTFRAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
    endif()

    # ── MAVLink C 库生成管线（与原版相同）────────────
    set(MAVLINK_GIT_DIR "${CMAKE_CURRENT_LIST_DIR}/mavlink")
    set(MAVLINK_LIBRARY_DIR "${CMAKE_BINARY_DIR}/mavlink")

    # uAvionix dialect
    add_custom_command(OUTPUT ...)
    # 主 dialect
    add_custom_command(OUTPUT ...)

    # mavlink_c INTERFACE library
    add_library(mavlink_c INTERFACE)
    target_include_directories(mavlink_c INTERFACE ${MAVLINK_LIBRARY_DIR})

    # ── Zephyr library ───────────────────────────────
    zephyr_library_named(rtframe_mavlink)

    # 核心源文件
    zephyr_library_sources(
        mavlink.c
        mavlink_command_sender.cpp
        mavlink_events.cpp
        mavlink_ftp.cpp
        mavlink_log_handler.cpp
        mavlink_main.cpp
        mavlink_messages.cpp
        mavlink_mission.cpp
        mavlink_parameters.cpp
        mavlink_rate_limiter.cpp
        mavlink_receiver.cpp
        mavlink_shell.cpp
        mavlink_simple_analyzer.cpp
        mavlink_stream.cpp
        mavlink_timesync.cpp
        mavlink_ulog.cpp
        mavlink_sign_control.cpp
        MavlinkStatustextHandler.cpp
        open_drone_id_translations.cpp
        tune_publisher.cpp
    )

    # streams (glob)
    file(GLOB STREAMS_SRCS "streams/*.hpp")
    zephyr_library_sources(${STREAMS_SRCS})

    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src/include
        ${RTFRAME_ROOT}/src/lib
        ${RTFRAME_ROOT}/src/middleware/uorb
        ${MAVLINK_LIBRARY_DIR}
        ${MAVLINK_LIBRARY_DIR}/${CONFIG_RTFRAME_MAVLINK_DIALECT}
        ${MAVLINK_LIBRARY_DIR}/${MAVLINK_DIALECT_UAVIONIX}
    )

    zephyr_library_compile_options(
        -Wno-enum-compare
        -Wno-address-of-packed-member
        -Wno-cast-align
    )

    zephyr_library_link_libraries(
        rtframe_uorb
        rtframe_parameters
        rtframe_hrt
        rtframe_perf
        rtframe_mavlink_log
        rtframe_crc
        rtframe_ringbuffer
        rtframe_timesync
        rtframe_tunes
        rtframe_geo
        rtframe_conversion
        rtframe_dataman_client
        rtframe_adsb
        mavlink_c
    )

    add_dependencies(rtframe_mavlink
        mavlink_c_generate mavlink_c_generate_uavionix
        rtframe_timesync rtframe_tunes rtframe_dataman_client rtframe_adsb
    )
endif()
```

### 4.2 创建 `modules/mavlink/Kconfig`

```kconfig
config RTFRAME_MAVLINK
    bool "MAVLink communication module"
    default n
    depends on RTFRAME_PARAM && RTFRAME_UORB

if RTFRAME_MAVLINK

config RTFRAME_MAVLINK_DIALECT
    string "MAVLink dialect"
    default "common"

config RTFRAME_MAVLINK_NETWORKING
    bool "Enable UDP networking"
    default y
    depends on NET

endif
```

### 4.3 更新 `src/lib/CMakeLists.txt`

```cmake
add_subdirectory_ifdef(CONFIG_RTFRAME_VERSION         version)
add_subdirectory_ifdef(CONFIG_RTFRAME_TIMESYNC        timesync)
add_subdirectory_ifdef(CONFIG_RTFRAME_TUNES           tunes)
add_subdirectory_ifdef(CONFIG_RTFRAME_ADSB           adsb)
add_subdirectory_ifdef(CONFIG_RTFRAME_DATAMAN_CLIENT  dataman_client)
add_subdirectory_ifdef(CONFIG_RTFRAME_MAVLINK         modules/mavlink)
```

### 4.4 更新 `prj.conf`

```kconfig
CONFIG_RTFRAME_MAVLINK=y
CONFIG_RTFRAME_MAVLINK_NETWORKING=y
CONFIG_NET_SOCKETS=y
CONFIG_RTFRAME_VERSION=y
CONFIG_RTFRAME_TIMESYNC=y
CONFIG_RTFRAME_TUNES=y
CONFIG_RTFRAME_DATAMAN_CLIENT=y
CONFIG_RTFRAME_ADSB=y
```

---

## 五、Phase 2：vwork 线程集成

### 5.1 多实例架构

每个 MAVLink 实例（MAV_0、MAV_1、MAV_2）是**独立的线程**，每个实例持有：
- 独立的 `vwork::Thread`（独立槽位）
- 独立的 `_channel`（`MAVLINK_COMM_0/1/2`）
- 共享的全局 `mavlink_module_instances[]` 表（仅作索引查找）

```
mavlink_module_instances[MAVLINK_COMM_0] → Mavlink instance 0 (thread: "vwork:mavlink0")
mavlink_module_instances[MAVLINK_COMM_1] → Mavlink instance 1 (thread: "vwork:mavlink1")
mavlink_module_instances[MAVLINK_COMM_2] → Mavlink instance 2 (thread: "vwork:mavlink2")
```

### 5.2 vwork_config.h 槽位注册

每个实例独立槽位，通过 VWORK_CONFIG_TABLE 宏展开生成独立栈和 config：

```cpp
// core/vwork_config.h  VWORK_CONFIG_TABLE 中添加
X(mavlink0, "vwork:mavlink0", 8192, PRIORITY_DEFAULT, Model::THREAD)
X(mavlink1, "vwork:mavlink1", 8192, PRIORITY_DEFAULT, Model::THREAD)
X(mavlink2, "vwork:mavlink2", 8192, PRIORITY_DEFAULT, Model::THREAD)
```

`VWORK_X_DECLARE` 宏为每个 X 展开：
- `K_THREAD_STACK_DECLARE(mavlink0_stack, 8192)` — 静态栈内存
- `static constexpr config_t mavlink0 { ... }` — config 对象

三个独立栈（`mavlink0_stack`、`mavlink1_stack`、`mavlink2_stack`）和三个独立 config。

### 5.3 Mavlink 类继承 vwork::Thread

```cpp
// mavlink_main.h
#include <core/vwork.h>

class Mavlink : public ModuleParams, public vwork::Thread
{
    // ... 原有 public/private 成员 ...

    // vwork::Thread 接口
    void init() override;
    void callback() override;  // 主循环一次迭代

private:
    int task_main(int argc, char *argv[]);  // 保留，callback() 中调用
};
```

关键改动：
- 删除 `px4_task_spawner()`、`task_spawn()`、`task_delete()` 相关代码
- 删除 `_task_id`（用 `vwork::Thread` 的 `_thread` 替代）
- 删除全局 `pthread_mutex_t mavlink_module_mutex`（用实例 `_send_mutex` 等替代）
- 析构函数中的 `request_stop()` + 等待逻辑 → `vwork::Thread` 自动管理
- `set_instance_id()` 中创建线程：`new vwork::Thread(cfg)` + `start()`

### 5.4 多实例启动流程适配

原 PX4：
```cpp
// 多实例通过参数配置启动（mavlink_params.yaml 中 serial_config）
// mavlink start -d /dev/ttyS1 -b 57600  // 实例 0
// mavlink start -d /dev/ttyS2 -b 57600  // 实例 1
// mavlink start -u 14556                 // 实例 2 (UDP)
```

适配后：
```cpp
// set_instance_id() 中根据 _instance_id 选择 vwork config
const vwork::config_t *cfg = [&]{
    switch(_instance_id) {
    case 0: return &vwork::configs::mavlink0;
    case 1: return &vwork::configs::mavlink1;
    case 2: return &vwork::configs::mavlink2;
    default: return nullptr;
    }
}();
```

---

## 六、Phase 3：UART / Socket 适配

### 6.1 串口适配

```cpp
// mavlink_main.cpp
#include <zephyr/drivers/uart.h>

const struct device *uart_dev = device_get_binding(device_name);
// uart_configure(), uart_fifo_read(), uart_tx() ...
```

### 6.2 Socket 适配

Zephyr 提供 POSIX-compatible socket API（`CONFIG_NET_SOCKETS=y`）：
```cpp
#include <zephyr/net/socket.h>
// zsock_socket(), zsock_bind(), zsock_setsockopt() ...
```

---

## 七、Phase 4：streams/*.hpp 适配（92 文件）

统一 include 路径，添加 adaptation 层头文件。

---

## 八、Phase 5：protocol handlers 审查

逐个审查 `mavlink_mission`、`mavlink_ftp`、`mavlink_ulog`、`mavlink_shell` 等文件，确认 uORB、dataman_client、timesync 等依赖已适配。

---

## 九、Phase 6：参数系统集成

### 9.1 mavlink_params.yaml 加入参数生成管线

```cmake
# lib/parameters/CMakeLists.txt
set(PARAM_YAML_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/events_params.yaml
    ${RTFRAME_ROOT}/src/modules/logger/module.yaml
    ${RTFRAME_ROOT}/src/modules/mavlink/mavlink_params.yaml
)
```

### 9.2 module.yaml 多实例参数

`MAV_${i}_MODE`、`MAV_${i}_RATE` 等模板参数需展开为具体参数名。

---

## 十、Phase 7：编译测试

```
cmake -B build -H . -DBOARD=nxp/vmu_rt1170/cm7
ninja -C build
```

---

## 实施顺序

```
Phase 0: 适配 lib/ 下 5 个模块
  ├─ version     (CMake + version.c Zephyr 分支)
  ├─ timesync   (CMake，源码已兼容)
  ├─ tunes      (CMake + circuit_breaker stub)
  ├─ adsb       (CMake + board_get_px4_guid stub + parameters.yaml)
  └─ dataman_client (CMake + px4_poll → k_poll 适配)

Phase 1: mavlink CMake/Kconfig
  ├─ modules/mavlink/CMakeLists.txt
  ├─ modules/mavlink/Kconfig
  ├─ src/lib/CMakeLists.txt (添加子目录)
  └─ prj.conf (CONFIG_RTFRAME_MAVLINK=y)

Phase 2: vwork 线程集成
  ├─ vwork_config.h (mavlink0/mavlink1/mavlink2 三个独立槽位)
  └─ mavlink_main.h/cpp (继承 vwork::Thread + 多实例启动)

Phase 3: UART / Socket 适配

Phase 4: streams/*.hpp 统一 include

Phase 5: protocol handlers 审查

Phase 6: 参数系统集成

Phase 7: 编译测试
```

---

## 关键文件清单

| 优先级 | 文件 | 操作 |
|-------|------|------|
| **P0** | `lib/version/CMakeLists.txt` | 重写 Zephyr + stub 版本头 |
| **P0** | `lib/version/version.c` | 添加 Zephyr 分支 |
| **P0** | `lib/version/version_stub.h.in` | 新建 |
| **P0** | `lib/timesync/CMakeLists.txt` | 重写 Zephyr |
| **P0** | `lib/tunes/CMakeLists.txt` | 重写 Zephyr |
| **P0** | `lib/adsb/CMakeLists.txt` | 重写 Zephyr |
| **P0** | `lib/dataman_client/CMakeLists.txt` | 重写 Zephyr |
| **P0** | `lib/dataman_client/DatamanClient.cpp` | px4_poll → k_poll 适配 |
| **P0** | `modules/mavlink/CMakeLists.txt` | 新建 |
| **P0** | `modules/mavlink/Kconfig` | 新建 |
| **P0** | `src/lib/CMakeLists.txt` | 添加子目录 |
| **P1** | `core/vwork_config.h` | 添加 mavlink0/mavlink1/mavlink2 三个槽位 |
| **P1** | `modules/mavlink/mavlink_main.h` | 继承 vwork::Thread |
| **P1** | `modules/mavlink/mavlink_main.cpp` | UART/Socket/vwork 适配 |
| **P2** | `modules/mavlink/streams/*.hpp` | 统一 include (92 文件) |
| **P2** | `modules/mavlink/mavlink_receiver.h/cpp` | include 路径适配 |
| **P2** | `modules/mavlink/mavlink_messages.cpp` | include 路径适配 |
| **P3** | `lib/parameters/CMakeLists.txt` | 添加 mavlink_params.yaml |
| **P3** | `prj.conf` | CONFIG_RTFRAME_MAVLINK=y 等 |
| **P4** | 编译测试，修复错误 | — |

---

## 改版记录

| 日期 | 版本 | 修改内容 |
|------|------|---------|
| 2026-06-02 | v1 | 初始规划 |
| 2026-06-02 | v2 | 更新：确认 geo/conversion/airspeed 已完成；version/timesync/tunes/adsb/dataman_client 待适配；加入 include/ stubs 清单 |
