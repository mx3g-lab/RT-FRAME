/****************************************************************************
 *
 *   Copyright (c) 2016-2022 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file mavlink_shell.cpp
 * A shell to be used via MAVLink
 *
 * @author Beat Küng <beat-kueng@gmx.net>
 */

#include "mavlink_shell.h"
#include <defines.h>
#include <log.h>
#include <px4_posix.h>

#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

#if 0 // __PX4_NUTTX (Zephyr)
#include <nshlib/nshlib.h>
#endif

#if 0 // __PX4_POSIX (Zephyr)
#include "../../../platforms/posix/src/px4/common/px4_daemon/pxh.h"
#endif

#if 0 // __PX4_CYGWIN (Zephyr)
#include <asm/socket.h>
#endif

MavlinkShell::~MavlinkShell()
{
	/* Shell not supported on Zephyr — no-op */
}

int MavlinkShell::start()
{
	/* Shell not supported on Zephyr — requires POSIX daemon (pxh) */
	return -1;
}

#if 0 // Zephyr: shell daemon not available

	int p1[2], p2[2];

	/* Create the shell task and redirect its stdin & stdout. If we used pthread, we would redirect
	 * stdin/out of the calling process as well, so we need px4_task_spawn_cmd. However NuttX only
	 * keeps (duplicates) the first 3 fd's when creating a new task, all others are not inherited.
	 * This means we need to temporarily change the first 3 fd's of the current task (or at least
	 * the first 2 if stdout=stderr).
	 */

	if (pipe(p1) != 0) {
		return -errno;
	}

	if (pipe(p2) != 0) {
		close(p1[0]);
		close(p1[1]);
		return -errno;
	}

	int ret = 0;

	_from_shell_fd  = p1[0];
	_to_shell_fd = p2[1];
	_shell_fds[0]  = p2[0];
	_shell_fds[1] = p1[1];

	/*
	 * Ensure that during the temporary phase no other thread from the same task writes to
	 * stdout (as it would end up in the pipe).
	 */
#if 0 // __PX4_NUTTX (Zephyr)
	sched_lock();
#endif

#if 0 // __PX4_POSIX (Zephyr)
	int remote_in_fd = dup(_shell_fds[0]);	// Input file descriptor for the remote shell
	int remote_out_fd = dup(_shell_fds[1]); // Output file descriptor for the remote shell

	char r_in[32];
	char r_out[32];
	snprintf(r_in, sizeof(r_in), "%d", remote_in_fd);
	snprintf(r_out, sizeof(r_out), "%d", remote_out_fd);
	char *const argv[3] = {r_in, r_out, nullptr};

#else
	int fd_backups[2]; //we don't touch stderr, we will redirect it to stdout in the startup of the shell task

	for (int i = 0; i < 2; ++i) {
		fd_backups[i] = dup(i);

		if (fd_backups[i] == -1) {
			ret = -errno;
		}
	}

	dup2(_shell_fds[0], 0);
	dup2(_shell_fds[1], 1);
#endif

	if (ret == 0) {
		_task = px4_task_spawn_cmd("mavlink_shell",
					   SCHED_DEFAULT,
					   SCHED_PRIORITY_DEFAULT,
					   2048,
					   &MavlinkShell::shell_start_thread,
#if 0 // __PX4_POSIX (Zephyr)
					   argv);
#else
					   nullptr);
#endif

		if (_task < 0) {
			ret = -1;
		}
	}

#if !defined(__PX4_POSIX)

	//restore fd's
	for (int i = 0; i < 2; ++i) {
		if (dup2(fd_backups[i], i) == -1) {
			ret = -errno;
		}

		close(fd_backups[i]);
	}

#endif

	//close unused pipe fd's
	close(_shell_fds[0]);
	close(_shell_fds[1]);

#if 0 // __PX4_NUTTX (Zephyr)
	sched_unlock();
#endif

	return ret;
}

int MavlinkShell::shell_start_thread(int argc, char *argv[])
{
#if 0 // __PX4_NUTTX (Zephyr)
	dup2(1, 2); //redirect stderror to stdout

	const int ret = nsh_consolemain(0, NULL);

	if (ret) {
		PX4_ERR("Mavlink shell failed: %d%s", ret, (ret == -ENOMEM) ? " (out of memory)" : "");
		return ret;
	}

#endif

#if 0 // __PX4_POSIX (Zephyr)

	if (argc != 3) {
		PX4_ERR("Mavlink shell bug");
		return -1;
	}

	int remote_in_fd = atoi(argv[1]);
	int remote_out_fd = atoi(argv[2]);

	px4_daemon::Pxh pxh;
	pxh.run_remote_pxh(remote_in_fd, remote_out_fd);
#endif

	return 0;
}

#endif /* Zephyr: shell daemon not available */

size_t MavlinkShell::write(uint8_t *buffer, size_t len)
{
	(void)buffer;
	return len;  /* Shell not supported on Zephyr */
}

size_t MavlinkShell::read(uint8_t *buffer, size_t len)
{
	(void)buffer; (void)len;
	return 0;  /* Shell not supported on Zephyr */
}

size_t MavlinkShell::available()
{
	return 0;  /* Shell not supported on Zephyr */
}
