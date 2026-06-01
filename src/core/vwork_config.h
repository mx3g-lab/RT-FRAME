#pragma once

#include <cstdint>
#include <zephyr/kernel.h>

namespace vwork
{

/* ============================================================
 *  优先级档位
 *
 *  Zephyr 抢占式优先级：数值越小优先级越高（0 最高）。
 *  约束：必须 < CONFIG_NUM_PREEMPT_PRIORITIES。
 *
 *  config 坑位的使用规则：
 *    - MODEL_WORKQUEUE（Periodic/Event）：多个任务可共用同一 config →
 *      共享一个 work queue 线程，串行执行。
 *    - MODEL_THREAD（Thread）：一个 config 只能被一个 Thread 独占
 *      （线程直接用 config 的栈）。
 * ============================================================ */
enum TaskPriority : int8_t {
	PRIORITY_CRITICAL     = 1,   /* 紧急/控制环 */
	PRIORITY_FAST_DRIVERS = 3,   /* 高速传感器 */
	PRIORITY_ACTUATORS    = 5,   /* 输出/执行器 */
	PRIORITY_FAST         = 8,   /* 快速业务 */
	PRIORITY_DEFAULT      = 12,  /* 普通驱动 */
	PRIORITY_SLOW_DRIVERS = 16,  /* 慢速驱动 */
	PRIORITY_FS           = 19,  /* 文件系统 */
	PRIORITY_SDCARD       = 21,  /* 存储 */
	PRIORITY_HEARTBEAT    = 23,  /* 心跳 / blink */
	PRIORITY_SHELL        = 25,  /* 交互 shell */
	PRIORITY_DEBUGGER     = 27,  /* 调试输出 */
	PRIORITY_LOW          = 29,  /* 后台低优 */
};

/* ============================================================
 *  执行模型（坑位属性）
 *
 *  WORKQUEUE —— work queue 线程，承载 Periodic（周期）/ Event（事件触发）。
 *               可多任务共享。
 *  THREAD    —— 独占线程，承载 Thread（用户自己循环，可阻塞）。独占。
 * ============================================================ */
enum class Model : uint8_t {
	WORKQUEUE,
	THREAD,
};

/*
 * config_t —— 线程资源坑位：名字 / 专属静态栈 / 优先级 / 执行模型。
 * 栈由 vwork_config.cpp 在编译期静态分配（K_THREAD_STACK_DEFINE）。
 */
struct config_t {
	const char        *name;
	k_thread_stack_t  *stack;
	uint16_t           stacksize;
	int8_t             priority;
	Model              model;
};

/* ============================================================
 *  线程坑位表（唯一真源）
 *
 *  在这里增删一行即可新增/删除一个坑位。
 *  栈大小、优先级、模型只写一次，.h 声明和 .cpp 定义都从此表展开。
 *
 *  X(变量名, 线程名, 栈字节, 优先级档位, 执行模型)
 * ============================================================ */
#define VWORK_CONFIG_TABLE(X)                                                    \
	X(debugger,  "vwork:debugger",  4096, PRIORITY_DEBUGGER,  Model::WORKQUEUE) \
	X(shell,     "vwork:shell",     4096, PRIORITY_SHELL,     Model::WORKQUEUE) \
	X(blink,     "vwork:blink",     1024, PRIORITY_HEARTBEAT, Model::WORKQUEUE) \
	X(heartbeat, "vwork:heartbeat", 2048, PRIORITY_CRITICAL,  Model::THREAD)    \
	X(sensor,    "vwork:sensor",    3072, PRIORITY_DEFAULT,   Model::WORKQUEUE) \
	X(sub,       "vwork:sub",       1536, PRIORITY_DEFAULT,   Model::THREAD)	\
	X(pub,		 "vwork:pub",       4096, PRIORITY_DEFAULT,   Model::THREAD)    \
	X(param_auto_start, "vwork:param_auto_start", 8192, PRIORITY_FS,      Model::THREAD)    \
	X(param_autosave, "vwork:param_autosave",  8192, PRIORITY_FS,      Model::WORKQUEUE) \
	X(logger,         "vwork:logger",       8192, PRIORITY_DEFAULT, Model::THREAD)    \
	X(log_writer,     "vwork:log_writer",   4096, PRIORITY_FS,      Model::THREAD)

namespace configs
{

/* 展开：声明 extern 栈 + 定义 config（constexpr，每 TU 一份无妨）。 */
#define VWORK_X_DECLARE(_var, _name, _sz, _prio, _model)  \
	K_THREAD_STACK_DECLARE(_var##_stack, _sz);        \
	static constexpr config_t _var {                  \
		.name      = _name,                       \
		.stack     = _var##_stack,                \
		.stacksize = (_sz),                       \
		.priority  = (_prio),                     \
		.model     = (_model),                    \
	};

VWORK_CONFIG_TABLE(VWORK_X_DECLARE)

#undef VWORK_X_DECLARE

} // namespace configs

} // namespace vwork
