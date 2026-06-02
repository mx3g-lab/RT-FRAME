#pragma once

#include <zephyr/kernel.h>

/**
 * dm_sync — dataman 初始化同步信号。
 * param_loader 完成后 give，dataman 启动前 wait，
 * 确保 dataman 的 Flash 操作在 param 就绪后才开始。
 */

extern struct k_sem g_dm_ready_sem;

static inline void dm_sync_give(void)
{
	k_sem_give(&g_dm_ready_sem);
}

/**
 * @param timeout  K_FOREVER / K_MSEC(n) 等
 * @return 0 = 就绪，-EAGAIN = 超时
 */
static inline int dm_sync_wait(k_timeout_t timeout)
{
	return k_sem_take(&g_dm_ready_sem, timeout);
}
