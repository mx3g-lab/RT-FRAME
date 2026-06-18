/**
 * @file px4_custom_mode.h
 * PX4 custom flight mode definition for MAVLink HEARTBEAT / AVAILABLE_MODES.
 *
 * Mirrors PX4 commander/px4_custom_mode.h — the union layout MUST match
 * PX4 bit-for-bit so that MAVLink mode reporting is correct.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

union px4_custom_mode {
	uint32_t data;
	struct {
		uint16_t main_mode;   /* PX4 main mode (PX4_CUSTOM_MAIN_MODE_*) */
		uint8_t  sub_mode;    /* PX4 sub mode */
		uint8_t  reserved;    /* unused */
	};
};

/* --- PX4 main mode constants --- */
#define PX4_CUSTOM_MAIN_MODE_MANUAL            1
#define PX4_CUSTOM_MAIN_MODE_ALTCTL            2
#define PX4_CUSTOM_MAIN_MODE_POSCTL            3
#define PX4_CUSTOM_MAIN_MODE_AUTO              4
#define PX4_CUSTOM_MAIN_MODE_ACRO              5
#define PX4_CUSTOM_MAIN_MODE_OFFBOARD          6
#define PX4_CUSTOM_MAIN_MODE_STABILIZED        7
#define PX4_CUSTOM_MAIN_MODE_RATTITUDE         8
#define PX4_CUSTOM_MAIN_MODE_SIMPLE            9

#ifdef __cplusplus
}
#endif

/*
 * Forward-declare — will be implemented in mavlink_messages.cpp or a
 * dedicated translation unit when vehicle_status uORB topic is available.
 * Until then, HEARTBEAT stream can use a stub returning PX4_CUSTOM_MAIN_MODE_MANUAL.
 */
union px4_custom_mode get_px4_custom_mode(uint8_t nav_state);
