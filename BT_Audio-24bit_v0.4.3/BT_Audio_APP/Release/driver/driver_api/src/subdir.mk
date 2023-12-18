################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../driver/driver_api/src/adc_interface.c \
../driver/driver_api/src/backup_interface.c \
../driver/driver_api/src/dac_interface.c \
../driver/driver_api/src/fft_api.c \
../driver/driver_api/src/flash_interface.c \
../driver/driver_api/src/i2c_host.c \
../driver/driver_api/src/i2c_interface.c \
../driver/driver_api/src/i2s_interface.c \
../driver/driver_api/src/ppwm_interface.c \
../driver/driver_api/src/sadc_interface.c \
../driver/driver_api/src/spdif_interface.c \
../driver/driver_api/src/spim_interface.c \
../driver/driver_api/src/sw_uart.c \
../driver/driver_api/src/uarts_interface.c 

OBJS += \
./driver/driver_api/src/adc_interface.o \
./driver/driver_api/src/backup_interface.o \
./driver/driver_api/src/dac_interface.o \
./driver/driver_api/src/fft_api.o \
./driver/driver_api/src/flash_interface.o \
./driver/driver_api/src/i2c_host.o \
./driver/driver_api/src/i2c_interface.o \
./driver/driver_api/src/i2s_interface.o \
./driver/driver_api/src/ppwm_interface.o \
./driver/driver_api/src/sadc_interface.o \
./driver/driver_api/src/spdif_interface.o \
./driver/driver_api/src/spim_interface.o \
./driver/driver_api/src/sw_uart.o \
./driver/driver_api/src/uarts_interface.o 

C_DEPS += \
./driver/driver_api/src/adc_interface.d \
./driver/driver_api/src/backup_interface.d \
./driver/driver_api/src/dac_interface.d \
./driver/driver_api/src/fft_api.d \
./driver/driver_api/src/flash_interface.d \
./driver/driver_api/src/i2c_host.d \
./driver/driver_api/src/i2c_interface.d \
./driver/driver_api/src/i2s_interface.d \
./driver/driver_api/src/ppwm_interface.d \
./driver/driver_api/src/sadc_interface.d \
./driver/driver_api/src/spdif_interface.d \
./driver/driver_api/src/spim_interface.d \
./driver/driver_api/src/sw_uart.d \
./driver/driver_api/src/uarts_interface.d 


# Each subdirectory must supply rules for building sources it contributes
driver/driver_api/src/%.o: ../driver/driver_api/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DFUNC_OS_EN=1 -DCFG_APP_CONFIG -DHAVE_CONFIG_H -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/cec/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_hdmi" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/audio" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/soft_watchdog" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_spdif" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_framework/slow_device_engine" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode__common" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_bt" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_i2s" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_idle" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_linein" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_media" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_radio" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_usb_audio" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/bluetooth" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/ble" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/flash_manage" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/rtc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/upgrade" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/hmi/detect" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/hmi/display" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/hmi/fm" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/hmi/key" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/system_config" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/flashboot" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_framework/mode_engine" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_framework/bluetooth_engine" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_framework/audio_engine" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/driver/driver_api/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/driver/driver_api/inc/otg" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/driver/driver/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/power" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/rtc/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/fatfs/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/bluetooth/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/audio/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/mv_utils/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/lrc/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/rtos/rtos_api" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/rtos/freertos/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/flashfs/inc" -Os1 -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -std=gnu99 -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


