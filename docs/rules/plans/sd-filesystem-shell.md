# 计划：SD 卡文件系统 + Shell 集成

## 目标

在 rtframe CM7 上启用 SD 卡文件系统，并通过 Zephyr Shell 提供文件操作命令（ls、cd、read、write、mount 等）。

## 方案

利用 Zephyr 内置机制，仅修改 Kconfig 配置，无需编写驱动代码：
- DTS 已就绪（usdhc1 节点含 pwr-gpios、cd-gpios、sdmmc 子节点）
- `CONFIG_FILE_SYSTEM_SHELL=y` 直接获得完整 fs 命令集
- 驱动链路：IMX_USDHC → SDMMC_SUBSYS → FatFS → fs API → shell

## 取舍

- 禁用 `FS_FATFS_MOUNT_MKFS`，防止意外格式化
- 使用 DTS fstab + `automount` 实现启动自动挂载，无需手动 `fs mount`
- `disk-access` 属性必须在 fstab 节点中声明（驱动 BUILD_ASSERT 要求）
- fatfs module 通过 `ZEPHYR_EXTRA_MODULES` 注入 `hardware/fatfs`，同时设置 `ZEPHYR_FATFS_MODULE_DIR` 指向本地 submodule

## 实施步骤

- [x] `targets/cm7/prj.conf` 追加 shell + SD + FS 配置
- [x] `cmake/zephyr.cmake` 注入 fatfs module（`ZEPHYR_EXTRA_MODULES` + `ZEPHYR_FATFS_MODULE_DIR`）
- [x] DTS 添加 fstab 节点，`automount` + `disk-access`
- [x] 新建 `docs/zephyr/filesystem.md` 文档
- [x] 更新 `docs/zephyr/README.md` 索引

## 验收标准

1. `make cm7` 编译零错误 ✅
2. 烧录后串口出现 `uart:~$` 提示符 ✅
3. 启动自动挂载 `/SD:` ✅
4. `fs ls /SD:` 列出文件 ✅（DATAMAN、LOG/、PARAM*.BSO 等）
5. `fs statvfs /SD:` 显示容量信息 ✅

## 创建日期

2026-05-31
