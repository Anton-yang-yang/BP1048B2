/**
 **************************************************************************************
 * @file    audio_effect_param.h
 * @brief
 *
 * @author  Sam
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
 
#ifndef __AUDIO_EFFECT_PARAM_H__
#define __AUDIO_EFFECT_PARAM_H__
 
#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"
#include "audio_effect_api.h"

typedef struct _AudioEffectCommParamBuffContext
{
	GainControlParam		paramGainControl_0;
	GainControlParam		paramGainControl_1;
	GainControlParam		paramGainControl_2;
	GainControlParam		paramGainControl_3;
	GainControlParam		paramGainControl_4;
	GainControlParam		paramGainControl_5;
	GainControlParam		paramGainControl_6;
	ExpanderParam		paramExpander_7;
	SilenceDetectorParam		paramSilenceDetector_8;
	CompanderParam		paramCompander_9;
	LLCompressorParam		paramLLCompressor_10;
	VBParam		paramVB_11;
	VBClassicParam		paramVBClassic_12;
	ThreeDParam		paramThreeD_13;
	ExciterParam		paramExciter_14;
	StereoWidenerParam		paramStereoWidener_15;
	EQParam		paramEQ_16;
	EQDRCParam		paramEQDRC_17;
	EQParam		paramEQ_18;
	LLCompressorParam		paramLLCompressor_19;
	PhaseControlParam		paramPhaseControl_20;
	VBParam		paramVB_21;
	VBClassicParam		paramVBClassic_22;
	EQDRCParam		paramEQDRC_23;
	EQParam		paramEQ_24;
}AudioEffectCommParamBuffContext;

typedef struct _AudioEffectAECParam_HFPBuffContext
{
	ExpanderParam		paramExpander_0;
	EQParam		paramEQ_1;
	AECParam		paramAEC_2;
	GainControlParam		paramGainControl_3;
	DRCParam		paramDRC_4;
}AudioEffectAECParam_HFPBuffContext;


#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__AUDIO_EFFECT_PARAM_H__

