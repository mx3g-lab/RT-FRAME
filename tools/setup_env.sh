#!/usr/bin/env bash
# Bootstrap the rtframe build environment on a new machine.
# Run once from the repo root: bash tools/setup_env.sh [--check] [SDK_VERSION]
#
# What it does:
#   0. Initialize git submodules if not already done
#   1. Download Zephyr SDK minimal skeleton for current platform
#   2. Download toolchains listed in tools/toolchains.conf
#   3. Download hosttools
#   4. Create Python venv at <repo>/.venv
#   5. Install Zephyr Python requirements into the venv

set -euo pipefail

# ── Parse args ────────────────────────────────────────────────────────────────
CHECK_ONLY=false
for arg in "$@"; do
    case "$arg" in
        --check) CHECK_ONLY=true ;;
        --*) echo "Unknown option: $arg"; exit 1 ;;
        *) SDK_VERSION="$arg" ;;
    esac
done

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SDK_VERSION="${SDK_VERSION:-1.0.0}"
SDK_DIR="${REPO_ROOT}/toolchain/zephyr-sdk-${SDK_VERSION}"
VENV_DIR="${REPO_ROOT}/.venv"
ZEPHYR_SCRIPTS="${REPO_ROOT}/middlewares/zephyr/scripts"
TOOLCHAINS_CONF="${REPO_ROOT}/tools/toolchains.conf"
SDK_BASE_URL="https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${SDK_VERSION}"

# ── Detect platform ───────────────────────────────────────────────────────────
OS="$(uname -s)"
ARCH="$(uname -m)"

case "${OS}" in
    Linux)
        case "${ARCH}" in
            x86_64)  PLATFORM="linux-x86_64" ;;
            aarch64) PLATFORM="linux-aarch64" ;;
            *) echo "ERROR: Unsupported Linux arch: ${ARCH}"; exit 1 ;;
        esac
        ;;
    Darwin)
        case "${ARCH}" in
            arm64)   PLATFORM="macos-aarch64" ;;
            x86_64)  PLATFORM="macos-x86_64" ;;
            *) echo "ERROR: Unsupported macOS arch: ${ARCH}"; exit 1 ;;
        esac
        ;;
    *) echo "ERROR: Unsupported OS: ${OS}"; exit 1 ;;
esac

SDK_MINIMAL_TAR="zephyr-sdk-${SDK_VERSION}_${PLATFORM}_minimal.tar.xz"
HOSTTOOLS_TAR="hosttools_${PLATFORM}.tar.xz"

echo "[ENV] Platform: ${OS} ${ARCH} (${PLATFORM})"

# ── Read toolchains.conf ──────────────────────────────────────────────────────
read_toolchains() {
    if [ ! -f "${TOOLCHAINS_CONF}" ]; then
        echo "ERROR: ${TOOLCHAINS_CONF} not found."
        exit 1
    fi
    # strip comments and blank lines
    grep -v '^\s*#' "${TOOLCHAINS_CONF}" | grep -v '^\s*$' || true
}

# ── Check tools ───────────────────────────────────────────────────────────────
need() {
    command -v "$1" &>/dev/null || { echo "ERROR: '$1' not found, please install it."; exit 1; }
}

need curl
need tar
need git

# Prefer python3.13+ (Zephyr requires >= 3.12)
if command -v python3.13 &>/dev/null; then
    PYTHON3=python3.13
elif command -v python3.12 &>/dev/null; then
    PYTHON3=python3.12
elif python3 -c "import sys; assert sys.version_info >= (3,12)" 2>/dev/null; then
    PYTHON3=python3
else
    echo "ERROR: Python 3.12+ not found. Zephyr requires Python >= 3.12."
    echo "       Linux:  sudo apt install python3.13"
    echo "       macOS:  brew install python@3.13"
    exit 1
fi
echo "[ENV] Using ${PYTHON3} ($(${PYTHON3} --version))"

# ── Check mode ────────────────────────────────────────────────────────────────
if [ "${CHECK_ONLY}" = true ]; then
    ERRORS=0

    # submodules
    if [ -f "${REPO_ROOT}/middlewares/zephyr/CMakeLists.txt" ] && \
       [ -f "${REPO_ROOT}/hardware/hal_nxp/zephyr/module.yml" ]; then
        echo "[OK ] submodules initialized"
    else
        echo "[ERR] submodules not initialized"
        ERRORS=$((ERRORS+1))
    fi

    # SDK
    INSTALLED_SDK=""
    for d in "${REPO_ROOT}"/toolchain/zephyr-sdk-*/; do
        [ -f "${d}sdk_version" ] && INSTALLED_SDK="${d}" && break
    done
    if [ -n "${INSTALLED_SDK}" ]; then
        echo "[OK ] SDK: ${INSTALLED_SDK}"
        # check each toolchain in conf
        while IFS= read -r tc; do
            GCC="${INSTALLED_SDK}gnu/${tc}/bin/${tc}-gcc"
            if [ -x "${GCC}" ]; then
                GCC_VER=$("${GCC}" --version 2>&1 | head -1)
                echo "[OK ]   toolchain: ${tc}"
                echo "          gcc: ${GCC_VER}"
            else
                echo "[ERR]   toolchain not found: ${tc}"
                ERRORS=$((ERRORS+1))
            fi
        done < <(read_toolchains)
    else
        echo "[ERR] No Zephyr SDK found under ${REPO_ROOT}/toolchain/"
        ERRORS=$((ERRORS+1))
    fi

    # venv
    if [ -f "${VENV_DIR}/bin/python3" ]; then
        PYTHON_VER=$("${VENV_DIR}/bin/python3" --version 2>&1)
        if "${VENV_DIR}/bin/python3" -c "import elftools" 2>/dev/null; then
            echo "[OK ] venv: ${VENV_DIR}"
            echo "      python: ${PYTHON_VER}"
        else
            echo "[ERR] venv exists but pyelftools not installed"
            ERRORS=$((ERRORS+1))
        fi
    else
        echo "[ERR] venv not found at ${VENV_DIR}"
        ERRORS=$((ERRORS+1))
    fi

    if [ "${ERRORS}" -eq 0 ]; then
        echo ""
        echo "Environment ready. Build with: make cm7 / make cm4"
    else
        echo ""
        echo "Environment has ${ERRORS} error(s). Run: bash tools/setup_env.sh"
        exit 1
    fi
    exit 0
