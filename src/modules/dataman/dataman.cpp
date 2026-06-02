/****************************************************************************
 *
 *   Copyright (c) 2013-2023 PX4 Development Team. All rights reserved.
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
 * @file dataman.cpp
 * DATAMANAGER driver.
 *
 * @author Jean Cyr
 * @author Lorenz Meier
 * @author Julian Oes
 * @author Thomas Gubler
 * @author David Sidrane
 */

#include <defines.h>
#include <hrt.h>
#include <parameters/param.h>
#include <perf/perf_counter.h>
#include <stdlib.h>

#include <uORB/Publication.hpp>
#include <uORB/Subscription.hpp>
#include <uORB/topics/dataman_request.h>
#include <uORB/topics/dataman_response.h>

#include <zephyr/kernel.h>
#include <log.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>

/* Zephyr stubs for NuttX/PX4 APIs used by file backend */
#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef PX4_O_MODE_666
#define PX4_O_MODE_666 (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#endif
#ifndef F_OK
#define F_OK 0
#endif
static inline int px4_access(const char *path, int mode) {
	struct stat st;
	(void)mode;
	return stat(path, &st) == 0 ? 0 : -1;
}

#include <vwork.h>
#include "dataman.h"
#include "task_register.h"
#include "dm_sync.h"

/* ── px4_getopt stub (Zephyr 不支持命令行参数解析) ── */
static inline int px4_getopt(int argc, char *const argv[], const char *optstring,
			     int *optind, const char **optarg) {
	(void)argc; (void)argv; (void)optstring; (void)optind; (void)optarg;
	return -1;
}

/* 前向声明，供 DatamanTask::run() 调用 */
static int task_main_loop();

#ifdef CONFIG_RTFRAME_DATAMAN_PERSISTENT_STORAGE
/* Private File based Operations */
static ssize_t _file_write(dm_item_t item, unsigned index, const void *buf, size_t count);
static ssize_t _file_read(dm_item_t item, unsigned index, void *buf, size_t count);
static int  _file_clear(dm_item_t item);
static int _file_initialize(unsigned max_offset);
static void _file_shutdown();
#endif

/* Private Ram based Operations */
static ssize_t _ram_write(dm_item_t item, unsigned index, const void *buf, size_t count);
static ssize_t _ram_read(dm_item_t item, unsigned index, void *buf, size_t count);
static int  _ram_clear(dm_item_t item);
static int _ram_initialize(unsigned max_offset);
static void _ram_shutdown();

typedef struct dm_operations_t {
	ssize_t (*write)(dm_item_t item, unsigned index, const void *buf, size_t count);
	ssize_t (*read)(dm_item_t item, unsigned index, void *buf, size_t count);
	int (*clear)(dm_item_t item);
	int (*initialize)(unsigned max_offset);
	void (*shutdown)();
	int (*wait)(struct k_sem *sem);
} dm_operations_t;

/* wait wrapper: k_sem_take 需要 timeout 参数，dm_operations_t::wait 只传 sem。 */
static inline int _sem_wait_wrap(struct k_sem *sem)
{
	return k_sem_take(sem, K_FOREVER);
}

#ifdef CONFIG_RTFRAME_DATAMAN_PERSISTENT_STORAGE
static constexpr dm_operations_t dm_file_operations = {
	.write   = _file_write,
	.read    = _file_read,
	.clear   = _file_clear,
	.initialize = _file_initialize,
	.shutdown = _file_shutdown,
	.wait = _sem_wait_wrap,
};
#endif

static constexpr dm_operations_t dm_ram_operations = {
	.write   = _ram_write,
	.read    = _ram_read,
	.clear   = _ram_clear,
	.initialize = _ram_initialize,
	.shutdown = _ram_shutdown,
	.wait = _sem_wait_wrap,
};

static const dm_operations_t *g_dm_ops;

static struct {
	union {
		struct {
			int fd;
		} file;
		struct {
			uint8_t *data;
			uint8_t *data_end;
		} ram;
	};
	bool running;
	bool silence = false;
} dm_operations_data;

/* Usage statistics */
static unsigned g_func_counts[DM_NUMBER_OF_FUNCS];

#define DM_SECTOR_HDR_SIZE 4	/* data manager per item header overhead */

