#include "logger_task.h"
#include "logger.h"
#include <zephyr/logging/log.h>
#include "task_register.h"
#include <parameters/param.h>
#include "sd_sync.h"

LOG_MODULE_REGISTER(logger_task, LOG_LEVEL_INF);
using namespace px4::logger;

void LoggerTask::run()
{
	/* 等待 param_loader 完成 SD 初始化（含参数加载），再操作文件系统，
	 * 避免 FatFS 层资源竞争。param_loader::run() 末尾调用 sd_sync_give()。*/
	sd_sync_wait(K_FOREVER);

	LOG_INF("logger task starting");
	/* 构造 Logger：默认参数 — 文件后端，12KB buffer，3500µs 间隔，boot_until_disarm 模式 */
	Logger *logger = new Logger(
		LogWriter::BackendFile,
		12 * 1024,
		3500,
		nullptr,                        /* poll_topic_name */
		Logger::LogMode::boot_until_disarm,
		false,                          /* log_name_timestamp */
		1.0f                            /* rate_factor */
	);

	if (!logger) {
		LOG_ERR("logger alloc failed");
		return;
	}

	logger->run();

	delete logger;
	LOG_INF("logger task exited");
}

RTFRAME_TASK_REGISTER(LoggerTask, vwork::configs::logger, INIT_LEVEL_APP, 0);
