/****************************************************************************
 *
 *   Copyright (c) 2016-2021 PX4 Development Team. All rights reserved.
 *   Ported to Zephyr RTOS / rtframe by rtframe contributors.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ****************************************************************************/

/**
 * @file sd_bench.cpp
 * @brief SD Card benchmarking — Zephyr shell command
 *
 * Usage (Zephyr shell):
 *   sd_bench                          # default: 4 KB block, 100 loops, no fsync
 *   sd_bench -b 16384 -r 200 -s       # 16 KB block, 200 loops, fsync each write
 *   sd_bench -h                       # show help
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>

LOG_MODULE_REGISTER(sd_bench, LOG_LEVEL_INF);

/* ------------------------------------------------------------------ */
/*  Defaults                                                            */
/* ------------------------------------------------------------------ */
#define BENCH_DEFAULT_BLOCK_SIZE   4096U
#define BENCH_DEFAULT_LOOPS        100U
#define BENCH_DEFAULT_KEEP         false
#define BENCH_DEFAULT_FSYNC        false
#define BENCH_FILE_PATH            "/SD:/sd_bench.tmp"

/* ------------------------------------------------------------------ */
/*  Helpers                                                             */
/* ------------------------------------------------------------------ */

/** Return monotonic time in microseconds. */
static inline int64_t bench_now_us(void)
{
	return (int64_t)k_cyc_to_us_floor64(k_cycle_get_64());
}

/** Fill buffer with a repeating pattern. */
static void fill_buffer(uint8_t *buf, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		buf[i] = (uint8_t)(i & 0xFF);
	}
}

/** Verify buffer matches the pattern written by fill_buffer(). */
static bool verify_buffer(const uint8_t *buf, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		if (buf[i] != (uint8_t)(i & 0xFF)) {
			return false;
		}
	}
	return true;
}

/* ------------------------------------------------------------------ */
/*  Core benchmark                                                      */
/* ------------------------------------------------------------------ */

struct bench_params {
	uint32_t block_size; /* bytes per write */
	uint32_t num_loops;  /* number of write iterations */
	bool     keep;       /* keep file after bench */
	bool     do_fsync;   /* fsync after each write */
};

