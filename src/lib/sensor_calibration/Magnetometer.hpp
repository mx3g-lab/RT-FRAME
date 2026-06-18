/**
 * @file Magnetometer.hpp
 * Minimal stub for PX4 lib/sensor_calibration/Magnetometer.hpp on RTFrame/Zephyr.
 * Magnetometer calibration not yet implemented — used for MAG_CAL_REPORT stream.
 */
#pragma once

#include <stdint.h>

namespace calibration
{

class Magnetometer
{
public:
	Magnetometer() = default;
	explicit Magnetometer(uint32_t device_id) : _device_id(device_id) {}

	uint32_t device_id() const { return _device_id; }

	int32_t cal_id() const { return 0; }

private:
	uint32_t _device_id{0};
};

} /* namespace calibration */