/* Table of the len of each item type including HDR size */
static constexpr size_t g_per_item_size_with_hdr[DM_KEY_NUM_KEYS] = {
	g_per_item_size[DM_KEY_SAFE_POINTS_0] + DM_SECTOR_HDR_SIZE,
	g_per_item_size[DM_KEY_SAFE_POINTS_1] + DM_SECTOR_HDR_SIZE,
	g_per_item_size[DM_KEY_SAFE_POINTS_STATE] + DM_SECTOR_HDR_SIZE,
	g_per_item_size[DM_KEY_FENCE_POINTS_0] + DM_SECTOR_HDR_SIZE,
	g_per_item_size[DM_KEY_FENCE_POINTS_1] + DM_SECTOR_HDR_SIZE,
	g_per_item_size[DM_KEY_FENCE_POINTS_STATE] + DM_SECTOR_HDR_SIZE,
	g_per_item_size[DM_KEY_WAYPOINTS_OFFBOARD_0] + DM_SECTOR_HDR_SIZE,
	g_per_item_size[DM_KEY_WAYPOINTS_OFFBOARD_1] + DM_SECTOR_HDR_SIZE,
	g_per_item_size[DM_KEY_MISSION_STATE] + DM_SECTOR_HDR_SIZE,
	g_per_item_size[DM_KEY_COMPAT] + DM_SECTOR_HDR_SIZE
};

/* Table of offset for index 0 of each item type */
static unsigned int g_key_offsets[DM_KEY_NUM_KEYS];

static uint8_t dataman_clients_count = 1;

static perf_counter_t _dm_read_perf{nullptr};
static perf_counter_t _dm_write_perf{nullptr};

#ifdef CONFIG_RTFRAME_DATAMAN_PERSISTENT_STORAGE
#ifndef PX4_STORAGEDIR
#define PX4_STORAGEDIR "/SD:"
#endif
/* The data manager store file handle and file name */
static const char *default_device_path = PX4_STORAGEDIR "/dataman";
static char *k_data_manager_device_path = nullptr;
#endif

static enum {
	BACKEND_NONE = 0,
	BACKEND_FILE,
	BACKEND_RAM,
	BACKEND_LAST
} backend = BACKEND_NONE;

static struct k_sem g_init_sema;
static bool g_task_should_exit;	/**< if true, dataman task should exit */

/* ============================================================
 * DatamanTask — vwork::Thread 封装，承载主循环
 * ============================================================ */
class DatamanTask : public vwork::Thread
{
public:
	DatamanTask() : vwork::Thread(vwork::configs::dataman) {}
	~DatamanTask() override = default;

	static void request_shutdown() { g_task_should_exit = true; }

private:
	void init() override;
	void run() override {
		PX4_INFO("DatamanTask running");
		dm_sync_wait(K_FOREVER);
		task_main_loop(); }
	void callback() override {} /* Thread 模式默认循环由 run() 接管，callback 留空 */
};

static DatamanTask *_task{nullptr};

static bool is_running()
{
	return dm_operations_data.running;
}

/* Calculate the offset in file of specific item */
static int
calculate_offset(dm_item_t item, unsigned index)
{

	/* Make sure the item type is valid */
	if (item >= DM_KEY_NUM_KEYS) {
		return -1;
	}

	/* Make sure the index for this item type is valid */
	if (index >= g_per_item_max_index[item]) {
		return -1;
	}

	/* Calculate and return the item index based on type and index */
	return g_key_offsets[item] + (index * g_per_item_size_with_hdr[item]);
}

/* Each data item is stored as follows
 *
 * byte 0: Length of user data item
 * byte 1: Unused (previously persistence of this data item)
 * byte 2: Unused (for future use)
 * byte 3: Unused (for future use)
 * byte DM_SECTOR_HDR_SIZE... : data item value
 *
 * The total size must not exceed g_per_item_max_index[item]
 */

/* write to the data manager RAM buffer  */
static ssize_t _ram_write(dm_item_t item, unsigned index, const void *buf, size_t count)
{
	if (item >= DM_KEY_NUM_KEYS) {
		return -1;
	}

	/* Get the offset for this item */
	int offset = calculate_offset(item, index);

	/* If item type or index out of range, return error */
	if (offset < 0) {
		return -1;
	}

	/* Make sure caller has not given us more data than we can handle */
	if (count > (g_per_item_size_with_hdr[item] - DM_SECTOR_HDR_SIZE)) {
		return -E2BIG;
	}

	uint8_t *buffer = &dm_operations_data.ram.data[offset];

	if (buffer > dm_operations_data.ram.data_end) {
		return -1;
	}

	/* Write out the data, prefixed with length */
	buffer[0] = count;
	buffer[1] = 0;
	buffer[2] = 0;
	buffer[3] = 0;

	if (count > 0) {
		memcpy(buffer + DM_SECTOR_HDR_SIZE, buf, count);
	}

	/* All is well... return the number of user data written */
	return count;
}

