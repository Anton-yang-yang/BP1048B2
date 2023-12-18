/*
 * mode_task_api.h
 *
 *  Created on: Mar 30, 2021
 *      Author: piwang
 */

#ifndef _MODE_TASK_API_H_
#define _MODE_TASK_API_H_
#include "type.h"


//配置系统标准通路
bool ModeCommonInit(void);

//清理标准通路配置，模式退出后未调用时 标准通路后台化，切模式期间无音效。
void ModeCommonDeinit(void);

//各模式下的通用消息处理
void CommonMsgProccess(uint16_t Msg);

bool AudioIoCommonForHfp(uint32_t sampleRate, uint16_t gain, uint8_t gainBoostSel);

void tws_device_close(void);

void tws_device_open_isr(void);

void PauseAuidoCore(void);


#endif /* _MODE_TASK_API_H_ */
