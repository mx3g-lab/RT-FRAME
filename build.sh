#!/usr/bin/env bash
# build.sh — RTFrame 通用构建脚本
#
# Usage: ./build.sh [-p <target_dir>] [-c] [-m] [-b] [-f] [-k] [-g] [-s] [-j <n>] [-h]
#
#   -p <target_dir>   target 目录路径（默认 targets/nxp/vmu_rt1170/cm7）
#   -c                清理 build 目录
#   -m                CMake 配置
#   -b                编译（无 CMakeCache 时自动 -m）
#   -f                JLink 烧录
#   -k                menuconfig（结束后提示 sync）
#   -g                guiconfig（结束后提示 sync）
#   -s                sync .config -> defconfig
#   -j <n>            并行编译核心数（默认 14）
#   -h                显示帮助

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ── 全局环境 ──────────────────────────────────────────────────────────────────
ZEPHYR_BASE="${ZEPHYR_BASE:-$SCRIPT_DIR/middlewares/zephyr}"
ZEPHYR_MODULES="${ZEPHYR_MODULES:-$SCRIPT_DIR/hardware/hal_nxp:$SCRIPT_DIR/hardware/cmsis:$SCRIPT_DIR/hardware/cmsis_6}"
JOBS="${JOBS:-14}"
JLINK="${JLINK:-JLinkExe}"
JLINK_SPEED="${JLINK_SPEED:-4000}"

# ── venv ──────────────────────────────────────────────────────────────────────
for v in "$SCRIPT_DIR/.venv/bin/activate" \
          "$ZEPHYR_BASE/../.venv/bin/activate"; do
    if [ -f "$v" ]; then
        # shellcheck disable=SC1090
        source "$v"
        break
    fi
done

export ZEPHYR_BASE ZEPHYR_MODULES

# ── Helpers ───────────────────────────────────────────────────────────────────
die()  { echo "[error] $*" >&2; exit 1; }
info() { echo "[build.sh] $*"; }

show_help() {
    cat <<EOF
Usage: ./build.sh [-p <target_dir>] [options]

  -p <target_dir>   Target directory (default: targets/nxp/vmu_rt1170/cm7)
  -j <n>            Parallel jobs (default: 14)
  -c                Clean build directory
  -m                CMake configure
  -b                Build (auto-configures if needed)
  -f                Flash via JLink
  -k                menuconfig (prompts sync after)
  -g                guiconfig  (prompts sync after)
  -s                Sync .config differences back to defconfig
  -h                Show this help

Examples:
  ./build.sh -b                              # build cm7 (default)
  ./build.sh -p targets/nxp/vmu_rt1170/cm4 -b
  ./build.sh -c -b -f                        # clean, build, flash cm7
  ./build.sh -k                              # menuconfig cm7
EOF
}

load_target() {
    local conf="$SCRIPT_DIR/$TARGET_DIR/target.conf"
    [ -f "$conf" ] || die "target.conf not found: $conf"
    # shellcheck disable=SC1090
    source "$conf"
    [ -n "$BUILD_DIR" ]    || die "BUILD_DIR not set in target.conf"
    [ -n "$CMAKE_SOURCE" ] || die "CMAKE_SOURCE not set in target.conf"
    [ -n "$DEFCONFIG" ]    || die "DEFCONFIG not set in target.conf"
}

do_clean() {
    info "Cleaning $BUILD_DIR ..."
    rm -rf "$BUILD_DIR"
}

do_configure() {
    info "Configuring $TARGET_NAME ..."
    cmake -B "$BUILD_DIR" -S "$CMAKE_SOURCE"
}

do_build() {
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
        info "No CMakeCache found, configuring first ..."
        do_configure
    fi
    info "Building $TARGET_NAME ..."
    cmake --build "$BUILD_DIR" -- -j"$JOBS"
}

do_flash() {
    [ -n "$JLINK_DEVICE" ] || die "JLINK_DEVICE not set in target.conf"
    [ -n "$FLASH_ADDR" ]   || die "FLASH_ADDR not set in target.conf"
    local script="/tmp/jlink_${TARGET_NAME}.jlink"
    printf 'r\nloadbin %s/zephyr/zephyr.bin,%s\nr\ng\nexit\n' \
        "$BUILD_DIR" "$FLASH_ADDR" > "$script"
    info "Flashing $TARGET_NAME ..."
    "$JLINK" -device "$JLINK_DEVICE" -if SWD -speed "$JLINK_SPEED" \
        -autoconnect 1 -CommanderScript "$script"
}

do_menuconfig() {
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
        cmake -B "$BUILD_DIR" -S "$CMAKE_SOURCE" -Wno-dev 2>/dev/null || true
    fi
    info "Opening menuconfig for $TARGET_NAME ..."
    cmake --build "$BUILD_DIR" --target menuconfig
    do_sync
}

do_guiconfig() {
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
        cmake -B "$BUILD_DIR" -S "$CMAKE_SOURCE" -Wno-dev 2>/dev/null || true
    fi
    info "Opening guiconfig for $TARGET_NAME ..."
    cmake --build "$BUILD_DIR" --target guiconfig
    do_sync
}

do_sync() {
    info "Syncing .config for $TARGET_NAME ..."
    python3 tools/sync_defconfig.py \
        --target-dir "$TARGET_DIR" \
        --defconfig  "$DEFCONFIG" \
        --dot-config "$BUILD_DIR/zephyr/.config" || true
}

# ── 参数解析 ──────────────────────────────────────────────────────────────────
TARGET_DIR="${TARGET_DIR:-targets/nxp/vmu_rt1170/cm7}"
DO_CLEAN=0
DO_CONFIGURE=0
DO_BUILD=0
DO_FLASH=0
DO_MENUCONFIG=0
DO_GUICONFIG=0
DO_SYNC=0

while getopts ":p:j:cmbfkgsh" opt; do
    case $opt in
        p) TARGET_DIR="$OPTARG" ;;
        c) DO_CLEAN=1 ;;
        m) DO_CONFIGURE=1 ;;
        b) DO_BUILD=1 ;;
        f) DO_FLASH=1 ;;
        k) DO_MENUCONFIG=1 ;;
        g) DO_GUICONFIG=1 ;;
        s) DO_SYNC=1 ;;
        j) JOBS="$OPTARG" ;;
        h) show_help; exit 0 ;;
        :) die "Option -$OPTARG requires an argument." ;;
        \?) die "Unknown option: -$OPTARG" ;;
    esac
done

[ -z "$TARGET_DIR" ] && { show_help; exit 1; }
load_target

# ── 按逻辑顺序执行 ────────────────────────────────────────────────────────────
[ $DO_CLEAN      -eq 1 ] && do_clean      || true
[ $DO_CONFIGURE  -eq 1 ] && do_configure  || true
[ $DO_BUILD      -eq 1 ] && do_build      || true
[ $DO_MENUCONFIG -eq 1 ] && do_menuconfig || true
[ $DO_GUICONFIG  -eq 1 ] && do_guiconfig  || true
[ $DO_FLASH      -eq 1 ] && do_flash      || true
[ $DO_SYNC       -eq 1 ] && do_sync       || true