#ifdef CONFIG_RTFRAME_DATAMAN_PERSISTENT_STORAGE
/* write to the data manager file */
static ssize_t
_file_write(dm_item_t item, unsigned index, const void *buf, size_t count)
{
	if (item >= DM_KEY_NUM_KEYS) {
		return -1;
	}

	unsigned char buffer[g_per_item_size_with_hdr[item]];

	/* Get the offset for this item */
	const int offset = calculate_offset(item, index);

	/* If item type or index out of range, return error */
	if (offset < 0) {
		return -1;
	}

	/* Make sure caller has not given us more data than we can handle */
	if (count > (g_per_item_size_with_hdr[item] - DM_SECTOR_HDR_SIZE)) {
		return -E2BIG;
	}

	/* Write out the data, prefixed with length */
	buffer[0] = count;
	buffer[1] = 0;
	buffer[2] = 0;
	buffer[3] = 0;

	if (count > 0) {
		memcpy(buffer + DM_SECTOR_HDR_SIZE, buf, count);
	}

	count += DM_SECTOR_HDR_SIZE;

	bool write_success = false;

	for (int i = 0; i < 2; i++) {
		int ret_seek = lseek(dm_operations_data.file.fd, offset, SEEK_SET);

		if (ret_seek < 0) {
			PX4_ERR("file write lseek failed %d", errno);
			continue;
		}

		if (ret_seek != offset) {
			PX4_ERR("file write lseek failed, incorrect offset %d vs %d", ret_seek, offset);
			continue;
		}

		int ret_write = write(dm_operations_data.file.fd, buffer, count);

		if (ret_write < 0) {
			PX4_ERR("file write failed %d", errno);
			continue;
		}

		if (ret_write != (ssize_t)count) {
			PX4_ERR("file write failed, wrote %d bytes, expected %zu", ret_write, count);
			continue;

		} else {
			write_success = true;
			break;
		}
	}

	if (!write_success) {
		return -1;
	}

	/* Make sure data is written to physical media */
	fsync(dm_operations_data.file.fd);

	/* All is well... return the number of user data written */
	return count - DM_SECTOR_HDR_SIZE;
}
#endif

/* Retrieve from the data manager RAM buffer*/
static ssize_t _ram_read(dm_item_t item, unsigned index, void *buf, size_t count)
{
	if (item >= DM_KEY_NUM_KEYS) {
		return -1;
	}

	/* Get the offset for this item */
	int offset = calculate_offset(item, index);

	/* If item type or index out of range, return error */
	if (offset < 0) {
		return -1;
	}

	/* Make sure the caller hasn't asked for more data than we can handle */
	if (count > (g_per_item_size_with_hdr[item] - DM_SECTOR_HDR_SIZE)) {
		return -E2BIG;
	}

	/* Read the prefix and data */

	uint8_t *buffer = &dm_operations_data.ram.data[offset];

	if (buffer > dm_operations_data.ram.data_end) {
		return -1;
	}

	/* See if we got data */
	if (buffer[0] > 0) {
		/* We got more than requested!!! */
		if (buffer[0] > count) {
			return -1;
		}

		/* Looks good, copy it to the caller's buffer */
		memcpy(buf, buffer + DM_SECTOR_HDR_SIZE, buffer[0]);
	}

	/* Return the number of bytes of caller data read */
	return buffer[0];
}

