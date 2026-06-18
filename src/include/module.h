/**
 * @file module.h
 * Minimal PX4 module abstraction for RTFrame/Zephyr.
 * Provides px4_main_t typedef and px4_task_spawn_cmd wrapper.
 */

#pragma once

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>

/* Default module name (can be overridden before including this header) */
#ifndef MODULE_NAME
#define MODULE_NAME "rtframe_module"
#endif

/* Module usage printing (PX4 CLI stubs for Zephyr) */
#define PRINT_MODULE_USAGE_NAME(name, desc)              (void)(name); (void)(desc)
#define PRINT_MODULE_USAGE_NAME_SIMPLE(name, desc)       (void)(name); (void)(desc)
#define PRINT_MODULE_USAGE_PARAM_INT(key, dflt, ...)        (void)(key); (void)(dflt)
#define PRINT_MODULE_USAGE_PARAM_STRING(key, dflt, ...)     (void)(key); (void)(dflt)
#define PRINT_MODULE_USAGE_PARAM_FLAG(key, desc, opt)    (void)(key); (void)(desc); (void)(opt)
#define PRINT_MODULE_USAGE_PARAM_FLOAT(key, dflt, min, max, desc, opt) (void)(key); (void)(dflt); (void)(min); (void)(max); (void)(desc); (void)(opt)
#define PRINT_MODULE_USAGE_ARG(key, desc, optional)      (void)(key); (void)(desc); (void)(optional)
#define PRINT_MODULE_USAGE_COMMAND_DESCR(key, desc)      (void)(key); (void)(desc)
#define PRINT_MODULE_DESCRIPTION(desc)                   (void)(desc)

#ifndef px4_usleep
#define px4_usleep(us) k_usleep(us)
#endif
/* Additional PX4 task stubs */
#define px4_task_delete(t) (void)(t)
static inline int px4_get_parameter_value(const char *name, int &value) { (void)name; value = 0; return 0; }
static inline int px4_getopt(int argc, char *const argv[], const char *opts, int *idx, const char **arg) { (void)argc;(void)argv;(void)opts;(void)idx;(void)arg; return -1; }

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

#include <pthread_compat.h>

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
