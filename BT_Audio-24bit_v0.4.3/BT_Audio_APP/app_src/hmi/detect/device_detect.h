/**
 **************************************************************************************
 * @file    device_detect.h
 * @brief
 *
 * @author  pi
 * @version V1.0.0
 *
 * $Created: 2018-1-5 11:40:00$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __DEVICE_DETECT_H__
#define __DEVICE_DETECT_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "timeout.h"
#include "mode_task.h"
#include "app_message.h"


#define	LINE_IN_EVENT_BIT		0x01
#define	LINE_OUT_EVENT_BIT		0x02
#define CARD_IN_EVENT_BIT		0x04
#define CARD_OUT_EVENT_BIT		0x08
#define UDISK_IN_EVENT_BIT		0x10
#define UDISK_OUT_EVENT_BIT		0x20
#define USB_DEVICE_IN_EVENT_BIT	0x40
#define USB_DEVICE_OUT_EVENT_BIT 0x80
#define HDMI_IN_EVENT_BIT		0x100
#define HDMI_OUT_EVENT_BIT		0x200

#define DEVICE_DETECT_ACTTION_POINT 1

#define DEVICE_DETECT_TIMER 10
#define DEVICE_CARD_DETECT_TIMER 250
#define DEVICE_LINEIN_DETECT_TIMER 500
#define DEVICE_HDMI_DETECT_TIMER 250
#define DEVICE_USB_DEVICE_DETECT_TIMER 300
#define DEVICE_BREAK_POINT_TIMER 800


#define MIPS_LOG_INTERVAL				10000//ms 注意：高优先级任务持续阻塞device时，会影响mips log输出


#define		DETECT_INIT_TIME			20
#define 	DETECT_JITTER_TIME			100
#define		DETECT_OUT_WAIT_TIME		150 //拔出 屏蔽期，避免事件频繁。取代消抖，意在模式硬件重新初始化。

typedef enum _DETECT_STATE
{
	DETECT_STATE_IDLE = 0,
	DETECT_STATE_OUT,
	DETECT_STATE_IN,
	DETECT_STATE_NONE,
} DETECT_STATE;

typedef struct _DeviceDetMsgTable
{
	MessageId MsgID;
	uint32_t SupportMode;
	SysModeNumber EnterMode;
}DeviceEventMsgTable;

bool IsUDiskLink(void);
uint32_t DeviceDetect(void);
// 上电时，硬件扫描消抖
void InitDeviceDetect(void);
bool GetUSBDeviceInitState(void);
void UDiskRemovedFlagSet(bool State);
void SetUSBDeviceInitState(bool state);
void DeviceServiceInit(void);
void DeviceServicePocess(uint16_t device_msgId);
DETECT_STATE GetCardState(void);
DETECT_STATE GetUdiscState(void);
bool IsMediaPlugOut(void);

#endif //__DEV_DETECT_H__