#ifdef CONFIG_RTFRAME_DATAMAN_PERSISTENT_STORAGE
/* Retrieve from the data manager file */
static ssize_t
_file_read(dm_item_t item, unsigned index, void *buf, size_t count)
{
	if (item >= DM_KEY_NUM_KEYS) {
		return -1;
	}

	unsigned char buffer[g_per_item_size_with_hdr[item]];

	/* Get the offset for this item */
	int offset = calculate_offset(item, index);

	/* If item type or index out of range, return error */
	if (offset < 0) {
		return -1;
	}

	/* Make sure the caller hasn't asked for more data than we can handle */
	if (count > (g_per_item_size_with_hdr[item] - DM_SECTOR_HDR_SIZE)) {
		return -E2BIG;
	}

	int len = -1;
	bool read_success = false;

	for (int i = 0; i < 2; i++) {
		int ret_seek = lseek(dm_operations_data.file.fd, offset, SEEK_SET);

		if ((ret_seek < 0) && !dm_operations_data.silence) {
			PX4_ERR("file read lseek failed %d", errno);
			continue;
		}

		if ((ret_seek != offset) && !dm_operations_data.silence) {
			PX4_ERR("file read lseek failed, incorrect offset %d vs %d", ret_seek, offset);
			continue;
		}

		/* Read the prefix and data */
		len = read(dm_operations_data.file.fd, buffer, count + DM_SECTOR_HDR_SIZE);

		/* Check for read error */
		if (len >= 0) {
			read_success = true;
			break;

		} else {
			if (!dm_operations_data.silence) {
				PX4_ERR("file read failed %d", errno);
			}
		}
	}

	if (!read_success) {
		return -1;
	}

	/* A zero length entry is a empty entry */
	if (len == 0) {
		buffer[0] = 0;
	}

	/* See if we got data */
	if (buffer[0] > 0) {
		/* We got more than requested!!! */
		if (buffer[0] > count) {
			return -1;
		}

		/* Looks good, copy it to the caller's buffer */
		memcpy(buf, buffer + DM_SECTOR_HDR_SIZE, buffer[0]);

	} else {
		memset(buf, 0, count);
	}

	/* Return the number of bytes of caller data read */
	return buffer[0];
}
#endif

static int  _ram_clear(dm_item_t item)
{
	if (item >= DM_KEY_NUM_KEYS) {
		return -1;
	}

	/* Get the offset of 1st item of this type */
	int offset = calculate_offset(item, 0);

	/* Check for item type out of range */
	if (offset < 0) {
		return -1;
	}

	int result = 0;

	/* Clear all items of this type */
	for (int i = 0; (unsigned)i < g_per_item_max_index[item]; i++) {
		uint8_t *buf = &dm_operations_data.ram.data[offset];

		if (buf > dm_operations_data.ram.data_end) {
			result = -1;
			break;
		}

		buf[0] = 0;
		offset += g_per_item_size_with_hdr[item];
	}

	return result;
}

#ifdef CONFIG_RTFRAME_DATAMAN_PERSISTENT_STORAGE
static int
_file_clear(dm_item_t item)
{
	if (item >= DM_KEY_NUM_KEYS) {
		return -1;
	}

	/* Get the offset of 1st item of this type */
	int offset = calculate_offset(item, 0);

	/* Check for item type out of range */
	if (offset < 0) {
		return -1;
	}

	int result = 0;

	/* Clear all items of this type */
	for (int i = 0; (unsigned)i < g_per_item_max_index[item]; i++) {
		char buf[1];

		if (lseek(dm_operations_data.file.fd, offset, SEEK_SET) != offset) {
			result = -1;
			break;
		}

		/* Avoid SD flash wear by only doing writes where necessary */
		if (read(dm_operations_data.file.fd, buf, 1) < 1) {
			break;
		}

		/* If item has length greater than 0 it needs to be overwritten */
		if (buf[0]) {
			if (lseek(dm_operations_data.file.fd, offset, SEEK_SET) != offset) {
				result = -1;
				break;
			}

			buf[0] = 0;

			if (write(dm_operations_data.file.fd, buf, 1) != 1) {
				result = -1;
				break;
			}
		}

		offset += g_per_item_size_with_hdr[item];
	}

	/* Make sure data is actually written to physical media */
	fsync(dm_operations_data.file.fd);
	return result;
}
#endif

