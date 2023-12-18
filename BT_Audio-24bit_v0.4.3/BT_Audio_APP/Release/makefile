################################################################################
# Automatically-generated file. Do not edit!
################################################################################

ifndef ANDESIGHT_ROOT
ANDESIGHT_ROOT=/cygdrive/C/Andestech/AndeSight300RDS
endif

ifndef CROSS_COMPILE
CROSS_COMPILE=nds32le-elf-
endif

ifndef SECONDARY_OUTPUT_PATH
SECONDARY_OUTPUT_PATH=output
endif

$(shell mkdir -p $(SECONDARY_OUTPUT_PATH))

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include app_framework/audio_engine/subdir.mk
-include app_framework/bluetooth_engine/subdir.mk
-include app_framework/mode_engine/subdir.mk
-include app_framework/slow_device_engine/subdir.mk
-include app_src/app_mode__common/subdir.mk
-include app_src/app_mode_bt/subdir.mk
-include app_src/app_mode_hdmi/subdir.mk
-include app_src/app_mode_i2s/subdir.mk
-include app_src/app_mode_idle/subdir.mk
-include app_src/app_mode_linein/subdir.mk
-include app_src/app_mode_media/subdir.mk
-include app_src/app_mode_radio/subdir.mk
-include app_src/app_mode_spdif/subdir.mk
-include app_src/app_mode_usb_audio/subdir.mk
-include app_src/components/audio/subdir.mk
-include app_src/components/audio/music_parameter/subdir.mk
-include app_src/components/ble/subdir.mk
-include app_src/components/bluetooth/subdir.mk
-include app_src/components/dump/subdir.mk
-include app_src/components/flash_manage/subdir.mk
-include app_src/components/rtc/subdir.mk
-include app_src/components/soft_watchdog/subdir.mk
-include app_src/components/upgrade/subdir.mk
-include app_src/hmi/detect/subdir.mk
-include app_src/hmi/display/subdir.mk
-include app_src/hmi/fm/subdir.mk
-include app_src/hmi/key/subdir.mk
-include app_src/subdir.mk
-include app_src/power/subdir.mk
-include app_src/startup/subdir.mk
-include driver/driver_api/src/subdir.mk
-include driver/driver_api/src/otg/device/subdir.mk
-include middleware/audio/inc/subdir.mk
-include middleware/audio/src/subdir.mk
-include middleware/bluetooth/src/subdir.mk
-include middleware/fatfs/src/subdir.mk
-include middleware/flashboot/subdir.mk
-include middleware/flashfs/src/subdir.mk
-include middleware/mv_utils/src/subdir.mk
-include middleware/rtos/rtos_api/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
BUILD_ARTIFACT_FILE_BASE_NAME = BT_Audio_APP

LINKER_OUTPUTS = BT_Audio_APP.adx