static void run_benchmark(const struct shell *sh, const struct bench_params *p)
{
	uint8_t *buf = (uint8_t *)k_malloc(p->block_size);
	if (!buf) {
		shell_error(sh, "sd_bench: malloc(%u) failed", p->block_size);
		return;
	}
	fill_buffer(buf, p->block_size);

	/* ---- Write benchmark ---- */
	shell_print(sh, "sd_bench: writing %u x %u B blocks to %s (fsync=%s)...",
	            p->num_loops, p->block_size, BENCH_FILE_PATH,
	            p->do_fsync ? "yes" : "no");

	int fd = open(BENCH_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		shell_error(sh, "sd_bench: open for write failed: %d", errno);
		k_free(buf);
		return;
	}

	int64_t t_start = bench_now_us();
	int64_t worst_write_us = 0;

	for (uint32_t i = 0; i < p->num_loops; i++) {
		int64_t t0 = bench_now_us();

		ssize_t written = write(fd, buf, p->block_size);
		if (written != (ssize_t)p->block_size) {
			shell_error(sh, "sd_bench: write failed at loop %u (ret=%zd errno=%d)",
			            i, written, errno);
			close(fd);
			k_free(buf);
			return;
		}

		if (p->do_fsync) {
			fsync(fd);
		}

		int64_t dt = bench_now_us() - t0;
		if (dt > worst_write_us) {
			worst_write_us = dt;
		}
	}

	/* Final fsync to flush everything */
	fsync(fd);
	close(fd);

	int64_t t_write_total_us = bench_now_us() - t_start;
	uint64_t total_bytes = (uint64_t)p->block_size * p->num_loops;
	float write_speed = (float)total_bytes / (float)t_write_total_us; /* MB/s */

	shell_print(sh, "sd_bench: WRITE  %.2f MB/s  (worst block: %.1f ms,  total: %.1f ms)",
	            (double)write_speed,
	            (double)worst_write_us / 1000.0,
	            (double)t_write_total_us / 1000.0);

	/* ---- Read benchmark ---- */
	shell_print(sh, "sd_bench: reading back...");

	fd = open(BENCH_FILE_PATH, O_RDONLY);
	if (fd < 0) {
		shell_error(sh, "sd_bench: open for read failed: %d", errno);
		k_free(buf);
		return;
	}

	int64_t t_read_start = bench_now_us();
	int64_t worst_read_us = 0;
	bool data_ok = true;

	for (uint32_t i = 0; i < p->num_loops; i++) {
		int64_t t0 = bench_now_us();

		ssize_t rd = read(fd, buf, p->block_size);
		if (rd != (ssize_t)p->block_size) {
			shell_error(sh, "sd_bench: read failed at loop %u (ret=%zd errno=%d)",
			            i, rd, errno);
			close(fd);
			k_free(buf);
			return;
		}

		int64_t dt = bench_now_us() - t0;
		if (dt > worst_read_us) {
			worst_read_us = dt;
		}

		if (!verify_buffer(buf, p->block_size)) {
			data_ok = false;
			shell_error(sh, "sd_bench: data mismatch at loop %u!", i);
		}
	}

	close(fd);

	int64_t t_read_total_us = bench_now_us() - t_read_start;
	float read_speed = (float)total_bytes / (float)t_read_total_us;

	shell_print(sh, "sd_bench: READ   %.2f MB/s  (worst block: %.1f ms,  total: %.1f ms)",
	            (double)read_speed,
	            (double)worst_read_us / 1000.0,
	            (double)t_read_total_us / 1000.0);

	shell_print(sh, "sd_bench: data integrity: %s", data_ok ? "OK" : "FAIL");
	shell_print(sh, "sd_bench: total size: %llu KB", total_bytes / 1024ULL);

	/* ---- Cleanup ---- */
	k_free(buf);

	if (!p->keep) {
		unlink(BENCH_FILE_PATH);
	} else {
		shell_print(sh, "sd_bench: keeping %s", BENCH_FILE_PATH);
	}
}

/* ------------------------------------------------------------------ */
/*  Shell command handler                                               */
/* ------------------------------------------------------------------ */

static void print_help(const struct shell *sh)
{
	shell_print(sh,
	    "Usage: sd_bench [options]\n"
	    "  -b <bytes>   block size (default: %u)\n"
	    "  -r <count>   number of loops (default: %u)\n"
	    "  -k           keep temp file after test\n"
	    "  -s           fsync after each write\n"
	    "  -h           show this help",
	    BENCH_DEFAULT_BLOCK_SIZE, BENCH_DEFAULT_LOOPS);
}

static int cmd_sd_bench(const struct shell *sh, size_t argc, char **argv)
{
	struct bench_params p = {
		.block_size = BENCH_DEFAULT_BLOCK_SIZE,
		.num_loops  = BENCH_DEFAULT_LOOPS,
		.keep       = BENCH_DEFAULT_KEEP,
		.do_fsync   = BENCH_DEFAULT_FSYNC,
	};

	for (size_t i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0) {
			print_help(sh);
			return 0;
		} else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
			p.block_size = (uint32_t)atoi(argv[++i]);
			if (p.block_size == 0) {
				shell_error(sh, "sd_bench: invalid block size");
				return -EINVAL;
			}
		} else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
			p.num_loops = (uint32_t)atoi(argv[++i]);
			if (p.num_loops == 0) {
				shell_error(sh, "sd_bench: invalid loop count");
				return -EINVAL;
			}
		} else if (strcmp(argv[i], "-k") == 0) {
			p.keep = true;
		} else if (strcmp(argv[i], "-s") == 0) {
			p.do_fsync = true;
		} else {
			shell_error(sh, "sd_bench: unknown option: %s", argv[i]);
			print_help(sh);
			return -EINVAL;
		}
	}

	run_benchmark(sh, &p);
	return 0;
}

SHELL_CMD_ARG_REGISTER(sd_bench, NULL,
    "SD card benchmark\n"
    "  sd_bench [-b block_size] [-r loops] [-k] [-s] [-h]",
    cmd_sd_bench, 1, 8);
