/**
 **************************************************************************************
 * @file    usb_audio_mode.h
 * @brief   usb audio mode 
 *
 * @author  pi
 * @version V1.0.0
 *
 * $Created: 2018-05-08 11:40:00$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __USB_AUDIO_MODE_H__
#define __USB_AUDIO_MODE_H__


#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 

bool UsbDevicePlayInit(void);
bool UsbDevicePlayDeinit(void);
void UsbDevicePlayRun(uint16_t msgId);
#ifdef __cplusplus
}
#endif // __cplusplus 

#endif // __USB_AUDIO_MODE_H__
