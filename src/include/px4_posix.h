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

/* Board UUID stubs */
#define PX4_CPU_UUID_BYTE_LENGTH 12
#define PX4_CPU_UUID_WORD32_UNIQUE_M  1
#define PX4_CPU_UUID_WORD32_UNIQUE_H  2
typedef uint32_t uuid_uint32_t[PX4_CPU_UUID_BYTE_LENGTH/4];
typedef uint8_t  px4_guid_t[16];
static inline void board_get_uuid32(uuid_uint32_t uid) {
	for (int i = 0; i < (PX4_CPU_UUID_BYTE_LENGTH/4); i++) uid[i] = (uint32_t)(0xdead0000 + i);
}
static inline void board_get_px4_guid(px4_guid_t guid) {
	for (int i = 0; i < 16; i++) guid[i] = (uint8_t)i;
}

#ifdef __cplusplus
}
#endif
