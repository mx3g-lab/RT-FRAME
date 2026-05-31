#include "test_sdcard.h"

#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/posix/unistd.h>
#include <string.h>
#include <errno.h>

LOG_MODULE_REGISTER(test_sdcard, LOG_LEVEL_INF);

#define TEST_FILE  "/SD:/sdcard_test.bson"
#define TEST_DATA  "Hello SD card! 0123456789\n"
#define TEST_LEN   (sizeof(TEST_DATA) - 1)

static bool _done = false;

void TestSdcard::init()
{
	LOG_INF("sdcard test init");
}

void TestSdcard::callback()
{
	if (_done) {
		return;
	}
	_done = true;

	/* ---- write ---- */
	LOG_INF("=== write %s ===", TEST_FILE);

	int fd = ::open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		LOG_ERR("open for write failed: errno=%d", errno);
		return;
	}

	ssize_t written = ::write(fd, TEST_DATA, TEST_LEN);
	::close(fd);

	if (written != (ssize_t)TEST_LEN) {
		LOG_ERR("write failed: wrote %zd of %zu bytes, errno=%d", written, TEST_LEN, errno);
		return;
	}
	LOG_INF("wrote %zd bytes OK", written);

	/* ---- read back ---- */
	LOG_INF("=== read back ===");

	fd = ::open(TEST_FILE, O_RDONLY);
	if (fd < 0) {
		LOG_ERR("open for read failed: errno=%d", errno);
		return;
	}

	char buf[64] = {};
	ssize_t nread = ::read(fd, buf, sizeof(buf) - 1);
	::close(fd);

	if (nread < 0) {
		LOG_ERR("read failed: errno=%d", errno);
		return;
	}

	buf[nread] = '\0';
	LOG_INF("read %zd bytes: \"%s\"", nread, buf);

	/* ---- verify ---- */
	if (nread == (ssize_t)TEST_LEN && memcmp(buf, TEST_DATA, TEST_LEN) == 0) {
		LOG_INF("=== PASS ===");
	} else {
		LOG_ERR("=== FAIL: data mismatch ===");
	}
}

// RTFRAME_TASK_REGISTER(TestSdcard, vwork::configs::sensor, INIT_LEVEL_APP, 1_hz);
