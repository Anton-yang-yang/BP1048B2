#include <string.h>
#include <nds32_intrinsic.h>
#include <stdlib.h>
#include "math.h"
#include "debug.h"
#include "app_config.h"
#include "audio_effect_api.h"
#include "audio_effect_library.h"
#include "audio_effect.h"
#include "ctrlvars.h"
#include "comm_param.h"
#include "audio_effect_user.h"
#include "eq_params.h"
#include "main_task.h"


ControlVariablesUserContext gCtrlUserVars;

#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
EQDRCUnit 			*music_trebbass_eq_unit = NULL;//trab/bass
#endif
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
EQUnit 				*music_mode_eq_unit = NULL;//eq mode
#endif
#ifdef CFG_FUNC_MIC_AUTOTUNE_STEP_EN
AutoTuneUnit 		*mic_autotune_unit = NULL;//
#endif
#ifdef CFG_FUNC_MIC_CHORUS_STEP_EN
ChorusUnit	 		*mic_chorus_unit = NULL;//
#endif
#ifdef CFG_FUNC_MIC_ECHO_STEP_EN
EchoUnit 			*mic_echo_unit = NULL;
#endif
#ifdef CFG_FUNC_MIC_REVERB_STEP_EN
ReverbUnit			*mic_reverb_unit = NULL;
#endif
#ifdef CFG_FUNC_MIC_PLATE_REVERB_STEP_EN
PlateReverbUnit 	*mic_platereverb_unit = NULL;
#endif
#ifdef CFG_FUNC_MIC_REVERB_PRO_STEP_EN
ReverbProUnit 		*mic_reverbpro_unit = NULL;
#endif

#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN

// EQ 参数加载函数
void LoadEqMode(const uint8_t *buff)
{
	if(music_mode_eq_unit == NULL)
	{
		return;
	}
	memcpy(&music_mode_eq_unit->param, buff, EQ_PARAM_LEN);
	AudioEffectEQFilterConfig(music_mode_eq_unit, gCtrlVars.sample_rate);
	gCtrlVars.AutoRefresh = 1;//////调音时模式发生改变，上位机会自动读取音效数据，1=允许上位读，0=不需要上位机读取
}

//EQ Mode调节函数
void EqModeSet(uint8_t EqMode)
{
    switch(EqMode)
	{
		case EQ_MODE_FLAT:
			LoadEqMode(&Flat[0]);
			break;
		case EQ_MODE_CLASSIC:
			LoadEqMode(&Classical[0]);
			break;
		case EQ_MODE_POP:
			LoadEqMode(&Pop[0]);
			break;
		case EQ_MODE_ROCK:
			LoadEqMode(&Rock[0]);
			break;
		case EQ_MODE_JAZZ:
			LoadEqMode(&Jazz[0]);
			break;
		case EQ_MODE_VOCAL_BOOST:
			LoadEqMode(&Vocal_Booster[0]);
			break;
		default:
			break;
	}
}
#endif

//music treb,bass调节函数
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
extern const int16_t BassTrebGainTable[16];
void MusicBassTrebAjust(int16_t 	BassGain,	int16_t TrebGain)
{
	if(music_trebbass_eq_unit==NULL)
	{
		DBG("Tone Var is Null\n");
		return;
	}
	music_trebbass_eq_unit->param.param_eq.eq_params[0].gain =  BassTrebGainTable[BassGain];
	music_trebbass_eq_unit->param.param_eq.eq_params[1].gain =  BassTrebGainTable[TrebGain];
#if	CFG_AUDIO_EFFECT_EQDRC_EN
	AudioEffectEQDRCConfig(music_trebbass_eq_unit, gCtrlVars.sample_rate);
#endif
	gCtrlVars.AutoRefresh = 1;
}
#endif

