# rtframe

Zephyr-based firmware for vmu_rt1170 (i.MX RT1176, CM7 + CM4).

## Dependencies

| Dependency | Variable | Description |
|---|---|---|
| Zephyr RTOS | `ZEPHYR_BASE` | Path to zephyr repo root |
| arm-none-eabi toolchain | `GNUARMEMB_TOOLCHAIN_PATH` | GCC ARM toolchain root (optional, falls back to system PATH) |
| NXP HAL + other modules | `ZEPHYR_MODULES` | Colon-separated list of module paths |

### Minimal setup example

```bash
export ZEPHYR_BASE=/path/to/zephyr
export ZEPHYR_MODULES=/path/to/zephyrproject/modules/hal/nxp:/path/to/zephyrproject/modules/hal/cmsis
# GNUARMEMB_TOOLCHAIN_PATH is optional if arm-none-eabi-gcc is in PATH
```

## Build

### CM7 (main core)

```bash
cmake -B build/cm7 -S targets/cm7
cmake --build build/cm7 -- -j$(nproc)
```

### CM4 (co-processor)

```bash
cmake -B build/cm4 -S targets/cm4
cmake --build build/cm4 -- -j$(nproc)
```

## Project structure

```
rtframe/
├── cmake/          # Toolchain and Zephyr resolution
├── boards/         # Board DTS/Kconfig — owned by this repo, not overlays
├── core/           # System foundation (event bus, logging, params)
├── modules/        # Hardware-agnostic application modules
├── drivers/        # Hardware abstraction over Zephyr drivers
├── lib/            # Pure algorithm libraries (no OS dependency)
├── src/            # Application entry points
└── targets/
    ├── cm7/        # CM7 CMakeLists + prj.conf
    └── cm4/        # CM4 CMakeLists + prj.conf
```
