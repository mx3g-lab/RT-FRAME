#!/usr/bin/env bash
# Bootstrap the rtframe build environment on a new machine.
# Run once from the repo root: bash tools/setup_env.sh
#
# What it does:
#   1. Download Zephyr SDK 1.0.0 (arm-zephyr-eabi + hosttools only)
#   2. Install the SDK to <repo>/toolchain/zephyr-sdk-1.0.0
#   3. Create Python venv at <repo>/.venv
#   4. Install Zephyr Python requirements into the venv

set -euo pipefail

# Parse args
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

SDK_BASE_URL="https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${SDK_VERSION}"
# minimal bundle: arm only + hosttools
SDK_MINIMAL_TAR="zephyr-sdk-${SDK_VERSION}_linux-x86_64_minimal.tar.xz"
ARM_TOOLCHAIN_TAR="toolchain_gnu_linux-x86_64_arm-zephyr-eabi.tar.xz"
HOSTTOOLS_TAR="hosttools_linux-x86_64.tar.xz"

need() {
    command -v "$1" &>/dev/null || { echo "ERROR: '$1' not found, please install it."; exit 1; }
}

need curl
need tar
need python3

# ── Step 1: Zephyr SDK ────────────────────────────────────────────────────────
if [ -f "${SDK_DIR}/sdk_version" ]; then
    echo "[SDK] already installed at ${SDK_DIR}, skipping download."
else
    echo "[SDK] Downloading Zephyr SDK ${SDK_VERSION} (arm-zephyr-eabi + hosttools)..."
    TMP_DIR="$(mktemp -d)"
    trap 'rm -rf "$TMP_DIR"' EXIT

    # minimal SDK skeleton
    curl -fL "${SDK_BASE_URL}/${SDK_MINIMAL_TAR}" -o "${TMP_DIR}/${SDK_MINIMAL_TAR}"
    echo "[SDK] Extracting SDK skeleton..."
    mkdir -p "${REPO_ROOT}/toolchain"
    tar -xJf "${TMP_DIR}/${SDK_MINIMAL_TAR}" -C "${REPO_ROOT}/toolchain"

    # arm-zephyr-eabi toolchain
    curl -fL "${SDK_BASE_URL}/${ARM_TOOLCHAIN_TAR}" -o "${TMP_DIR}/${ARM_TOOLCHAIN_TAR}"
    echo "[SDK] Extracting arm-zephyr-eabi..."
    tar -xJf "${TMP_DIR}/${ARM_TOOLCHAIN_TAR}" -C "${SDK_DIR}"
    # SDK cmake expects toolchains under gnu/
    mkdir -p "${SDK_DIR}/gnu"
    for d in "${SDK_DIR}"/*-zephyr-*; do
        [ -d "$d" ] && mv "$d" "${SDK_DIR}/gnu/"
    done

    # hosttools (cmake, dtc, ninja, …)
    curl -fL "${SDK_BASE_URL}/${HOSTTOOLS_TAR}" -o "${TMP_DIR}/${HOSTTOOLS_TAR}"
    echo "[SDK] Extracting hosttools..."
    tar -xJf "${TMP_DIR}/${HOSTTOOLS_TAR}" -C "${SDK_DIR}"

    # register cmake package so Zephyr can find the SDK
    # Not needed: our cmake/zephyr.cmake uses HINTS to locate the SDK directly.
    # "${SDK_DIR}/setup.sh" -c

    echo "[SDK] Done: ${SDK_DIR}"
fi

# ── Step 2: Python venv ───────────────────────────────────────────────────────
if [ -f "${VENV_DIR}/bin/activate" ]; then
    echo "[VENV] already exists at ${VENV_DIR}, skipping."
else
    if [ ! -d "${ZEPHYR_SCRIPTS}" ]; then
        echo "ERROR: middlewares/zephyr not found."
        echo "       Run: git submodule update --init middlewares/zephyr"
        exit 1
    fi

    echo "[VENV] Creating Python venv at ${VENV_DIR}..."
    python3 -m venv "${VENV_DIR}"
    # shellcheck disable=SC1091
    source "${VENV_DIR}/bin/activate"
    pip install --upgrade pip -q
    echo "[VENV] Installing Zephyr Python requirements..."
    pip install -r "${ZEPHYR_SCRIPTS}/requirements.txt" -q
    echo "[VENV] Done."
fi

# ── Verify ────────────────────────────────────────────────────────────────────
ERRORS=0

# Find the installed SDK (any version under toolchain/)
INSTALLED_SDK=""
for d in "${REPO_ROOT}"/toolchain/zephyr-sdk-*/; do
    [ -f "${d}sdk_version" ] && INSTALLED_SDK="${d}" && break
done

if [ -n "${INSTALLED_SDK}" ]; then
    GCC="${INSTALLED_SDK}gnu/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc"
    if [ -x "${GCC}" ]; then
        GCC_VER=$("${GCC}" --version 2>&1 | head -1)
        echo "[OK ] SDK: ${INSTALLED_SDK}"
        echo "      gcc: ${GCC_VER}"
    else
        echo "[ERR] SDK found but arm-zephyr-eabi-gcc not executable: ${GCC}"
        ERRORS=$((ERRORS+1))
    fi
else
    echo "[ERR] No Zephyr SDK found under ${REPO_ROOT}/toolchain/"
    ERRORS=$((ERRORS+1))
fi

if [ -f "${VENV_DIR}/bin/activate" ]; then
    PYTHON_VER=$(source "${VENV_DIR}/bin/activate" && python3 --version 2>&1)
    # check a key package used by Zephyr build scripts
    if "${VENV_DIR}/bin/python3" -c "import elftools" 2>/dev/null; then
        echo "[OK ] venv: ${VENV_DIR}"
        echo "      python: ${PYTHON_VER}"
    else
        echo "[ERR] venv exists but pyelftools not installed: ${VENV_DIR}"
        ERRORS=$((ERRORS+1))
    fi
else
    echo "[ERR] venv not found at ${VENV_DIR}"
    ERRORS=$((ERRORS+1))
fi

if [ "${ERRORS}" -eq 0 ]; then
    cat <<EOF

Environment ready.
  SDK:  ${INSTALLED_SDK:-${SDK_DIR}}
  venv: ${VENV_DIR}

Build:
  make cm7
  make cm4
EOF
else
    echo ""
    echo "Environment has ${ERRORS} error(s). Run: bash tools/setup_env.sh"
    exit 1
fi