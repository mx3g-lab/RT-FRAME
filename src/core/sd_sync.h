#pragma once

#include <zephyr/kernel.h>

/**
 * sd_sync — param_loader 初始化完成后 give，其他需要在 param 就绪后才
 * 操作 SD 卡的模块（logger 等）在此 take。
 *
 * 用法：
 *   param_loader 完成后：sd_sync_give();
 *   logger 启动前：    sd_sync_wait(K_FOREVER);
 */

extern struct k_sem g_sd_ready_sem;

static inline void sd_sync_give(void)
{
	k_sem_give(&g_sd_ready_sem);
}

/**
 * @param timeout  K_FOREVER / K_MSEC(n) 等
 * @return 0 = 就绪，-EAGAIN = 超时
 */
static inline int sd_sync_wait(k_timeout_t timeout)
{
	return k_sem_take(&g_sd_ready_sem, timeout);
}
