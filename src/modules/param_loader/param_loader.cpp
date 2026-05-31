#include "param_loader.h"

#include <zephyr/logging/log.h>
#include <parameters/param.h>
#include "task_register.h"

LOG_MODULE_REGISTER(param_loader, LOG_LEVEL_INF);

#define PARAM_FILE "/SD:/params.bson"

void ParamLoader::init()
{
	param_init();
	param_set_default_file(PARAM_FILE);
	int ret = param_load_default();
	if (ret == 0) {
		LOG_INF("params loaded from %s", PARAM_FILE);
	} else {
		LOG_WRN("load failed (%d), using defaults", ret);
	}
}

/* period=0 对 Periodic 无效，用一个极大值让 callback 实际上永不触发。 */
RTFRAME_TASK_REGISTER(ParamLoader, vwork::configs::param_save, INIT_LEVEL_DRIVER, UINT32_MAX);
