#include "rtos_api.h"

#ifndef __IDLE_MODE_H__
#define __IDLE_MODE_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 


bool IdleModeInit(void);

bool IdleModeDeinit(void);

void IdleModeRun(uint16_t msgId);

void IdleModeConfig(void);

uint16_t GetEnterIdleModeScanKey(void);

void SendEnterIdleModeMsg(void);

void SendQuitIdleModeMsg(void);


#ifdef __cplusplus
}
#endif // __cplusplus 

#endif // __LINE_MODE_H__



