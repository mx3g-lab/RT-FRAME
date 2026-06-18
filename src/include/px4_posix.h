/**
 * @file px4_posix.h
 * PX4 POSIX compatibility stubs for RTFrame/Zephyr.
 */

#pragma once

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- px4_prctl emulation --- */
#define PR_SET_NAME 0

static inline int px4_getpid(void) { return (int)(uintptr_t)k_current_get(); }

static inline int px4_prctl(int option, const char *name, int pid)
{
	(void)option;
	(void)pid;
	/* Set Zephyr thread name if called with PR_SET_NAME */
	k_thread_name_set(k_current_get(), name);
	return 0;
}

#ifdef __cplusplus
}
#endif
