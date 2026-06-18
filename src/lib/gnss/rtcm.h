/**
 * @file rtcm.h
 * Minimal stub for PX4 lib/gnss/rtcm.h on RTFrame/Zephyr.
 *
 * GPS RTCM message assembly — not yet implemented on Zephyr.
 * Provides empty GpsRtcmMessageAssembler to allow mavlink module compilation.
 */

#pragma once

#include <stdint.h>

namespace gnss
{

class GpsRtcmMessageAssembler
{
public:
	GpsRtcmMessageAssembler() = default;

	/**
	 * Add a fragment to the assembler.
	 * @return -1 on error, 0 if no message assembled, >0 length of assembled message
	 */
	int add_fragment(uint8_t flags, const uint8_t *data, int len)
	{
		(void)flags; (void)data; (void)len;
		return 0;  /* no message assembled yet (not implemented) */
	}

	/** Get assembled message data */
	const uint8_t *message_data() const { return nullptr; }

	/** Get assembled message length */
	int message_length() const { return 0; }
};

} /* namespace gnss */
