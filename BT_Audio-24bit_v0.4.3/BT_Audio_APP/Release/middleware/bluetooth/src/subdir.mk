################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../middleware/bluetooth/src/ble_app_callback.c \
../middleware/bluetooth/src/bt_mid_a2dp.c \
../middleware/bluetooth/src/bt_mid_avrcp.c \
../middleware/bluetooth/src/bt_mid_hfp.c \
../middleware/bluetooth/src/bt_mid_interface.c \
../middleware/bluetooth/src/bt_mid_manager.c \
../middleware/bluetooth/src/bt_mid_tws.c 

OBJS += \
./middleware/bluetooth/src/ble_app_callback.o \
./middleware/bluetooth/src/bt_mid_a2dp.o \
./middleware/bluetooth/src/bt_mid_avrcp.o \
./middleware/bluetooth/src/bt_mid_hfp.o \
./middleware/bluetooth/src/bt_mid_interface.o \
./middleware/bluetooth/src/bt_mid_manager.o \
./middleware/bluetooth/src/bt_mid_tws.o 

C_DEPS += \
./middleware/bluetooth/src/ble_app_callback.d \
./middleware/bluetooth/src/bt_mid_a2dp.d \
./middleware/bluetooth/src/bt_mid_avrcp.d \
./middleware/bluetooth/src/bt_mid_hfp.d \
./middleware/bluetooth/src/bt_mid_interface.d \
./middleware/bluetooth/src/bt_mid_manager.d \
./middleware/bluetooth/src/bt_mid_tws.d 


# Each subdirectory must supply rules for building sources it contributes
middleware/bluetooth/src/%.o: ../middleware/bluetooth/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DFUNC_OS_EN=1 -DCFG_APP_CONFIG -DHAVE_CONFIG_H -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/cec/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_hdmi" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/audio" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/soft_watchdog" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_spdif" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_framework/slow_device_engine" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode__common" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_bt" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_i2s" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_idle" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_linein" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_media" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_radio" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_usb_audio" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/bluetooth" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/ble" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/flash_manage" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/rtc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/upgrade" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/hmi/detect" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/hmi/display" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/hmi/fm" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/hmi/key" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/system_config" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/flashboot" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_framework/mode_engine" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_framework/bluetooth_engine" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_framework/audio_engine" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/driver/driver_api/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/driver/driver_api/inc/otg" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/driver/driver/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/power" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/rtc/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/fatfs/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/bluetooth/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/audio/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/mv_utils/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/lrc/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/rtos/rtos_api" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/rtos/freertos/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/flashfs/inc" -Os1 -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -std=gnu99 -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