#ifdef CONFIG_RTFRAME_DATAMAN_PERSISTENT_STORAGE
static int
_file_initialize(unsigned max_offset)
{
	const bool file_existed = (px4_access(k_data_manager_device_path, F_OK) == 0);

	/* Open or create the data manager file */
	dm_operations_data.file.fd = open(k_data_manager_device_path, O_RDWR | O_CREAT | O_BINARY, PX4_O_MODE_666);

	if (dm_operations_data.file.fd < 0) {
		PX4_WARN("Could not open data manager file %s", k_data_manager_device_path);
		k_sem_give(&g_init_sema); /* Don't want to hang startup */
		return -1;
	}

	if ((unsigned)lseek(dm_operations_data.file.fd, max_offset, SEEK_SET) != max_offset) {
		close(dm_operations_data.file.fd);
		PX4_WARN("Could not seek data manager file %s", k_data_manager_device_path);
		k_sem_give(&g_init_sema); /* Don't want to hang startup */
		return -1;
	}

	dataman_compat_s compat_state{};

	dm_operations_data.silence = true;

	g_dm_ops->read(DM_KEY_COMPAT, 0, &compat_state, sizeof(compat_state));

	dm_operations_data.silence = false;

	if (!file_existed || (compat_state.key != DM_COMPAT_KEY)) {

		/* Write current compat info */
		compat_state.key = DM_COMPAT_KEY;
		int ret = g_dm_ops->write(DM_KEY_COMPAT, 0, &compat_state, sizeof(compat_state));

		if (ret != sizeof(compat_state)) {
			PX4_ERR("Failed writing compat: %d", ret);
		}

		for (uint32_t item = DM_KEY_SAFE_POINTS_0; item <= DM_KEY_MISSION_STATE; ++item) {
			g_dm_ops->clear((dm_item_t)item);
		}

		mission_s mission{};
		mission.timestamp = hrt_absolute_time();
		mission.mission_dataman_id = DM_KEY_WAYPOINTS_OFFBOARD_0;
		mission.count = 0;
		mission.current_seq = 0;
		mission.mission_id = 0u;
		mission.geofence_id = 0u;
		mission.safe_points_id = 0u;

		mission_stats_entry_s stats;
		stats.num_items = 0;
		stats.opaque_id = 0;

		g_dm_ops->write(DM_KEY_MISSION_STATE, 0, reinterpret_cast<uint8_t *>(&mission), sizeof(mission_s));
		g_dm_ops->write(DM_KEY_FENCE_POINTS_STATE, 0, reinterpret_cast<uint8_t *>(&stats), sizeof(mission_stats_entry_s));
		g_dm_ops->write(DM_KEY_SAFE_POINTS_STATE, 0, reinterpret_cast<uint8_t *>(&stats), sizeof(mission_stats_entry_s));
	}

	dm_operations_data.running = true;

	return 0;
}
#endif

static int
_ram_initialize(unsigned max_offset)
{
	/* In memory */
	dm_operations_data.ram.data = (uint8_t *)malloc(max_offset);

	if (dm_operations_data.ram.data == nullptr) {
		PX4_WARN("Could not allocate %u bytes of memory", max_offset);
		k_sem_give(&g_init_sema); /* Don't want to hang startup */
		return -1;
	}

	memset(dm_operations_data.ram.data, 0, max_offset);
	dm_operations_data.ram.data_end = &dm_operations_data.ram.data[max_offset - 1];
	dm_operations_data.running = true;

	return 0;
}

#ifdef CONFIG_RTFRAME_DATAMAN_PERSISTENT_STORAGE
static void
_file_shutdown()
{
	close(dm_operations_data.file.fd);
	dm_operations_data.running = false;
}
#endif

static void
_ram_shutdown()
{
	free(dm_operations_data.ram.data);
	dm_operations_data.running = false;
}

/* ============================================================
 * DatamanTask::init 实现
 * ============================================================ */
void DatamanTask::init()
{
	/* Dataman can use disk or RAM */
	switch (backend) {
#ifdef CONFIG_RTFRAME_DATAMAN_PERSISTENT_STORAGE

	case BACKEND_FILE:
		g_dm_ops = &dm_file_operations;
		break;
#endif

	case BACKEND_RAM:
		g_dm_ops = &dm_ram_operations;
		break;

	default:
		PX4_WARN("No valid backend set.");
		return;
	}

	/* Initialize global variables */
	g_key_offsets[0] = 0;

	for (int i = 0; i < ((int)DM_KEY_NUM_KEYS - 1); i++) {
		g_key_offsets[i + 1] = g_key_offsets[i] + (g_per_item_max_index[i] * g_per_item_size_with_hdr[i]);
	}

	unsigned max_offset = g_key_offsets[DM_KEY_NUM_KEYS - 1] + (g_per_item_max_index[DM_KEY_NUM_KEYS - 1] *
			      g_per_item_size_with_hdr[DM_KEY_NUM_KEYS - 1]);

	for (unsigned i = 0; i < DM_NUMBER_OF_FUNCS; i++) {
		g_func_counts[i] = 0;
	}

	g_task_should_exit = false;

	_dm_read_perf = perf_alloc(PC_ELAPSED, MODULE_NAME": read");
	_dm_write_perf = perf_alloc(PC_ELAPSED, MODULE_NAME": write");

	int ret = g_dm_ops->initialize(max_offset);

	if (ret) {
		g_task_should_exit = true;
		return;
	}

	switch (backend) {
#ifdef CONFIG_RTFRAME_DATAMAN_PERSISTENT_STORAGE

	case BACKEND_FILE:
		PX4_INFO("data manager file '%s' size is %u bytes", k_data_manager_device_path, max_offset);
		break;
#endif

	case BACKEND_RAM:
		PX4_INFO("data manager RAM size is %u bytes", max_offset);
		break;

	default:
		break;
	}

	/* Tell startup that the worker thread has completed its initialization */
	k_sem_give(&g_init_sema);
}

