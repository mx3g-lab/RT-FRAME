#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include "rtframe_core.h"
#include "task_register.h"
#include "sd_sync.h"

/* 全局信号量：param_loader 完成初始化后 give，logger/dataman 等 take */
K_SEM_DEFINE(g_sd_ready_sem, 0, 1);
K_SEM_DEFINE(g_dm_ready_sem, 0, 1);

LOG_MODULE_REGISTER(rtframe_core, LOG_LEVEL_INF);

STRUCT_SECTION_START_EXTERN(task_entry);
STRUCT_SECTION_END_EXTERN(task_entry);

static const char *model_str(vwork::Model m)
{
	switch (m) {
	case vwork::Model::WORKQUEUE: return "workq";
	case vwork::Model::THREAD:    return "thread";
	}
	return "?";
}

/* 汇总打印所有已注册任务（启动时 + 可被 shell 复用）。 */
void task_list_print()
{
	LOG_INF("");
	LOG_INF("================================Task List================================");
	LOG_INF("%-18s %-9s %-16s %8s %4s %5s %3s",
	        "TASK", "MODEL", "CONFIG", "PERIOD", "PRIO", "STACK", "LVL");
	STRUCT_SECTION_FOREACH(task_entry, e) {
		LOG_INF("%-18s %-9s %-16s %8u %4d %5u %3u",
		        e->name, model_str(e->model), e->cfg_name,
		        e->period_us, e->priority, e->stacksize, e->init_level);
	}
	LOG_INF("=========================================================================");
	LOG_INF("");
}

void task_init()
{
	/* 按 init_level 从低到高启动；同级内按 section 顺序 */
	for (int level = INIT_LEVEL_DRIVER; level <= INIT_LEVEL_APP; ++level) {
		STRUCT_SECTION_FOREACH(task_entry, e) {
			if (e->init_level == level) {
				bool ok = e->start_fn();
				if (!ok) {
					LOG_ERR("start FAIL: %s", e->name);
				}
			}
		}
	}
}

void rtframe_core_init()
{
	LOG_INF("rtframe init");
	task_list_print();
	task_init();
	LOG_INF("tasks started");
}

/* ---------- shell: rtframe task ---------- */
static int cmd_task(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	task_list_print();
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(rtframe_subcmds,
	SHELL_CMD(task, NULL, "List registered tasks", cmd_task),
	SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(rtframe, &rtframe_subcmds, "rtframe commands", NULL);
