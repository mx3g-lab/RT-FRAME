/**
 * @file module.h
 * Minimal PX4 module abstraction for RTFrame/Zephyr.
 * Provides px4_main_t typedef and px4_task_spawn_cmd wrapper.
 */

#pragma once

#include <zephyr/kernel.h>
#include <stdio.h>

/* --- px4_main_t: standard PX4 task entry signature --- */
typedef int (*px4_main_t)(int argc, char *argv[]);
typedef int px4_task_t;  /* PX4 task handle — int on Zephyr */

/* Priority levels (numerical, higher = higher priority in PX4; mapped to Zephyr) */
#define SCHED_DEFAULT           -1
#define SCHED_PRIORITY_DEFAULT  -1

/* Stack size adjustment (PX4 concept — NOP on Zephyr) */
#define PX4_STACK_ADJUSTED(n) (n)

/* --- px4_task_spawn_cmd: spawn a thread via pthread (CONFIG_POSIX_API required) --- */
#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

struct px4_task_args {
	px4_main_t entry;
	int argc;
	char *argv[];
};

static inline void *_px4_task_trampoline(void *arg)
{
	struct px4_task_args *a = (struct px4_task_args *)arg;
	a->entry(a->argc, a->argv);
	return nullptr;
}

static inline int px4_task_spawn_cmd(const char *name, int sched, int prio,
				     int stack_size, px4_main_t entry,
				     char *const *argv)
{
	(void)sched;
	(void)prio;
	(void)stack_size;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, stack_size > 0 ? stack_size : PTHREAD_STACK_MIN);

	/* count argc from argv */
	int argc = 0;
	if (argv) {
		while (argv[argc]) { argc++; }
	}

	/* allocate args on heap (freed by trampoline — simplified; caller owns argv) */
	struct px4_task_args *a = (struct px4_task_args *)malloc(
		sizeof(struct px4_task_args) + (argc + 1) * sizeof(char *));
	if (!a) { return -1; }
	a->entry = entry;
	a->argc = argc;
	for (int i = 0; i < argc; i++) { a->argv[i] = argv[i]; }
	a->argv[argc] = nullptr;

	pthread_t tid;
	int ret = pthread_create(&tid, &attr, _px4_task_trampoline, a);
	pthread_attr_destroy(&attr);

	if (ret == 0) {
		pthread_setname_np(tid, name);
	}
	return ret;
}

#ifdef __cplusplus
}
#endif