/* ============================================================
 * 主循环（接管原 task_main 的循环体）
 * ============================================================ */
static int task_main_loop()
{

	PX4_INFO("task_main_loop running");
	uORB::Publication<dataman_response_s> dataman_response_pub{ORB_ID(dataman_response)};
	const int dataman_request_sub = orb_subscribe(ORB_ID(dataman_request));

	if (dataman_request_sub < 0) {
		PX4_ERR("Failed to subscribe (%i)", errno);
	}

	/* Start the endless loop, waiting for then processing work requests */
	while (true) {
		/* 1s timeout — 允许定期检查 g_task_should_exit */
		(void)k_sem_take(&g_init_sema, K_MSEC(1000));

		/* 有请求到达：sem 被 uORB subsystem 释放（如果有的话）。
		 * 为兼容性，仍用 orb_check 确认。 */
		bool updated = false;
		orb_check(dataman_request_sub, &updated);

		if (updated) {
			dataman_request_s request;
			orb_copy(ORB_ID(dataman_request), dataman_request_sub, &request);

			dataman_response_s response{};
			response.client_id = request.client_id;
			response.request_type = request.request_type;
			response.item = request.item;
			response.index = request.index;
			response.status = dataman_response_s::STATUS_FAILURE_NO_DATA;

			ssize_t result;

			switch (request.request_type) {

			case DM_GET_ID:
				if (dataman_clients_count < UINT8_MAX) {
					response.client_id = dataman_clients_count++;
					memcpy(response.data, &request.timestamp, sizeof(hrt_abstime));
				} else {
					PX4_ERR("Max Dataman clients reached!");
				}
				break;

			case DM_WRITE:
				g_func_counts[DM_WRITE]++;
				perf_begin(_dm_write_perf);
				result = g_dm_ops->write(static_cast<dm_item_t>(request.item), request.index,
							 &(request.data), request.data_length);
				perf_end(_dm_write_perf);
				response.status = (result > 0) ? dataman_response_s::STATUS_SUCCESS :
								dataman_response_s::STATUS_FAILURE_WRITE_FAILED;
				break;

			case DM_READ:
				g_func_counts[DM_READ]++;
				perf_begin(_dm_read_perf);
				result = g_dm_ops->read(static_cast<dm_item_t>(request.item), request.index,
							&(response.data), request.data_length);
				perf_end(_dm_read_perf);
				response.status = (result >= 0) ? dataman_response_s::STATUS_SUCCESS :
								dataman_response_s::STATUS_FAILURE_READ_FAILED;
				break;

			case DM_CLEAR:
				g_func_counts[DM_CLEAR]++;
				result = g_dm_ops->clear(static_cast<dm_item_t>(request.item));
				response.status = (result == 0) ? dataman_response_s::STATUS_SUCCESS :
								dataman_response_s::STATUS_FAILURE_CLEAR_FAILED;
				break;

			default:
				break;
			}

			response.timestamp = hrt_absolute_time();
			dataman_response_pub.publish(response);
		}

		/* time to go???? */
		if (g_task_should_exit) {
			break;
		}
	}

	orb_unsubscribe(dataman_request_sub);
	g_dm_ops->shutdown();

	perf_free(_dm_read_perf);
	_dm_read_perf = nullptr;
	perf_free(_dm_write_perf);
	_dm_write_perf = nullptr;

	return 0;
}

