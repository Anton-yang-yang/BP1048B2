/**
 **************************************************************************************
 * @file    tws mode.h
 * @brief    
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2020-3-6 11:40:00$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __TWS_MODE_H__
#define __TWS_MODE_H__


#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 

typedef struct _TWS_CONFIG_
{
	uint8_t PairMode;
	uint8_t AudioMode;//master传输音频给从机的格式 0:单声道；1：立体声
	uint8_t IsEffectEn;
	uint8_t IsRemindSyncEn;//0:提示音不发送给slave；1:提示音发送给slave同步播放提示音
//	uint8_t MasterSound;//0=L,R, 1=L, 2= R,3=L+R
//	uint8_t SlaverSound;//0=L,R, 1=L, 2= R,3=L+R
//	uint8_t OutMode;    //输出模式选择:
//0:对箱立体声(主箱L,副箱R);1:对箱单声道(主副都是L+R);2:主箱立体声(L,R)，副箱单声道(L+R)
}TWS_CONFIG;
extern TWS_CONFIG	gTwsCfg;

void TwsSlaveModeEnter(void);
void TwsSlaveModeExit(void);
void TWS_Params_Init(void);
bool TwsSlavePlayInit(void);
void TwsSlavePlayRun(uint16_t msgId);
bool TwsSlavePlayDeinit(void);

uint16_t TwsSinkDataSet(void* Buf, uint16_t Len);

uint16_t TwsSinkSpaceLenGet(void);

uint16_t AudioDAC0DataSet_tws(void* Buf, uint16_t Len);

#ifdef __cplusplus
}
#endif // __cplusplus 

#endif // __TWS_MODE_H__