GCOV_OUT += \
$(foreach subdir,$(SUBDIRS),$(wildcard $(subdir)/*.gcda) $(wildcard $(subdir)/*.gcno)) \

GPROF_OUT += \
../gmon.sum ../gmon.out \

SYMBOL_OUTPUTS += \
$(SECONDARY_OUTPUT_PATH)/symbol.txt \
$(SECONDARY_OUTPUT_PATH)/symbol.txt \

READELF_OUTPUTS += \
$(SECONDARY_OUTPUT_PATH)/readelf.txt \
$(SECONDARY_OUTPUT_PATH)/readelf.txt \

OBJDUMP_OUTPUTS += \
$(SECONDARY_OUTPUT_PATH)/objdump.txt \
$(SECONDARY_OUTPUT_PATH)/objdump.txt \

OBJCOPY_OUTPUTS += \
$(SECONDARY_OUTPUT_PATH)/$(BUILD_ARTIFACT_FILE_BASE_NAME).bin \
$(SECONDARY_OUTPUT_PATH)/$(BUILD_ARTIFACT_FILE_BASE_NAME).bin \

SIZE_OUTPUTS += \
$(SECONDARY_OUTPUT_PATH)/.PHONY.size \
$(SECONDARY_OUTPUT_PATH)/.PHONY.size \


# All Target
all: pre-build 
	$(MAKE) main-build

# Main-build Target
main-build: BT_Audio_APP.adx secondary-outputs

# Tool invocations
BT_Audio_APP.adx: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Andes C Linker'
	$(CROSS_COMPILE)gcc -Os1 -nostartfiles -static -L"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/bluetooth" -L"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/cec" -L"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/driver/driver" -L"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/audio" -L"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/rtc" -L"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/flashfs" -L"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/lrc" -L"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/fatfs" -mcmodel=medium -ldsp -mext-dsp -T"../BP10_8Mbit.ld" -Wl,--gc-sections -mvh -o "BT_Audio_APP.adx" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

$(foreach subdir,$(SUBDIRS),$(wildcard $(subdir)/*.gcda) $(wildcard $(subdir)/*.gcno)): BT_Audio_APP.adx $(OBJS) $(USER_OBJS)
../gmon.sum ../gmon.out: BT_Audio_APP.adx $(OBJS) $(USER_OBJS)

$(SECONDARY_OUTPUT_PATH)/symbol.txt: $(LINKER_OUTPUTS)
	@echo 'Invoking: NM (symbol listing)'
	$(CROSS_COMPILE)nm -n -l -C "BT_Audio_APP.adx" > $(SECONDARY_OUTPUT_PATH)/symbol.txt
	@echo 'Finished building: $@'
	@echo ' '

$(SECONDARY_OUTPUT_PATH)/readelf.txt: $(LINKER_OUTPUTS)
	@echo 'Invoking: Readelf (ELF info listing)'
	$(CROSS_COMPILE)readelf -a "BT_Audio_APP.adx" > $(SECONDARY_OUTPUT_PATH)/readelf.txt
	@echo 'Finished building: $@'
	@echo ' '

$(SECONDARY_OUTPUT_PATH)/objdump.txt: $(LINKER_OUTPUTS)
	@echo 'Invoking: Objdump (disassembly)'
	$(CROSS_COMPILE)objdump -x -d -C "BT_Audio_APP.adx" > $(SECONDARY_OUTPUT_PATH)/objdump.txt
	@echo 'Finished building: $@'
	@echo ' '

$(SECONDARY_OUTPUT_PATH)/$(BUILD_ARTIFACT_FILE_BASE_NAME).bin: $(LINKER_OUTPUTS)
	@echo 'Invoking: Objcopy (object content copy)'
	$(CROSS_COMPILE)objcopy -S -O binary "BT_Audio_APP.adx" $(SECONDARY_OUTPUT_PATH)/$(BUILD_ARTIFACT_FILE_BASE_NAME).bin
	@echo 'Finished building: $@'
	@echo ' '

$(SECONDARY_OUTPUT_PATH)/.PHONY.size: $(LINKER_OUTPUTS)
	@echo 'Invoking: Size (section size listing)'
	$(CROSS_COMPILE)size  "BT_Audio_APP.adx" | tee $(SECONDARY_OUTPUT_PATH)/.PHONY.size
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJCOPY_OUTPUTS)$(OBJS)$(C_DEPS)$(GCOV_OUT)$(SYMBOL_OUTPUTS)$(OBJDUMP_OUTPUTS)$(READELF_OUTPUTS)$(GPROF_OUT)$(SIZE_OUTPUTS)$(EXECUTABLES)$(S_UPPER_DEPS) BT_Audio_APP.adx
	-@echo ' '

pre-build:
	-..\tools\script.bat
	-@echo ' '

post-build:
	-$(CROSS_COMPILE)objcopy -O binary "BT_Audio_APP.adx" output/BT_Audio_APP.bin ; ../tools/script_post.bat
	-@echo ' '

secondary-outputs: $(SYMBOL_OUTPUTS) $(READELF_OUTPUTS) $(OBJDUMP_OUTPUTS) $(OBJCOPY_OUTPUTS) $(SIZE_OUTPUTS)

.PHONY: all clean dependents
.SECONDARY: main-build pre-build post-build
.NOTPARALLEL: pre-build

-include ../makefile.targets