static int
start()
{
	k_sem_init(&g_init_sema, 1, 0);
	_task = new DatamanTask();

	if (!_task || !_task->start(0)) {
		PX4_ERR("task start failed");
		delete _task;
		_task = nullptr;
		return -1;
	}

	k_sem_take(&g_init_sema, K_FOREVER);
	return 0;
}

static void
status()
{
	/* display usage statistics */
	PX4_INFO("Writes   %u", g_func_counts[DM_WRITE]);
	PX4_INFO("Reads    %u", g_func_counts[DM_READ]);
	PX4_INFO("Clears   %u", g_func_counts[DM_CLEAR]);

	perf_print_counter(_dm_read_perf);
	perf_print_counter(_dm_write_perf);
}

static void
stop()
{
	/* Tell the worker task to shut down */
	if (_task) {
		DatamanTask::request_shutdown();
	}
}

static void
usage()
{

}

static int backend_check()
{
	if (backend != BACKEND_NONE) {
		PX4_WARN("-f and -r are mutually exclusive");
		usage();
		return -1;
	}

	return 0;
}

int dataman_main(int argc, char *argv[])
{
	if (argc < 2) {
		usage();
		return -1;
	}

	if (!strcmp(argv[1], "start")) {

		if (is_running()) {
			PX4_WARN("dataman already running");
			return -1;
		}

		int ch;
		int dmoptind = 1;
		const char *dmoptarg = nullptr;

		while ((ch = px4_getopt(argc, argv, "f:r", &dmoptind, &dmoptarg)) != EOF) {
			switch (ch) {
			case 'f':
				if (backend_check()) {
					return -1;
				}

#ifdef CONFIG_RTFRAME_DATAMAN_PERSISTENT_STORAGE
				backend = BACKEND_FILE;
				k_data_manager_device_path = strdup(dmoptarg);
				PX4_INFO("dataman file set to: %s", k_data_manager_device_path);
#else
				backend = BACKEND_RAM;
				PX4_WARN("dataman does not support persistent storage. Falling back to RAM.");
#endif
				break;

			case 'r':
				if (backend_check()) {
					return -1;
				}

				backend = BACKEND_RAM;
				break;

			//no break
			default:
				usage();
				return -1;
			}
		}

		if (backend == BACKEND_NONE) {
#ifdef CONFIG_RTFRAME_DATAMAN_PERSISTENT_STORAGE
			backend = BACKEND_FILE;
			k_data_manager_device_path = strdup(default_device_path);
#else
			backend = BACKEND_RAM;
#endif
		}

		start();

		if (!is_running()) {
			PX4_ERR("dataman start failed");
#ifdef CONFIG_RTFRAME_DATAMAN_PERSISTENT_STORAGE
			free(k_data_manager_device_path);
			k_data_manager_device_path = nullptr;
#endif
			return -1;
		}

		return 0;
	}

	/* Worker thread should be running for all other commands */
	if (!is_running()) {
		PX4_WARN("dataman worker thread not running");
		usage();
		return -1;
	}

	if (!strcmp(argv[1], "stop")) {
		stop();
#ifdef CONFIG_RTFRAME_DATAMAN_PERSISTENT_STORAGE
		free(k_data_manager_device_path);
		k_data_manager_device_path = nullptr;
#endif

	} else if (!strcmp(argv[1], "status")) {
		status();

	} else {
		usage();
		return -1;
	}

	return 0;
}

static_assert(sizeof(dataman_request_s::data) == sizeof(dataman_response_s::data), "request and response data are not the same size");
static_assert(sizeof(dataman_response_s::data) >= MISSION_SAFE_POINT_SIZE, "mission_item_s can't fit in the response data");
static_assert(sizeof(dataman_response_s::data) >= MISSION_FENCE_POINT_SIZE, "mission_fance_point_s can't fit in the response data");
static_assert(sizeof(dataman_response_s::data) >= MISSION_ITEM_SIZE, "mission_item_s can't fit in the response data");
static_assert(sizeof(dataman_response_s::data) >= MISSION_SIZE, "mission_s can't fit in the response data");
static_assert(sizeof(dataman_response_s::data) >= DATAMAN_COMPAT_SIZE, "dataman_compat_s can't fit in the response data");
static_assert(sizeof(dataman_response_s::data) >= sizeof(hrt_abstime), "hrt_abstime can't fit in the response data");

RTFRAME_TASK_REGISTER(DatamanTask, vwork::configs::dataman, INIT_LEVEL_APP, 0);