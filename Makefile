BUILD := ./build.sh

VMU_CM7 := targets/nxp/vmu_rt1170/cm7
VMU_CM4 := targets/nxp/vmu_rt1170/cm4

.PHONY: all cm7 cm4 \
        flash_cm7 flash_cm4 \
        menuconfig_cm7 menuconfig_cm4 \
        guiconfig_cm7 guiconfig_cm4 \
        sync_cm7 sync_cm4 \
        clean help

all: cm7

cm7:
	$(BUILD) -p $(VMU_CM7) -b

cm4:
	$(BUILD) -p $(VMU_CM4) -b

flash_cm7:
	$(BUILD) -p $(VMU_CM7) -f

flash_cm4:
	$(BUILD) -p $(VMU_CM4) -f

menuconfig_cm7:
	$(BUILD) -p $(VMU_CM7) -k

menuconfig_cm4:
	$(BUILD) -p $(VMU_CM4) -k

guiconfig_cm7:
	$(BUILD) -p $(VMU_CM7) -g

guiconfig_cm4:
	$(BUILD) -p $(VMU_CM4) -g

sync_cm7:
	$(BUILD) -p $(VMU_CM7) -s

sync_cm4:
	$(BUILD) -p $(VMU_CM4) -s

clean:
	rm -rf build/

help:
	@echo "Targets:"
	@echo "  all              - Build CM7 (default)"
	@echo "  cm7              - Build CM7"
	@echo "  cm4              - Build CM4"
	@echo "  flash_cm7        - Flash CM7 via JLink"
	@echo "  flash_cm4        - Flash CM4 via JLink"
	@echo "  menuconfig_cm7   - Terminal Kconfig UI for CM7"
	@echo "  menuconfig_cm4   - Terminal Kconfig UI for CM4"
	@echo "  guiconfig_cm7    - GUI Kconfig UI for CM7"
	@echo "  guiconfig_cm4    - GUI Kconfig UI for CM4"
	@echo "  sync_cm7         - Sync .config -> CM7 defconfig"
	@echo "  sync_cm4         - Sync .config -> CM4 defconfig"
	@echo "  clean            - Remove build/"
	@echo ""
	@echo "For advanced usage: ./build.sh -h"
