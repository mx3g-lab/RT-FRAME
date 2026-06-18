/**
 * @file defines.h
 *
 * uORB 基础宏定义（替代 px4_platform_common/defines.h）
 */

#pragma once

/* ─── 返回值 ─────────────────────────────────────────────────────────────── */
#define PX4_OK       0
#define PX4_ERROR   (-1)
#define OK           0
#define ERROR       (-1)

/* ─── 符号导出（Zephyr 不需要） ──────────────────────────────────────────── */
#define __EXPORT

/* ─── PX4_ISFINITE ───────────────────────────────────────────────────────── */
#ifdef __cplusplus
static inline constexpr bool PX4_ISFINITE(float x) { return __builtin_isfinite(x); }
static inline constexpr bool PX4_ISFINITE(double x) { return __builtin_isfinite(x); }
#else
#define PX4_ISFINITE(x) __builtin_isfinite(x)
#endif

/* ─── 文件系统路径 ────────────────────────────────────────────────────────── */
#ifndef PX4_ROOTFSDIR
#define PX4_ROOTFSDIR "/SD:"
#endif
#ifndef PX4_STORAGEDIR
#define PX4_STORAGEDIR "/SD:"
#endif

/* ─── 数学常量 ───────────────────────────────────────────────────────────── */
#define M_PI_F          3.14159265f
#define M_TWOPI_F       6.28318531f
#define M_PI_2_F        1.57079632f
#define M_DEG_TO_RAD_F  0.0174532925f
#define M_RAD_TO_DEG_F  57.2957795f
#define M_SQRT2_F       1.41421356f

#ifndef M_PI
#define M_PI            3.141592653589793238462643383279502884
#endif

#define M_DEG_TO_RAD    0.017453292519943295
#define M_RAD_TO_DEG    57.295779513082323
