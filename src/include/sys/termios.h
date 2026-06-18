/**
 * @file sys/termios.h
 * Minimal termios stub for MAVLink module compilation on Zephyr.
 *
 * The actual UART I/O will be rewritten to use the Zephyr UART API
 * (device_get_binding + uart_poll_in/out) instead of POSIX termios.
 * This header provides just enough to compile during the migration.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t tcflag_t;
typedef uint32_t speed_t;
typedef uint8_t  cc_t;

#define NCCS 32

struct termios {
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	cc_t     c_cc[NCCS];
	speed_t  c_ispeed;
	speed_t  c_ospeed;
};

/* Baud rates */
#define B0       0
#define B57600   57600
#define B115200  115200
#define B921600  921600

/* c_cflag bits */
#define CS8      0000060
#define CSTOPB   0000100
#define CREAD    0000200
#define PARENB   0000400
#define CLOCAL   0004000
#define CRTSCTS  020000000000

/* c_iflag bits */
#define IXON     0002000
#define IXOFF    0010000

/* c_oflag bits */
#define OPOST    0000001

/* c_lflag bits */
#define ECHO     0000010
#define ICANON   0000002
#define ISIG     0000001

/* tcsetattr actions */
#define TCSANOW  0
#define TCSADRAIN 1

/* open() flags (should be in fcntl.h, but provided for convenience) */
#ifndef O_RDWR
#define O_RDWR   2
#endif
#ifndef O_NOCTTY
#define O_NOCTTY 0
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK 04000
#endif

/* Stub functions — will be replaced by Zephyr UART API */
static inline int tcgetattr(int fd, struct termios *t) { (void)fd; (void)t; return 0; }
static inline int tcsetattr(int fd, int act, const struct termios *t) { (void)fd; (void)act; (void)t; return 0; }
static inline int cfsetispeed(struct termios *t, speed_t s) { (void)t; (void)s; return 0; }
static inline int cfsetospeed(struct termios *t, speed_t s) { (void)t; (void)s; return 0; }
static inline void cfmakeraw(struct termios *t) { (void)t; }

#ifdef __cplusplus
}
#endif
