/**
 **************************************************************************************
 * @file    hdmi_play.h
 * @brief   hdmi in
 *
 * @author  Cecilia Wang
 * @version V1.0.0
 *
 * $Created: 2018-1-5 11:40:00$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __HDMI_IN_MODE_H__
#define __HDMI_IN_MODE_H__


#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 

#define HDMI_IN_SOURCE_NUM					APP_SOURCE_NUM

bool HdmiInPlayInit(void);
void HdmiInPlayRun(uint16_t msgId);
bool HdmiInPlayDeinit(void);


#ifdef __cplusplus
}
#endif // __cplusplus 

#endif // __HDMI_IN_MODE_H__
