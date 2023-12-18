/**
 **************************************************************************************
 * @file    audio_effect_flash_param.h
 * @brief
 *
 * @author  Sam
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
 
#ifndef __AUDIO_EFFECT_FLASH_PARAM_H__
#define __AUDIO_EFFECT_FLASH_PARAM_H__
 
#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"
#include "audio_effect_api.h"

#pragma pack (1)
typedef struct _EffectPaCt{
	int8_t 		Flag;//音效是否使能
	int16_t 	Offset;//地址偏移量
	int16_t		Len;//参数有效长度
}EffectPaCt;
#pragma pack ()

typedef struct _EffectHeadPaCt{
	int32_t 	Head;//头部信息
	int32_t 	Num;//有效的音效组数
	int32_t		MaxLen;//参数最大长度
}EffectHeadPaCt;

//extern uint8_t EffectParamFlashIndex;
extern uint8_t EffectParamFlashUpdataFlag;

void EffectParamFlashUpdata(void);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__AUDIO_EFFECT_FLASH_PARAM_H__

