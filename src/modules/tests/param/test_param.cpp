#include "test_param.h"

#include <zephyr/logging/log.h>
#include <parameters/param.h>

LOG_MODULE_REGISTER(test_param, LOG_LEVEL_INF);

#define PARAM_FILE "/SD:/params.bson"

static bool _phase_done = false;

void TestParam::init()
{
	LOG_INF("param test init");
	param_set_default_file(PARAM_FILE);
	LOG_INF("default file: %s", param_get_default_file());
}

void TestParam::callback()
{
	if (_phase_done) {
		return;
	}
	_phase_done = true;

	/* ---- Phase 1: set values and save to SD ---- */
	LOG_INF("=== Phase 1: set + save ===");

	param_t p_stat = param_find("EV_TSK_STAT_DIS");
	param_t p_rc   = param_find("EV_TSK_RC_LOSS");

	if (p_stat == PARAM_INVALID || p_rc == PARAM_INVALID) {
		LOG_ERR("param_find failed — aborting");
		return;
	}

	int32_t v_stat = 42;
	int32_t v_rc   = 99;
	param_set(p_stat, &v_stat);
	param_set(p_rc,   &v_rc);

	int32_t check = 0;
	param_get(p_stat, &check);
	LOG_INF("EV_TSK_STAT_DIS set to %d (read back %d)", v_stat, check);
	param_get(p_rc, &check);
	LOG_INF("EV_TSK_RC_LOSS  set to %d (read back %d)", v_rc, check);

	int save_ret = param_save_default(true);
	LOG_INF("param_save_default -> %d (%s)", save_ret, save_ret == 0 ? "OK" : "FAIL");

	param_print_status();

	/* ---- Phase 2: reset in-memory values to defaults ---- */
	LOG_INF("=== Phase 2: reset all ===");
	param_reset_all();

	int32_t after_reset = -1;
	param_get(p_stat, &after_reset);
	LOG_INF("EV_TSK_STAT_DIS after reset = %d (expect 0)", after_reset);
	param_get(p_rc, &after_reset);
	LOG_INF("EV_TSK_RC_LOSS  after reset = %d (expect 0)", after_reset);

	/* ---- Phase 3: load from SD and verify ---- */
	LOG_INF("=== Phase 3: load from SD ===");
	int load_ret = param_load_default();
	LOG_INF("param_load_default -> %d (%s)", load_ret, load_ret == 0 ? "OK" : "FAIL");

	int32_t after_load = -1;
	param_get(p_stat, &after_load);
	LOG_INF("EV_TSK_STAT_DIS after load = %d (expect %d) %s",
		after_load, v_stat, after_load == v_stat ? "[PASS]" : "[FAIL]");
	param_get(p_rc, &after_load);
	LOG_INF("EV_TSK_RC_LOSS  after load = %d (expect %d) %s",
		after_load, v_rc, after_load == v_rc ? "[PASS]" : "[FAIL]");

	param_print_status();
}

// RTFRAME_TASK_REGISTER(TestParam, vwork::configs::sensor, INIT_LEVEL_APP, 1_hz);