//music treb,bass调节函数
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
#if CFG_AUDIO_EFFECT_EQ_EN
extern const int16_t BassTrebGainTable[16];
void MicBassTrebAjust(int16_t BassGain,	int16_t TrebGain)
{
	if(music_trebbass_eq_unit == NULL)
	{
		DBG("Tone Var is Null\n");
		return;
	}
	music_trebbass_eq_unit->param.param_eq.eq_params[0].gain =  BassTrebGainTable[BassGain];
	music_trebbass_eq_unit->param.param_eq.eq_params[1].gain =  BassTrebGainTable[TrebGain];
#if	CFG_AUDIO_EFFECT_EQDRC_EN
	AudioEffectEQDRCConfig(music_trebbass_eq_unit, gCtrlVars.sample_rate);
#endif
	gCtrlVars.AutoRefresh = 1;
}
#endif
#endif

#ifdef CFG_FUNC_MIC_AUTOTUNE_STEP_EN
#if CFG_AUDIO_EFFECT_AUTO_TUNE_EN
void AutoTuneStepSet(uint8_t AutoTuneStep)
{
	if(mic_autotune_unit == NULL)
	{
		return;
	}
	mic_autotune_unit->param.key = AutoTuneKeyTbl[AutoTuneStep];
	AudioEffectAutoTuneInit(mic_autotune_unit, mic_autotune_unit->channel, gCtrlVars.sample_rate);

	gCtrlVars.AutoRefresh = 1;
}
#endif
#endif

#ifdef CFG_FUNC_MIC_CHORUS_STEP_EN
#if CFG_AUDIO_EFFECT_CHORUS_EN
void ChorusStepSet(uint8_t ChorusStep)
{
	uint16_t step;
	if(mic_chorus_unit == NULL)
	{
		return;
	}
    step = gCtrlUserVars.max_chorus_wet / MAX_MIC_EFFECT_DELAY_STEP;
	if(ChorusStep >= (MAX_MIC_EFFECT_DELAY_STEP-1))
	{
		mic_chorus_unit->param.wet = gCtrlUserVars.max_chorus_wet;
	}
	else
	{
		mic_chorus_unit->param.wet = ChorusStep * step;
	}
}
#endif
#endif

#ifdef CFG_FUNC_MIC_ECHO_STEP_EN
#if CFG_AUDIO_EFFECT_ECHO_EN
void EchoStepSet(uint8_t EchoStep)
{
	uint16_t step;

	if(mic_echo_unit == NULL)
	{
		return;
	}
	step = gCtrlUserVars.max_echo_delay / MAX_MIC_EFFECT_DELAY_STEP;
	if(EchoStep >= (MAX_MIC_EFFECT_DELAY_STEP-1))
	{
		//gCtrlVars.echo_unit.delay_samples = gCtrlUserVars.max_echo_delay;
		mic_echo_unit->param.delay = gCtrlUserVars.max_echo_delay;
	}
	else
	{
		//gCtrlVars.echo_unit.delay_samples = EchoStep * step;
		mic_echo_unit->param.delay = EchoStep * step;
	}
	step = gCtrlUserVars.max_echo_depth/ MAX_MIC_EFFECT_DELAY_STEP;
	if(EchoStep >= (MAX_MIC_EFFECT_DELAY_STEP-1))
	{
		mic_echo_unit->param.attenuation = gCtrlUserVars.max_echo_depth;
	}
	else
	{
		mic_echo_unit->param.attenuation = EchoStep * step;
	}

	gCtrlVars.AutoRefresh = 1;
}
#endif
#endif

