ZEPHYR_BASE    ?= $(CURDIR)/middlewares/zephyr
ZEPHYR_MODULES ?= $(CURDIR)/hardware/hal_nxp:$(CURDIR)/hardware/cmsis:$(CURDIR)/hardware/cmsis_6

NPROC := $(shell nproc)

# Find venv next to ZEPHYR_BASE or in well-known locations
VENV_CANDIDATES := \
	$(ZEPHYR_BASE)/../.venv/bin/activate \
	$(ZEPHYR_BASE)/.venv/bin/activate \
	$(CURDIR)/middlewares/.venv/bin/activate

VENV_ACTIVATE := $(firstword $(foreach v,$(VENV_CANDIDATES),$(wildcard $(v))))

define setup_env
	$(if $(VENV_ACTIVATE),. $(VENV_ACTIVATE) &&) \
	ZEPHYR_BASE=$(ZEPHYR_BASE) ZEPHYR_MODULES=$(ZEPHYR_MODULES)
endef

JLINK     ?= JLinkExe
JLINK_SPEED ?= 4000

# JLink flash addresses
FLASH_ADDR_CM7 := 0x30000000
FLASH_ADDR_CM4 := 0x20200000

.PHONY: cm7 cm4 all flash_cm7 flash_cm4 clean help

all: cm7 cm4

cm7:
	@rm -rf build/cm7
	$(setup_env) cmake -B build/cm7 -S targets/cm7
	$(setup_env) cmake --build build/cm7 -- -j$(NPROC)

cm4:
	@rm -rf build/cm4
	$(setup_env) cmake -B build/cm4 -S targets/cm4
	$(setup_env) cmake --build build/cm4 -- -j$(NPROC)

flash_cm7: ## Flash CM7 firmware via JLink
	@printf 'r\nloadbin build/cm7/zephyr/zephyr.bin,$(FLASH_ADDR_CM7)\nr\ng\nexit\n' > /tmp/jlink_cm7.jlink
	$(JLINK) -device MIMXRT1176xxxA_M7 -if SWD -speed $(JLINK_SPEED) -autoconnect 1 \
		-CommanderScript /tmp/jlink_cm7.jlink

flash_cm4: ## Flash CM4 firmware via JLink
	@printf 'r\nloadbin build/cm4/zephyr/zephyr.bin,$(FLASH_ADDR_CM4)\nr\ng\nexit\n' > /tmp/jlink_cm4.jlink
	$(JLINK) -device MIMXRT1176xxxA_M4 -if SWD -speed $(JLINK_SPEED) -autoconnect 1 \
		-CommanderScript /tmp/jlink_cm4.jlink

clean:
	rm -rf build/

help:
	@echo "Targets:"
	@echo "  cm7        - Build CM7 firmware"
	@echo "  cm4        - Build CM4 firmware"
	@echo "  all        - Build both"
	@echo "  flash_cm7  - Flash CM7 via JLink"
	@echo "  flash_cm4  - Flash CM4 via JLink"
	@echo "  clean      - Remove build/"
	@echo ""
	@echo "Variables:"
	@echo "  ZEPHYR_BASE    (default: middlewares/zephyr)"
	@echo "  ZEPHYR_MODULES (default: hardware/hal_nxp:...)"
	@echo "  JLINK          (default: JLinkExe)"
	@echo "  JLINK_SPEED    (default: 4000 kHz)"
