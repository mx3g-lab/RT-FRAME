#include "param_loader.h"

#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <parameters/param.h>
#include "task_register.h"
#include "sd_sync.h"
#include "dm_sync.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

LOG_MODULE_REGISTER(param_loader, LOG_LEVEL_INF);

#define PARAM_FILE "/SD:/params.bson"
#define PARAM_DIR "/SD:"
#define TEST_FILE "/SD:/param_test.txt"

/* -------------------------------------------------------
 * 等待 SD 卡挂载就绪（最多 wait_ms 毫秒）
 * 返回 true = 挂载成功
 * ------------------------------------------------------- */
static bool wait_for_sd(int wait_ms)
{
	struct fs_statvfs stat;
	int elapsed = 0;
	while (elapsed < wait_ms)
	{
		if (fs_statvfs(PARAM_DIR, &stat) == 0)
		{
			LOG_INF("[SD] mounted OK (waited %d ms)", elapsed);
			LOG_INF("[SD] block size=%lu, total blocks=%lu, free blocks=%lu",
					stat.f_bsize, stat.f_blocks, stat.f_bfree);
			return true;
		}
		k_sleep(K_MSEC(10));
		elapsed += 10;
	}
	LOG_ERR("[SD] mount timeout after %d ms", wait_ms);
	return false;
}

/* -------------------------------------------------------
 * 测试 SD 卡读写：写一个小文件再读回校验
 * ------------------------------------------------------- */
static void test_sd_rw()
{
	LOG_INF("[TEST] SD read/write test...");
	const char *wr_data = "rtframe_param_test_OK\n";

	/* 写 */
	struct fs_file_t f;
	fs_file_t_init(&f);
	int rc = fs_open(&f, TEST_FILE, FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC);
	if (rc < 0)
	{
		LOG_ERR("[TEST] fs_open for write failed: %d (errno=%d)", rc, errno);
		return;
	}
	ssize_t written = fs_write(&f, wr_data, strlen(wr_data));
	fs_sync(&f);
	fs_close(&f);
	if (written < 0)
	{
		LOG_ERR("[TEST] fs_write failed: %d", (int)written);
		return;
	}
	LOG_INF("[TEST] wrote %d bytes to %s", (int)written, TEST_FILE);

	/* 读回 */
	char buf[64] = {0};
	fs_file_t_init(&f);
	rc = fs_open(&f, TEST_FILE, FS_O_READ);
	if (rc < 0)
	{
		LOG_ERR("[TEST] fs_open for read failed: %d", rc);
		return;
	}
	ssize_t rd = fs_read(&f, buf, sizeof(buf) - 1);
	fs_close(&f);

	if (rd < 0)
	{
		LOG_ERR("[TEST] fs_read failed: %d", (int)rd);
		return;
	}
	buf[rd] = '\0';

	if (strcmp(buf, wr_data) == 0)
	{
		LOG_INF("[TEST] SD R/W PASS: \"%s\"", buf);
	}
	else
	{
		LOG_ERR("[TEST] SD R/W MISMATCH! got: \"%s\"", buf);
	}

	/* 删除测试文件 */
	fs_unlink(TEST_FILE);
}

/* -------------------------------------------------------
 * 测试 POSIX open/write/read/close
 * ------------------------------------------------------- */
static void test_posix_rw()
{
	LOG_INF("[TEST] POSIX open/write/read test...");
	const char *path = "/SD:/posix_test.txt";
	const char *msg = "POSIX_OK\n";

	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0)
	{
		LOG_ERR("[TEST] POSIX open(W) failed: fd=%d errno=%d", fd, errno);
		return;
	}
	int n = write(fd, msg, strlen(msg));
	fsync(fd);
	close(fd);
	LOG_INF("[TEST] POSIX write: %d bytes", n);

	char buf[32] = {0};
	fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		LOG_ERR("[TEST] POSIX open(R) failed: fd=%d errno=%d", fd, errno);
		return;
	}
	int rd = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	unlink(path);

	if (rd > 0 && strcmp(buf, msg) == 0)
	{
		LOG_INF("[TEST] POSIX R/W PASS");
	}
	else
	{
		LOG_ERR("[TEST] POSIX R/W FAIL: rd=%d buf=%s", rd, buf);
	}
}

