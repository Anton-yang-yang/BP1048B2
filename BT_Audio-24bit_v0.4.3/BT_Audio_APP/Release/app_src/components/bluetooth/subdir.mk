################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../app_src/components/bluetooth/bt_app_a2dp_deal.c \
../app_src/components/bluetooth/bt_app_avrcp_deal.c \
../app_src/components/bluetooth/bt_app_common.c \
../app_src/components/bluetooth/bt_app_connect.c \
../app_src/components/bluetooth/bt_app_ddb_info.c \
../app_src/components/bluetooth/bt_app_hfp_deal.c \
../app_src/components/bluetooth/bt_app_init.c \
../app_src/components/bluetooth/bt_app_sniff.c \
../app_src/components/bluetooth/bt_app_spp.c \
../app_src/components/bluetooth/bt_app_tws.c \
../app_src/components/bluetooth/bt_app_tws_connect.c \
../app_src/components/bluetooth/bt_boot_api.c \
../app_src/components/bluetooth/bt_em_config.c 

OBJS += \
./app_src/components/bluetooth/bt_app_a2dp_deal.o \
./app_src/components/bluetooth/bt_app_avrcp_deal.o \
./app_src/components/bluetooth/bt_app_common.o \
./app_src/components/bluetooth/bt_app_connect.o \
./app_src/components/bluetooth/bt_app_ddb_info.o \
./app_src/components/bluetooth/bt_app_hfp_deal.o \
./app_src/components/bluetooth/bt_app_init.o \
./app_src/components/bluetooth/bt_app_sniff.o \
./app_src/components/bluetooth/bt_app_spp.o \
./app_src/components/bluetooth/bt_app_tws.o \
./app_src/components/bluetooth/bt_app_tws_connect.o \
./app_src/components/bluetooth/bt_boot_api.o \
./app_src/components/bluetooth/bt_em_config.o 

C_DEPS += \
./app_src/components/bluetooth/bt_app_a2dp_deal.d \
./app_src/components/bluetooth/bt_app_avrcp_deal.d \
./app_src/components/bluetooth/bt_app_common.d \
./app_src/components/bluetooth/bt_app_connect.d \
./app_src/components/bluetooth/bt_app_ddb_info.d \
./app_src/components/bluetooth/bt_app_hfp_deal.d \
./app_src/components/bluetooth/bt_app_init.d \
./app_src/components/bluetooth/bt_app_sniff.d \
./app_src/components/bluetooth/bt_app_spp.d \
./app_src/components/bluetooth/bt_app_tws.d \
./app_src/components/bluetooth/bt_app_tws_connect.d \
./app_src/components/bluetooth/bt_boot_api.d \
./app_src/components/bluetooth/bt_em_config.d 


# Each subdirectory must supply rules for building sources it contributes
app_src/components/bluetooth/%.o: ../app_src/components/bluetooth/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DFUNC_OS_EN=1 -DCFG_APP_CONFIG -DHAVE_CONFIG_H -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/system_config" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_hdmi" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/cec/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_framework/slow_device_engine" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/flashboot" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_framework/mode_engine" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_framework/bluetooth_engine" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_framework/audio_engine" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/driver/driver_api/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/driver/driver_api/inc/otg" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/driver/driver/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/power" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/rtc/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/fatfs/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/bluetooth/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/audio/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/mv_utils/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/lrc/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/rtos/rtos_api" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/rtos/freertos/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/middleware/flashfs/inc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode__common" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_bt" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_i2s" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_idle" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_linein" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_media" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_radio" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/app_mode_usb_audio" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/audio" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/bluetooth" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/flash_manage" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/rtc" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/components/upgrade" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/hmi/fm" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/hmi/detect" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/hmi/display" -I"/cygdrive/C/Users/Michio/Desktop/BT_Audio-24bit_v0.4.3/BT_Audio_APP/app_src/hmi/key" -Os1 -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -std=gnu99 -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


