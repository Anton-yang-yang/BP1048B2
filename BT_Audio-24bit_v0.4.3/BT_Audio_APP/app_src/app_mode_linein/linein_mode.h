/**
 **************************************************************************************
 * @file    line_play.h
 * @brief   linein 
 *
 * @author  pi
 * @version V1.0.0
 *
 * $Created: 2018-1-5 11:40:00$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include "rtos_api.h"

#ifndef __LINE_MODE_H__
#define __LINE_MODE_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 
bool LineInPlayInit(void);
bool LineInPlayDeinit(void);
void LineInPlayRun(uint16_t msgId);
#ifdef __cplusplus
}
#endif // __cplusplus 

#endif // __LINE_MODE_H__