/* -------------------------------------------------------
 * 检测参数文件是否存在及其大小
 * ------------------------------------------------------- */
static void check_param_file()
{
	struct fs_dirent dirent;
	int rc = fs_stat(PARAM_FILE, &dirent);
	if (rc == 0)
	{
		LOG_INF("[PARAM] file exists: %s, size=%zu bytes", PARAM_FILE, dirent.size);
	}
	else
	{
		LOG_WRN("[PARAM] file not found: %s (rc=%d)", PARAM_FILE, rc);
	}
}

/* -------------------------------------------------------
 * 列出 SD 根目录内容
 * ------------------------------------------------------- */
static void list_sd_root()
{
	LOG_INF("[SD] listing %s ...", PARAM_DIR);
	struct fs_dir_t dir;
	struct fs_dirent entry;
	fs_dir_t_init(&dir);
	int rc = fs_opendir(&dir, PARAM_DIR);
	if (rc < 0)
	{
		LOG_ERR("[SD] opendir failed: %d", rc);
		return;
	}
	int count = 0;
	while (fs_readdir(&dir, &entry) == 0 && entry.name[0] != '\0')
	{
		if (entry.type == FS_DIR_ENTRY_DIR)
		{
			LOG_INF("[SD]   DIR  %s/", entry.name);
		}
		else
		{
			LOG_INF("[SD]   FILE %s (%zu bytes)", entry.name, entry.size);
		}
		count++;
	}
	fs_closedir(&dir);
	LOG_INF("[SD] total %d entries", count);
}

/* -------------------------------------------------------
 * 加载参数，加载失败则保存一次默认值
 * ------------------------------------------------------- */
static void load_or_init_params()
{
	param_init();
	param_set_default_file(PARAM_FILE);

	LOG_INF("[PARAM] loading from %s ...", PARAM_FILE);
	int ret = param_load_default();
	if (ret == 0)
	{
		LOG_INF("[PARAM] loaded OK from %s", PARAM_FILE);
	}
	else if (ret == 1)
	{
		LOG_WRN("[PARAM] file not found, saving defaults to %s", PARAM_FILE);
		int rc = param_save_default(true);
		if (rc == 0)
		{
			LOG_INF("[PARAM] defaults saved OK");
		}
		else
		{
			LOG_ERR("[PARAM] save defaults failed: %d (errno=%d)", rc, errno);
		}
	}
	else
	{
		LOG_ERR("[PARAM] load failed: %d, using defaults", ret);
	}
}

void ParamLoader::run()
{
	LOG_INF("[PARAM] ParamLoader starting");

	/* 1. 等 SD 卡挂载 */
	if (!wait_for_sd(5000))
	{
		LOG_ERR("[PARAM] SD not available, param system disabled");
		LOG_INF("[PARAM] To fix: check SD card, ensure it's FAT32 formatted, and properly connected");
		/* 2. 列出根目录 */
		list_sd_root();

		/* 3. SD R/W 功能测试 */
		test_sd_rw();

		/* 4. POSIX R/W 功能测试 */
		test_posix_rw();

		/* 5. 检查 param 文件状态 */
		check_param_file();

		return;
	}

	/* 6. 加载参数（首次运行则保存默认值） */
	load_or_init_params();

	LOG_INF("[PARAM] ParamLoader done");

	/* 通知所有等待 SD/param 就绪的模块（如 logger、dataman）可以开始了 */
	sd_sync_give();
	k_sleep(K_SECONDS(1));
	dm_sync_give();
}

RTFRAME_TASK_REGISTER(ParamLoader, vwork::configs::param_auto_start, INIT_LEVEL_DRIVER, 0);
