/**
 * @file dataman.h
 * Minimal stub for PX4 dataman/dataman.h on RTFrame/Zephyr.
 * Dataman persistent storage — not yet implemented on Zephyr.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int dm_item_t;

#define DM_KEY_WAYPOINTS_OFFBOARD_0  0
#define DM_KEY_WAYPOINTS_OFFBOARD_1  1
#define DM_KEY_SAFE_POINTS_0         2
#define DM_KEY_FENCE_POINTS_0        3
#define DM_KEY_WAYPOINTS_OFFBOARD_0_MAX 4
#define DM_KEY_FENCE_POINTS_MAX         5
#define DM_KEY_SAFE_POINTS_MAX          6
#define DM_KEY_NUM_KEYS                 7

#ifdef __cplusplus
}
#endif