#ifdef CFG_FUNC_MIC_REVERB_STEP_EN
#if CFG_AUDIO_EFFECT_REVERB_EN
void ReverbStepSet(uint8_t ReverbStep)
{
	uint16_t step;

	if(mic_reverb_unit == NULL)
	{
		return;
	}
    step = gCtrlUserVars.max_reverb_wet_scale/ MAX_MIC_EFFECT_DELAY_STEP;
	if(ReverbStep >= (MAX_MIC_EFFECT_DELAY_STEP-1))
	{
		mic_reverb_unit->param.wet_scale = gCtrlUserVars.max_reverb_wet_scale;
	}
	else
	{
		mic_reverb_unit->param.wet_scale = ReverbStep * step;
	}
    step = gCtrlUserVars.max_reverb_roomsize/ MAX_MIC_EFFECT_DELAY_STEP;
	if(ReverbStep >= (MAX_MIC_EFFECT_DELAY_STEP-1))
	{
		mic_reverb_unit->param.roomsize_scale = gCtrlUserVars.max_reverb_roomsize;
	}
	else
	{
		mic_reverb_unit->param.roomsize_scale = ReverbStep * step;
	}

	AudioEffectReverbConfig(mic_reverb_unit);

	gCtrlVars.AutoRefresh = 1;
}
#endif
#endif

#ifdef CFG_FUNC_MIC_PLATE_REVERB_STEP_EN
#if CFG_AUDIO_EFFECT_PLATE_REVERB_EN
void PlateReverbStepSet(uint8_t ReverbStep)
{
	uint16_t step;

	if(mic_platereverb_unit == NULL)
	{
		return;
	}
	step = gCtrlUserVars.max_plate_reverb_roomsize / MAX_MIC_EFFECT_DELAY_STEP;
	if(ReverbStep >= (MAX_MIC_EFFECT_DELAY_STEP-1))
	{
		mic_platereverb_unit->param.decay = gCtrlUserVars.max_plate_reverb_roomsize;
	}
	else
	{
		mic_platereverb_unit->param.decay = ReverbStep * step;
	}
	//APP_DBG("mic_wetdrymix   = %d\n",gCtrlVars.max_plate_reverb_wetdrymix);
	step = gCtrlUserVars.max_plate_reverb_wetdrymix / MAX_MIC_EFFECT_DELAY_STEP;
	if(ReverbStep >= (MAX_MIC_EFFECT_DELAY_STEP-1))
	{
		mic_platereverb_unit->param.wetdrymix = gCtrlUserVars.max_plate_reverb_wetdrymix;
	}
	else
	{
		mic_platereverb_unit->param.wetdrymix = ReverbStep * step;
	}

	AudioEffectPlateReverbConfig(mic_platereverb_unit);

	gCtrlVars.AutoRefresh = 1;
}
#endif
#endif

#ifdef CFG_FUNC_MIC_REVERB_PRO_STEP_EN
#if CFG_AUDIO_EFFECT_REVERB_PRO_EN
void ReverbProStepSet(uint8_t ReverbStep)
{
	uint16_t step,r;

	if(mic_reverbpro_unit == NULL)
	{
		return;
	}

	//+0  ~~~ -70
    r = abs(gCtrlUserVars.max_reverb_pro_wet);
    r = 70-r;
    step = r / MAX_MIC_EFFECT_DELAY_STEP;

	if(ReverbStep >= (MAX_MIC_EFFECT_DELAY_STEP-1))
	{
		mic_reverbpro_unit->param.wet = gCtrlUserVars.max_reverb_pro_wet;
	}
	else
	{
		r = MAX_MIC_EFFECT_DELAY_STEP - 1 - ReverbStep;
		r*= step;
		mic_reverbpro_unit->param.wet = gCtrlUserVars.max_reverb_pro_wet - r;

		if(ReverbStep == 0) mic_reverbpro_unit->param.wet = -70;
	}

    r = abs(gCtrlUserVars.max_reverb_pro_erwet);
    r = 70-r;
    step = r / MAX_MIC_EFFECT_DELAY_STEP;

	if(ReverbStep >= (MAX_MIC_EFFECT_DELAY_STEP-1))
	{
		mic_reverbpro_unit->param.erwet = gCtrlUserVars.max_reverb_pro_erwet;
	}
	else
	{
		r = MAX_MIC_EFFECT_DELAY_STEP - 1 - ReverbStep;
		r*= step;
		mic_reverbpro_unit->param.erwet = gCtrlUserVars.max_reverb_pro_erwet - r;

		if(ReverbStep == 0) mic_reverbpro_unit->param.erwet = -70;
	}

	AudioEffectReverbProInit(mic_reverbpro_unit, mic_reverbpro_unit->channel, gCtrlVars.sample_rate);

	gCtrlVars.AutoRefresh = 1;
}
#endif
#endif

