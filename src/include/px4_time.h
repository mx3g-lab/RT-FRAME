/**
 * @file px4_time.h
 * PX4 clock compatibility — thin wrappers around POSIX clock_gettime/settime.
 */

#pragma once

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int px4_clock_gettime(clockid_t clk, struct timespec *ts)
{
	return clock_gettime(clk, ts);
}

static inline int px4_clock_settime(clockid_t clk, const struct timespec *ts)
{
	return clock_settime(clk, ts);
}

#ifdef __cplusplus
}
#endif
