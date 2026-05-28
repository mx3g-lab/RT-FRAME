# rtframe

Zephyr-based firmware for **vmu_rt1170** (NXP i.MX RT1176, Cortex-M7 + Cortex-M4 AMP).

[![CI Build](https://github.com/mx3g-jh/RT-FRAME/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/mx3g-jh/RT-FRAME/actions/workflows/cmake-multi-platform.yml)

---

## Features

- Dual-core support: CM7 (primary) and CM4 (co-processor) as independent build targets
- Self-contained repo: Zephyr RTOS, NXP HAL, CMSIS all included as git submodules
- Board DTS/Kconfig fully owned by this repo (no overlays, direct edit)
- Pure CMake build — no west required
- One-command environment bootstrap: `bash tools/setup_env.sh`
- JLink flashing via `make flash_cm7` / `make flash_cm4`
- Console UART at **921600 baud** (lpuart1)

---

## Quick Start

### 1. Clone with submodules

```bash
git clone git@github.com:mx3g-jh/RT-FRAME.git
cd RT-FRAME
git submodule update --init --recursive
```

### 2. Bootstrap environment (once)

Downloads Zephyr SDK 1.0.0 (arm-zephyr-eabi + hosttools) and creates a Python venv:

```bash
bash tools/setup_env.sh
```

Verify the environment is ready:

```bash
bash tools/setup_env.sh --check
```

Expected output:
```
[OK ] SDK: .../toolchain/zephyr-sdk-1.0.0/
      gcc: arm-zephyr-eabi-gcc (Zephyr SDK 1.0.0) 14.3.0
[OK ] venv: .../.venv
      python: Python 3.13.x
```

### 3. Build

```bash
make cm7        # Build CM7 firmware
make cm4        # Build CM4 firmware
make all        # Build both
```

### 4. Flash

```bash
make flash_cm7  # Flash CM7 via JLink
make flash_cm4  # Flash CM4 via JLink
```

Requires JLink Commander (`JLinkExe`) in PATH, or override:

```bash
make flash_cm7 JLINK=/path/to/JLinkExe
```

---

## Build System

The project uses pure CMake — no west. Environment variables are optional; all dependencies default to paths inside the repo.

| Variable | Default | Description |
|---|---|---|
| `ZEPHYR_BASE` | `middlewares/zephyr` | Path to Zephyr RTOS root |
| `ZEPHYR_MODULES` | `hardware/hal_nxp:hardware/cmsis:hardware/cmsis_6` | Colon-separated module paths |
| `ZEPHYR_SDK_INSTALL_DIR` | `toolchain/zephyr-sdk-*` (auto-detected) | Zephyr SDK root |
| `JLINK` | `JLinkExe` | JLink Commander binary |
| `JLINK_SPEED` | `4000` (kHz) | JLink SWD speed |

### Toolchain detection priority

1. `ZEPHYR_SDK_INSTALL_DIR` env/cmake var
2. `toolchain/zephyr-sdk-*` inside repo (installed by `setup_env.sh`)
3. `GNUARMEMB_TOOLCHAIN_PATH` env/cmake var
4. `arm-none-eabi-gcc` in system PATH

---

## Project Structure

```
rtframe/
├── .github/
│   └── workflows/          # CI build and release workflows
├── boards/
│   └── nxp/vmu_rt1170/     # Board DTS, Kconfig, pinctrl — owned by this repo
├── cmake/
│   ├── toolchain.cmake     # Toolchain auto-detection
│   └── zephyr.cmake        # Zephyr base + module resolution
├── hardware/               # Git submodules
│   ├── hal_nxp/            # NXP HAL (zephyrproject-rtos/hal_nxp)
│   ├── cmsis/              # CMSIS (zephyrproject-rtos/cmsis)
│   └── cmsis_6/            # CMSIS6 (ARM-software/CMSIS_6)
├── middlewares/
│   └── zephyr/             # Zephyr RTOS (mx3g-jh/zephyr fork)
├── src/
│   ├── core/               # System foundation (event bus, logging, params)
│   ├── modules/            # Hardware-agnostic application modules
│   ├── drivers/            # Hardware abstraction over Zephyr drivers
│   ├── lib/                # Pure algorithm libraries (no OS dependency)
│   ├── main.cpp            # CM7 entry point
│   └── main_cm4.cpp        # CM4 entry point
├── targets/
│   ├── cm7/                # CM7 CMakeLists.txt + prj.conf
│   └── cm4/                # CM4 CMakeLists.txt + prj.conf
├── toolchain/              # Zephyr SDK install dir (gitignored, created by setup_env.sh)
├── tools/
│   └── setup_env.sh        # Environment bootstrap script
└── zephyr/
    └── module.yml          # Registers rtframe as a Zephyr module (dts_root)
```

---

## Submodules

| Path | Repository | Description |
|---|---|---|
| `middlewares/zephyr` | [mx3g-jh/zephyr](https://github.com/mx3g-jh/zephyr) | Zephyr RTOS fork (vmu_rt1170 board removed from upstream) |
| `hardware/hal_nxp` | [zephyrproject-rtos/hal_nxp](https://github.com/zephyrproject-rtos/hal_nxp) | NXP HAL |
| `hardware/cmsis` | [zephyrproject-rtos/cmsis](https://github.com/zephyrproject-rtos/cmsis) | CMSIS |
| `hardware/cmsis_6` | [zephyrproject-rtos/CMSIS_6](https://github.com/zephyrproject-rtos/CMSIS_6) | CMSIS6 |

Update submodules to latest tracked commit:

```bash
git submodule update --remote
```

---

## Hardware

**Target board**: vmu_rt1170 (NXP i.MX RT1176)

| Core | Arch | Flash address | Console |
|---|---|---|---|
| CM7 | Cortex-M7 @ 1GHz | `0x30000000` | lpuart1 @ 921600 |
| CM4 | Cortex-M4 @ 400MHz | `0x20200000` | lpuart1 @ 921600 |

---

## CI / Releases

GitHub Actions builds both CM7 and CM4 on every push to `main` and on pull requests. Tagged releases automatically attach firmware artifacts (`zephyr.elf`, `zephyr.bin`, `zephyr.hex`).

```bash
git tag v1.0.0
git push origin v1.0.0
```

---

## License

See [LICENSE](LICENSE).
