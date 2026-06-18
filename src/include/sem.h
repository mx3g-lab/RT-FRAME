/**
 * @file sem.h
 * PX4 semaphore compatibility layer over Zephyr k_sem.
 */

#pragma once

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct k_sem px4_sem_t;

static inline int px4_sem_init(px4_sem_t *sem, int pshared, unsigned int value)
{
	(void)pshared;
	k_sem_init(sem, value, value);
	return 0;
}

static inline int px4_sem_destroy(px4_sem_t *sem)
{
	/* k_sem has no destroy — no-op */
	(void)sem;
	return 0;
}

static inline int px4_sem_wait(px4_sem_t *sem)
{
	return k_sem_take(sem, K_FOREVER);
}

static inline int px4_sem_post(px4_sem_t *sem)
{
	k_sem_give(sem);
	return 0;
}

#ifdef __cplusplus
}
#endif
