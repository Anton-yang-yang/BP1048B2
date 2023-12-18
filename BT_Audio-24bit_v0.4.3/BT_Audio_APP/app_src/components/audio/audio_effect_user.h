#ifndef __AUDIO_EFFECT_USER_H__
#define __AUDIO_EFFECT_USER_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"
#include "audio_effect_api.h"
#include "audio_effect.h"


//#define CFG_FUNC_MUSIC_EQ_MODE_EN
//#define CFG_FUNC_MUSIC_TREB_BASS_EN
//#define CFG_FUNC_MIC_AUTOTUNE_STEP_EN
//#define CFG_FUNC_MIC_EFFECT_DELAY_EN
//
//以上4个宏SDK中定义到app_config.h头文件中，如果没有则定义在此文件中

#ifdef CFG_FUNC_MIC_EFFECT_DELAY_EN
	#define CFG_FUNC_MIC_CHORUS_STEP_EN
	#define CFG_FUNC_MIC_ECHO_STEP_EN
	#define CFG_FUNC_MIC_REVERB_STEP_EN
	#define CFG_FUNC_MIC_PLATE_REVERB_STEP_EN
	#define CFG_FUNC_MIC_REVERB_PRO_STEP_EN
#endif

extern EQDRCUnit		*music_trebbass_eq_unit;
extern EQUnit 			*music_mode_eq_unit;
extern AutoTuneUnit 	*mic_autotune_unit;
extern ChorusUnit	 	*mic_chorus_unit;
extern EchoUnit 		*mic_echo_unit;
extern ReverbUnit		*mic_reverb_unit;
extern PlateReverbUnit 	*mic_platereverb_unit;
extern ReverbProUnit 	*mic_reverbpro_unit;

typedef struct _ControlVariablesUserContext
{
//#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
//	uint8_t		EqMode;
//#endif
//#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
//	int16_t 	MicBassStep;
//	int16_t 	MicTrebStep;
//#endif
//#ifdef CFG_FUNC_MIC_AUTOTUNE_STEP_EN
//	int16_t 	MicAutoTuneStep;
//#endif
//
//#ifdef CFG_FUNC_MIC_EFFECT_DELAY_EN
//	int16_t 	MicEffectDelayStep;

	//------音效参数的最大值 来自调音参数数据-------------------------------//
#ifdef CFG_FUNC_MIC_CHORUS_STEP_EN
	uint16_t 	max_chorus_wet;
#endif
#ifdef CFG_FUNC_MIC_ECHO_STEP_EN
	int16_t		max_echo_delay;
	int16_t		max_echo_depth;
#endif
#ifdef CFG_FUNC_MIC_REVERB_STEP_EN
	int16_t 	max_reverb_wet_scale;
	int16_t 	max_reverb_roomsize;
#endif
#ifdef CFG_FUNC_MIC_PLATE_REVERB_STEP_EN
	int16_t		max_plate_reverb_roomsize;
	int16_t		max_plate_reverb_wetdrymix;
#endif
#ifdef CFG_FUNC_MIC_REVERB_PRO_STEP_EN
	int16_t		max_reverb_pro_wet;
	int16_t		max_reverb_pro_erwet;
#endif
//#endif
}ControlVariablesUserContext;


extern ControlVariablesUserContext gCtrlUserVars;

void EqModeSet(uint8_t EqMode);
void MicBassTrebAjust(int16_t BassGain,	int16_t TrebGain);
void AutoTuneStepSet(uint8_t AutoTuneStep);
void AudioEffectDelaySet(int16_t step);
void DefaultParamgsUserInit(void);
void AudioEffectUserNodeInit(void);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__AUDIO_EFFECT_USER_H__
