/**
 **************************************************************************************
 * @file    i2sin_mode.h
 * @brief   i2s in play mode 
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2019-1-4 17:27:00$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __I2SIN_MODE_H__
#define __I2SIN_MODE_H__


#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 
bool  I2SInPlayInit(void);
bool I2SInPlayDeinit(void);
void I2SInPlayRun(uint16_t msgId);
#ifdef __cplusplus
}
#endif // __cplusplus 

#endif // __I2SIN_MODE_H__
