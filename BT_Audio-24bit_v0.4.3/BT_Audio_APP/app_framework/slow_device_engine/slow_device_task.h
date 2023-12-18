/**
 **************************************************************************************
 * @file    mode_task.h
 * @brief  sys_mode
 *
 * @author  kenbu/bkd
 * @version V0.0.1
 *
 * $Created: 2021-02-26
 *
 * @Copyright (C) 2021, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __MODE_TASK_H__
#define __MODE_TASK_H__
#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"

#define SLOW_DEVICE_TASK_STACK_SIZE 512
#define SLOW_DEVICE_TASK_PRIO 3
#define Slow_Device_MESSAGE_QUEUE_NUM 20

void SlowDevice_MsgSend(uint16_t msgId);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif //__MODE_TASK_H__

