/**
 **************************************************************************************
 * @file    sys_mode.c
 * @brief   Program Entry 
 *
 * @author  ken
 * @version V1.0.0
 *
 * $Created: 2021-08-12
 *
 * @Copyright (C) 2021, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include "type.h"
#include "app_config.h"
#include "rtos_api.h"
#include "app_message.h"
#include "debug.h"
#include "slow_device_task.h"
#ifdef CFG_FUNC_OPEN_SLOW_DEVICE_TASK

MessageHandle SlowDeviceMsgHandle = NULL;

#ifdef CFG_FUNC_BREAKPOINT_EN
extern void SlowDeviceEventProcess(uint16_t device_msgId);
#endif

static void SlowDeviceEntrance(void * param)
{

	MessageContext		msg;
	SlowDeviceMsgHandle = MessageRegister(Slow_Device_MESSAGE_QUEUE_NUM);
	while(1)
	{
		MessageRecv(SlowDeviceMsgHandle, &msg, 0xffffffff);
		printf("slow device task\n");
		SlowDeviceEventProcess(msg.msgId);
	}
}

void CreatSlowDeviceTask(void)
{	
	xTaskHandle SlowDeviceHandle = NULL;

	if(xTaskCreate(SlowDeviceEntrance, "SlowDevice", SLOW_DEVICE_TASK_STACK_SIZE, NULL, SLOW_DEVICE_TASK_PRIO, &SlowDeviceHandle) != pdTRUE)
	{
		vTaskDelay(50);
		APP_DBG("system error ,mode task create fail 1\n");
		if(xTaskCreate(SlowDeviceEntrance, "SlowDevice", SLOW_DEVICE_TASK_STACK_SIZE, NULL, SLOW_DEVICE_TASK_PRIO, &SlowDeviceHandle) != pdTRUE)
		{
			while(1)
			{
				vTaskDelay(50);
				APP_DBG("system error ,mode task create fail 2\n");
			}
		}
	}
}

void SlowDevice_MsgSend(uint16_t msgId)
{
	MessageContext msgSend;
	msgSend.msgId = msgId;
	MessageSend(SlowDeviceMsgHandle, &msgSend);
}

#endif
	



