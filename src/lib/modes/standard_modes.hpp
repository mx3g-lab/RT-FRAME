/* PX4 standard flight modes stub for RTFrame/Zephyr */
#pragma once

enum class Mode : uint8_t {
	MANUAL = 0,
	ALTITUDE,
	POSITION,
	AUTO_MISSION,
	AUTO_LOITER,
	AUTO_RTL,
	ACRO,
	OFFBOARD,
	STABILIZED,
	RATTITUDE,
	AUTO_TAKEOFF,
	AUTO_LAND,
	AUTO_FOLLOW_TARGET,
	AUTO_PRECLAND,
	ORBIT,
	UNKNOWN = 255,
};

static inline Mode mode_from_nav_state(uint8_t nav_state) {
	return (nav_state < 15) ? (Mode)nav_state : Mode::UNKNOWN;
}

static inline const char *mode_str(Mode) { return "UNKNOWN"; }