fi

# ── Step 0: Submodules ────────────────────────────────────────────────────────
if [ ! -f "${REPO_ROOT}/middlewares/zephyr/CMakeLists.txt" ] || \
   [ ! -f "${REPO_ROOT}/hardware/hal_nxp/zephyr/module.yml" ]; then
    echo "[GIT] Initializing submodules..."
    git -C "${REPO_ROOT}" submodule update --init --recursive
    echo "[GIT] Done."
else
    echo "[GIT] Submodules already initialized."
fi

# ── Step 1: Zephyr SDK skeleton ───────────────────────────────────────────────
if [ -f "${SDK_DIR}/sdk_version" ]; then
    echo "[SDK] already installed at ${SDK_DIR}, skipping skeleton download."
else
    echo "[SDK] Downloading Zephyr SDK ${SDK_VERSION} skeleton for ${PLATFORM}..."
    TMP_DIR="$(mktemp -d)"
    trap 'rm -rf "$TMP_DIR"' EXIT

    curl -fL "${SDK_BASE_URL}/${SDK_MINIMAL_TAR}" -o "${TMP_DIR}/${SDK_MINIMAL_TAR}"
    echo "[SDK] Extracting SDK skeleton..."
    mkdir -p "${REPO_ROOT}/toolchain"
    tar -xJf "${TMP_DIR}/${SDK_MINIMAL_TAR}" -C "${REPO_ROOT}/toolchain"
    echo "[SDK] Skeleton done: ${SDK_DIR}"
fi

# ── Step 2: Toolchains ────────────────────────────────────────────────────────
mkdir -p "${SDK_DIR}/gnu"

while IFS= read -r tc; do
    TC_DIR="${SDK_DIR}/gnu/${tc}"
    if [ -d "${TC_DIR}" ]; then
        echo "[TC ] already installed: ${tc}"
        continue
    fi

    TC_TAR="toolchain_gnu_${PLATFORM}_${tc}.tar.xz"
    echo "[TC ] Downloading toolchain: ${tc}..."
    TMP_DIR="$(mktemp -d)"
    trap 'rm -rf "$TMP_DIR"' EXIT

    curl -fL "${SDK_BASE_URL}/${TC_TAR}" -o "${TMP_DIR}/${TC_TAR}"
    echo "[TC ] Extracting ${tc}..."
    tar -xJf "${TMP_DIR}/${TC_TAR}" -C "${SDK_DIR}"
    # move to gnu/ if extracted to root
    if [ -d "${SDK_DIR}/${tc}" ]; then
        mv "${SDK_DIR}/${tc}" "${SDK_DIR}/gnu/"
    fi
    echo "[TC ] Done: ${tc}"
done < <(read_toolchains)

# ── Step 3: Hosttools ─────────────────────────────────────────────────────────
if [ -f "${SDK_DIR}/hosttools/zephyr-sdk-x86_64-hosttools-standalone-0.10.sh" ] || \
   [ -d "${SDK_DIR}/hosttools/sysroots" ]; then
    echo "[HT ] hosttools already installed."
else
    echo "[HT ] Downloading hosttools for ${PLATFORM}..."
    TMP_DIR="$(mktemp -d)"
    trap 'rm -rf "$TMP_DIR"' EXIT
    curl -fL "${SDK_BASE_URL}/${HOSTTOOLS_TAR}" -o "${TMP_DIR}/${HOSTTOOLS_TAR}"
    echo "[HT ] Extracting hosttools..."
    tar -xJf "${TMP_DIR}/${HOSTTOOLS_TAR}" -C "${SDK_DIR}"
    echo "[HT ] Done."
fi

# ── Step 4: Python venv ───────────────────────────────────────────────────────
if [ -f "${VENV_DIR}/bin/activate" ]; then
    echo "[VENV] already exists at ${VENV_DIR}, skipping."
else
    if [ ! -d "${ZEPHYR_SCRIPTS}" ]; then
        echo "ERROR: middlewares/zephyr not found."
        echo "       Run: git submodule update --init middlewares/zephyr"
        exit 1
    fi

    echo "[VENV] Creating Python venv at ${VENV_DIR}..."
    "${PYTHON3}" -m venv "${VENV_DIR}"
    # shellcheck disable=SC1091
    source "${VENV_DIR}/bin/activate"
    pip install --upgrade pip
    echo "[VENV] Installing Zephyr Python requirements..."
    pip install -r "${ZEPHYR_SCRIPTS}/requirements.txt"
    echo "[VENV] Installing uORB message generator deps (empy/genmsg)..."
    # empy must be 3.3.x — empy 4.x changed em.Interpreter API, breaks PX4 generator
    pip install "empy==3.3.4" pyros-genmsg
    echo "[VENV] Done."
fi

# ── Done ──────────────────────────────────────────────────────────────────────
bash "${BASH_SOURCE[0]}" --check