#ifdef CFG_FUNC_MIC_EFFECT_DELAY_EN
void AudioEffectDelaySet(int16_t step)
{
	#ifdef CFG_FUNC_MIC_CHORUS_STEP_EN
	ChorusStepSet(step);
	#endif
	#ifdef CFG_FUNC_MIC_ECHO_STEP_EN
	EchoStepSet(step);
	#endif
	#ifdef CFG_FUNC_MIC_REVERB_STEP_EN
	ReverbStepSet(step);
	#endif
	#ifdef CFG_FUNC_MIC_PLATE_REVERB_STEP_EN
	PlateReverbStepSet(step);
	#endif
	#ifdef CFG_FUNC_MIC_REVERB_PRO_STEP_EN
	ReverbProStepSet(step);
	#endif
}
#endif


//用户手动控制音效指针赋初值为NULL
//音效切换后都需要重新清零一遍
void AudioEffectUserNodeInit(void)
{
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
	music_trebbass_eq_unit = NULL;//trab/bass
#endif
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
	music_mode_eq_unit = NULL;//eq mode
#endif
#ifdef CFG_FUNC_MIC_AUTOTUNE_STEP_EN
	mic_autotune_unit = NULL;//
#endif
#ifdef CFG_FUNC_MIC_CHORUS_STEP_EN
	mic_chorus_unit = NULL;//
#endif
#ifdef CFG_FUNC_MIC_ECHO_STEP_EN
 	mic_echo_unit = NULL;
#endif
#ifdef CFG_FUNC_MIC_REVERB_STEP_EN
	mic_reverb_unit = NULL;
#endif
#ifdef CFG_FUNC_MIC_PLATE_REVERB_STEP_EN
	mic_platereverb_unit = NULL;
#endif
#ifdef CFG_FUNC_MIC_REVERB_PRO_STEP_EN
	mic_reverbpro_unit = NULL;
#endif
}

void DefaultParamgsUserInit(void)
{
//#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
//	mainAppCt.EqMode = EQ_MODE_FLAT;
//#endif
//#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
//	mainAppCt.MicBassStep = 7;
//	mainAppCt.MicTrebStep = 7;
//#endif
//#ifdef CFG_FUNC_MIC_AUTOTUNE_STEP_EN
//	mainAppCt.MicAutoTuneStep = 2;
//#endif
//
//#ifdef CFG_FUNC_MIC_EFFECT_DELAY_EN
//	mainAppCt.MicEffectDelayStep = MAX_MIC_EFFECT_DELAY_STEP;
//#endif
}

//用户相关音效调节参数同步函数
//调用AudioEffectModeSel()设置音效模式之后，需要再调用下此函数，保证用户设置音效参数同步

void AudioEffectParamSync(void)
{
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
	EqModeSet(mainAppCt.EqMode);
#endif
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
	MicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
#endif
#ifdef CFG_FUNC_MIC_AUTOTUNE_STEP_EN
	AutoTuneStepSet(mainAppCt.MicAutoTuneStep);
#endif
#ifdef CFG_FUNC_MIC_EFFECT_DELAY_EN
	AudioEffectDelaySet(mainAppCt.MicEffectDelayStep);
#endif
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	{
		extern bool IsEffectChange;
		IsEffectChange = 0;//EQ模式切换中或EQ模式参数更新，不需要再做音效及内存初始化处理
	}
#endif
}
