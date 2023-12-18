#include <string.h>
#include <nds32_intrinsic.h>
#include "debug.h"
#include "app_config.h"
#include "ctrlvars.h"
#include "watchdog.h"
#include "rtos_api.h"
#include "audio_effect_api.h"
#include "audio_effect_library.h"
#include "communication.h"
#include "audio_adc.h"
#include "main_task.h"
#include "math.h"
#include "bt_config.h"
#include "bt_hf_api.h"
#include "bt_manager.h"
#include "audio_vol.h"
#include "tws_mode.h"
#include "stdlib.h"
#include "eq.h"
#include "blue_aec.h"
#include "overdrive.h"
#include "distortion_exp.h"
#include "eq_drc.h"

#ifdef BT_TWS_SUPPORT
#include "bt_tws_api.h"
#endif

#ifdef CFG_FUNC_AUDIO_EFFECT_EN

#ifdef FUNC_OS_EN
osMutexId AudioEffectMutex = NULL;
#endif

#ifdef CFG_FUNC_ECHO_DENOISE
int16_t*  EchoAudioBuf=NULL;
#endif


#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
int16_t EqModeAudioBuf[512*2];
EQContext EqBufferBak;
uint32_t music_eq_mode_unit = NULL;
#endif

#if CFG_AUDIO_EFFECT_AUTO_TUNE_EN
void AudioEffectAutoTuneInit(AutoTuneUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	uint16_t SamplesPreFrame = 256;
	if(unit->enable == FALSE)
	{
		DBG("AutoTune invalid\n");
		return;
	}
	unit->channel = channel;
	auto_tune_init(&unit->ct, channel, sample_rate, unit->param.key, SamplesPreFrame);
}

void AudioEffectAutoTuneApply(AutoTuneUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	auto_tune_apply(&unit->ct, pcm_in, pcm_out, unit->param.snap_mode, unit->param.key, unit->param.pitch_accuracy);
}
#endif

#if CFG_AUDIO_EFFECT_DRC_EN
void AudioEffectDRCInit(DRCUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	//位宽对齐
	int32_t fc[2] = {unit->param.fc[0], unit->param.fc[1]};
	int32_t threshold[4] = {unit->param.threshold[0], unit->param.threshold[1], unit->param.threshold[2], unit->param.threshold[3]};
	int32_t ratio[4] = {unit->param.ratio[0], unit->param.ratio[1], unit->param.ratio[2], unit->param.ratio[3]};
	int32_t attack_tc[4] = {unit->param.attack_tc[0], unit->param.attack_tc[1], unit->param.attack_tc[2], unit->param.attack_tc[3]};
	int32_t release_tc[4] = {unit->param.release_tc[0], unit->param.release_tc[1], unit->param.release_tc[2], unit->param.release_tc[3]};

	if(unit->enable == FALSE)
	{
		DBG("DRC invalid\n");
		return;
	}
	unit->channel = channel;
	//drc_init(&unit->ct, channel, sample_rate, unit->param.fc, unit->param.mode, unit->param.q, threshold, ratio, attack_tc, release_tc);
	//drc_init(&unit->ct, unit->channel, sample_rate, unit->param.mode, unit->param.cf_type, unit->param.q_l, unit->param.q_h,unit->param.fc,unit->param.threshold, unit->param.ratio, unit->param.attack_tc, unit->param.release_tc);
	drc_init(&unit->ct, unit->channel, sample_rate, unit->param.mode, unit->param.cf_type, unit->param.q_l, unit->param.q_h, fc, threshold, ratio, attack_tc, release_tc);
	DBG("DRC INIT OK\n");
}

void AudioEffectDRCApply(DRCUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	int32_t pregain[4] = {unit->param.pregain[0], unit->param.pregain[1], unit->param.pregain[2], unit->param.pregain[3]};
	if(unit->enable == FALSE)
	{
		return;
	}
	drc_apply(&unit->ct, pcm_in, pcm_out, n, pregain);
}

#ifdef CFG_AUDIO_WIDTH_24BIT
void AudioEffectDRCApply24(DRCUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	int32_t pregain[4] = {unit->param.pregain[0], unit->param.pregain[1], unit->param.pregain[2], unit->param.pregain[3]};
	if(unit->enable == FALSE)
	{
		return;
	}
	drc_apply24(&unit->ct, pcm_in, pcm_out, n, pregain);
}
#endif
#endif

#if CFG_AUDIO_EFFECT_ECHO_EN
void AudioEffectEchoInit(EchoUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	uint32_t max_delay_samples;
	if(unit->enable == FALSE)
	{
		return;
	}
	unit->channel = channel;
	max_delay_samples = unit->param.max_delay * gCtrlVars.sample_rate/1000;
	echo_init(&unit->ct,  channel, sample_rate, unit->param.fc, max_delay_samples, unit->param.high_quality_enable, unit->buf);
}

void AudioEffectEchoApply(EchoUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	uint32_t delay_samples;
	if(unit->enable == FALSE)
	{
		return;
	}
	delay_samples = unit->param.delay * gCtrlVars.sample_rate/1000;
	echo_apply(&unit->ct, pcm_in, pcm_out, n, unit->param.attenuation, delay_samples, unit->param.dry, unit->param.wet);
}
#endif

#if CFG_AUDIO_EFFECT_EQ_EN
EQFilterParams tempEqParam [10];
void AudioEffectEQInit(EQUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	uint32_t i, filter_count = 0;;
	int16_t *eq_pp = NULL;//&pnode->efft_param[2]
	if(unit->enable == FALSE)
	{
		return;
	}
	eq_pp = (int16_t *)unit->param.eq_params;

	memset(tempEqParam, 0x00, sizeof(tempEqParam));
	for(i=0; i<10; i++, eq_pp+=5)
	{
		if(eq_pp[0])
		{
			tempEqParam[filter_count].type = eq_pp[1];
			tempEqParam[filter_count].f0 = eq_pp[2];
			tempEqParam[filter_count].Q = eq_pp[3];
			tempEqParam[filter_count].gain = eq_pp[4];
			filter_count ++;
		}
	}
	unit->channel = channel;
	//unit->param.filter_count = filter_count;
	eq_clear_delay_buffer(&unit->ct);
	if(unit->param.calculation_type)
	{
		eq_init_float(&unit->ct, sample_rate, tempEqParam, filter_count, unit->param.pregain, unit->channel);
	}
	else
	{
		eq_init(&unit->ct, sample_rate, tempEqParam, filter_count, unit->param.pregain, unit->channel);
	}
}

void AudioEffectEQPregainConfig(EQUnit *unit)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	eq_clear_delay_buffer(&unit->ct);
	eq_configure_pregain(&unit->ct, (int32_t)unit->param.pregain);
}

void AudioEffectEQFilterConfig(EQUnit *unit, uint32_t sample_rate)
{
	uint32_t i, filter_count = 0;;
	int16_t *eq_pp = NULL;

	if(unit->enable == FALSE)
	{
		return;
	}
	eq_pp = (int16_t *)unit->param.eq_params;

	for(i=0; i<10; i++, eq_pp+=5)
	{
		if(eq_pp[0])
		{
			tempEqParam[filter_count].type = eq_pp[1];
			tempEqParam[filter_count].f0 = eq_pp[2];
			tempEqParam[filter_count].Q = eq_pp[3];
			tempEqParam[filter_count].gain = eq_pp[4];
			filter_count ++;
		}
	}
	//unit->param.filter_count = filter_count;
	eq_configure_filters(&unit->ct, sample_rate, tempEqParam, filter_count);
}

void AudioEffectEQApply(EQUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN		
	uint32_t i=0;
	uint8_t channel;
	uint32_t cnt = n / EQ_BUFFER_NUM_SAMPLES;

	if(music_eq_mode_unit == unit)
	{
		if(mainAppCt.EqModeBak != mainAppCt.EqMode)
		{
			channel = unit->channel;
			memcpy(EqModeAudioBuf, pcm_in, n * 2 * channel);
			for(i = 0; i < cnt; i++)
			{
				eq_apply(&unit->ct, (int16_t *)(pcm_in + i * EQ_BUFFER_NUM_SAMPLES * channel), (int16_t *)(pcm_out + i * EQ_BUFFER_NUM_SAMPLES * channel), EQ_BUFFER_NUM_SAMPLES);
			}
			//if(RemainLen > 0)		//此处需要采样帧必须为128时有作用
			//{
			//	eq_apply(unit->ct, (int16_t *)(pcm_in + i * EQ_BUFFER_NUM_SAMPLES * channel), (int16_t *)(pcm_out + i * EQ_BUFFER_NUM_SAMPLES * channel), RemainLen);
			//}
			du_efft_fadeout_sw(pcm_out, n, channel);
			#ifdef FUNC_OS_EN
			osMutexUnlock(AudioEffectMutex);
			#endif
			EqModeSet(mainAppCt.EqMode);
			#ifdef FUNC_OS_EN
			osMutexLock(AudioEffectMutex);
		    #endif
			for(i = 0; i < cnt; i++)
			{
				eq_apply(&unit->ct, (int16_t *)(EqModeAudioBuf + i * EQ_BUFFER_NUM_SAMPLES * channel), (int16_t *)(EqModeAudioBuf + i * EQ_BUFFER_NUM_SAMPLES * channel), EQ_BUFFER_NUM_SAMPLES);
			}
			//if(RemainLen > 0)		//此处需要采样帧必须为128时有作用
			//{
			//	eq_apply(unit->ct, (int16_t *)(EqModeAudioBuf + i * EQ_BUFFER_NUM_SAMPLES * channel), (int16_t *)(EqModeAudioBuf + i * EQ_BUFFER_NUM_SAMPLES * channel), RemainLen);
			//}
			du_efft_fadein_sw((int16_t *)EqModeAudioBuf, n, channel);
			for(i = 0; i < n; i++)
			{
				pcm_out[2*i + 0] = __nds32__clips(((int32_t)pcm_out[2*i + 0] + (int32_t)EqModeAudioBuf[2*i + 0]), 16-1);
				pcm_out[2*i + 1] = __nds32__clips(((int32_t)pcm_out[2*i + 1] + (int32_t)EqModeAudioBuf[2*i + 1]), 16-1);
			}
			mainAppCt.EqModeBak = mainAppCt.EqMode;
			return;
		}
	}
#endif
	eq_apply(&unit->ct, pcm_in, pcm_out, n);
}

#ifdef CFG_AUDIO_WIDTH_24BIT
void AudioEffectEQApply24(EQUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
	uint32_t i=0;
	uint8_t channel;
	uint32_t cnt = n / EQ_BUFFER_NUM_SAMPLES;

	if(music_eq_mode_unit == unit)
	{
		if(mainAppCt.EqModeBak != mainAppCt.EqMode)
		{
			channel = unit->channel;
			memcpy(EqModeAudioBuf, pcm_in, n * 2 * channel);
			for(i = 0; i < cnt; i++)
			{
				eq_apply(&unit->ct, (int16_t *)(pcm_in + i * EQ_BUFFER_NUM_SAMPLES * channel), (int16_t *)(pcm_out + i * EQ_BUFFER_NUM_SAMPLES * channel), EQ_BUFFER_NUM_SAMPLES);
			}
			//if(RemainLen > 0)		//此处需要采样帧必须为128时有作用
			//{
			//	eq_apply(unit->ct, (int16_t *)(pcm_in + i * EQ_BUFFER_NUM_SAMPLES * channel), (int16_t *)(pcm_out + i * EQ_BUFFER_NUM_SAMPLES * channel), RemainLen);
			//}
			du_efft_fadeout_sw(pcm_out, n, channel);
			#ifdef FUNC_OS_EN
			osMutexUnlock(AudioEffectMutex);
			#endif
			EqModeSet(mainAppCt.EqMode);
			#ifdef FUNC_OS_EN
			osMutexLock(AudioEffectMutex);
		    #endif
			for(i = 0; i < cnt; i++)
			{
				eq_apply(&unit->ct, (int16_t *)(EqModeAudioBuf + i * EQ_BUFFER_NUM_SAMPLES * channel), (int16_t *)(EqModeAudioBuf + i * EQ_BUFFER_NUM_SAMPLES * channel), EQ_BUFFER_NUM_SAMPLES);
			}
			//if(RemainLen > 0)		//此处需要采样帧必须为128时有作用
			//{
			//	eq_apply(unit->ct, (int16_t *)(EqModeAudioBuf + i * EQ_BUFFER_NUM_SAMPLES * channel), (int16_t *)(EqModeAudioBuf + i * EQ_BUFFER_NUM_SAMPLES * channel), RemainLen);
			//}
			du_efft_fadein_sw((int16_t *)EqModeAudioBuf, n, channel);
			for(i = 0; i < n; i++)
			{
				pcm_out[2*i + 0] = __nds32__clips(((int32_t)pcm_out[2*i + 0] + (int32_t)EqModeAudioBuf[2*i + 0]), 16-1);
				pcm_out[2*i + 1] = __nds32__clips(((int32_t)pcm_out[2*i + 1] + (int32_t)EqModeAudioBuf[2*i + 1]), 16-1);
			}
			mainAppCt.EqModeBak = mainAppCt.EqMode;
			return;
		}
	}
#endif
	eq_apply24(&unit->ct, pcm_in, pcm_out, n, 1);
}
#endif
#endif

#if CFG_AUDIO_EFFECT_NOISE_SUPPRESSOR_EN
void AudioEffectExpanderInit(ExpanderUnit *unit, uint8_t channel, uint32_t sample_rate)
{
    if(unit->enable == FALSE)
	{
		return;
	}
    unit->channel = channel;
	expander_init(&unit->ct,  channel, sample_rate, unit->param.threshold, unit->param.ratio, unit->param.attack_time, unit->param.release_time);
}

void AudioEffectExpanderThresholdConfig(ExpanderUnit *unit)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	expander_set_threshold(&unit->ct,  unit->param.threshold);
}

void AudioEffectExpanderApply(ExpanderUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	expander_apply(&unit->ct, pcm_in, pcm_out, n);
}

#ifdef CFG_AUDIO_WIDTH_24BIT
void AudioEffectExpanderApply24(ExpanderUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	expander_apply24(&unit->ct, pcm_in, pcm_out, n);
}
#endif
#endif

#if CFG_AUDIO_EFFECT_FREQ_SHIFTER_EN
void AudioEffectFreqShifterInit(FreqShifterUnit *unit)
{
	if(unit->enable == FALSE)
	{
		DBG("FreqShifter invalid\n");
		return;
	}
	freqshifter_init(&unit->ct,DeltafTable[unit->param.deltaf]);
}

void AudioEffectFreqShifterApply(FreqShifterUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	freqshifter_apply(&unit->ct, pcm_in, pcm_out, n);
}
#endif

#if CFG_AUDIO_EFFECT_HOWLING_SUPPRESSOR_EN
void AudioEffectHowlingSuppressorInit(HowlingUnit *unit)
{
	if(unit->enable == FALSE)
	{
		DBG("HowlingSuppressor invalid\n");
		return;
	}
	howling_suppressor_init(&unit->ct, unit->param.suppression_mode);
}

void AudioEffectHowlingSuppressorApply(HowlingUnit *unit, int16_t *pcm_in, int16_t *pcm_out, int32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	howling_suppressor_apply(&unit->ct,  pcm_in, pcm_out, n);
}
#endif

#if CFG_AUDIO_EFFECT_PITCH_SHIFTER_EN
#define CFG_MIC_PITCH_SHIFTER_FRAME_SIZE               (256)// (512)  //unit in sample
void AudioEffectPitchShifterInit(PitchShifterUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("PitchShifter invalid\n");
		return;
	}
	unit->channel = channel;
	pitch_shifter_init(&unit->ct, channel, sample_rate, unit->param.semitone_steps, CFG_MIC_PITCH_SHIFTER_FRAME_SIZE);//512
}
void AudioEffectPitchShifterConfig(PitchShifterUnit *unit)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	pitch_shifter_configure(&unit->ct, unit->param.semitone_steps);
}
void AudioEffectPitchShifterApply(PitchShifterUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}

	{
		uint32_t PSSample = (CFG_MIC_PITCH_SHIFTER_FRAME_SIZE >> 1);
		uint32_t Cnt = n / PSSample;
		uint16_t iIdx;

		for(iIdx = 0; iIdx < Cnt; iIdx++)
		{
			pitch_shifter_apply(&unit->ct, (int16_t *)(pcm_in  + (PSSample * unit->channel) * iIdx),
									  	  (int16_t *)(pcm_out + (PSSample * unit->channel) * iIdx));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_REVERB_EN
void AudioEffectReverbInit(ReverbUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("Reverb invalid\n");
		return;
	}
	unit->channel = channel;
	reverb_init(&unit->ct, channel, sample_rate);
	reverb_configure(&unit->ct, unit->param.dry_scale, unit->param.wet_scale, unit->param.width_scale, unit->param.roomsize_scale, unit->param.damping_scale);
}

void AudioEffectReverbConfig(ReverbUnit *unit)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	reverb_configure(&unit->ct, unit->param.dry_scale, unit->param.wet_scale, unit->param.width_scale, unit->param.roomsize_scale, unit->param.damping_scale);
}

void AudioEffectReverbApply(ReverbUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	reverb_apply(&unit->ct, pcm_in, pcm_out, n);
}
#endif

#if CFG_AUDIO_EFFECT_SILENCE_DECTOR_EN
void AudioEffectSilenceDetectorInit(SilenceDetectorUnit *unit, uint8_t channel, uint32_t sample_rate)
{
    if(unit->enable == FALSE)
	{
    	DBG("SilenceDetector invalid\n");
		return;
	}
    unit->channel = channel;
    silence_detector_init(&unit->ct,  channel, sample_rate);
}
//pcm_out实际上没有用，仅仅为了代码对齐
void AudioEffectSilenceDetectorApply(SilenceDetectorUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	unit->param.level = silence_detector_apply(&unit->ct, pcm_in, n);
	
#ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN
	//DBG("unit->param.level = %d\n", unit->param.level);
    if(unit->param.level > SILENCE_THRESHOLD)
    {
        mainAppCt.Silence_Power_Off_Time = 0;
    }
#endif
}

#ifdef CFG_AUDIO_WIDTH_24BIT
//pcm_out实际上没有用，仅仅为了代码对齐
void AudioEffectSilenceDetectorApply24(SilenceDetectorUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	uint32_t level;
	if(unit->enable == FALSE)
	{
		return;
	}
	level = silence_detector_apply24(&unit->ct, pcm_in, n);

	unit->param.level = (level>>8);

#ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN
	//DBG("unit->param.level = %d\n", unit->param.level);
    if(unit->param.level > SILENCE_THRESHOLD)
    {
        mainAppCt.Silence_Power_Off_Time = 0;
    }
#endif
}
#endif
#endif

#if CFG_AUDIO_EFFECT_3D_EN
void AudioEffectThreeDInit(ThreeDUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("ThreeD invalid\n");
		return;
	}
	unit->channel = channel;
	three_d_init(&unit->ct, channel, sample_rate);
}

void AudioEffectThreeDApply(ThreeDUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	three_d_apply(&unit->ct, pcm_in, pcm_out, n, unit->param.intensity);
}
#ifdef CFG_AUDIO_WIDTH_24BIT
void AudioEffectThreeDApply24(ThreeDUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	three_d_apply24(&unit->ct, pcm_in, pcm_out, n, unit->param.intensity);
}
#endif
#endif

#if CFG_AUDIO_EFFECT_VB_EN
void AudioEffectVBInit(VBUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("VB invalid\n");
		return;
	}
	unit->channel = channel;
	vb_init(&unit->ct, channel, sample_rate, unit->param.f_cut);
}

void AudioEffectVBApply(VBUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
    {
    	return;
    }
	vb_apply(&unit->ct, pcm_in, pcm_out, n, unit->param.intensity, unit->param.enhanced);
}

#ifdef CFG_AUDIO_WIDTH_24BIT
void AudioEffectVBApply24(VBUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
    {
    	return;
    }
	vb_apply24(&unit->ct, pcm_in, pcm_out, n, unit->param.intensity, unit->param.enhanced);
}
#endif
#endif

#if CFG_AUDIO_EFFECT_VOICE_CHANGER_EN
void AudioEffectVoiceChangerInit(VoiceChangerUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("VoiceChanger invalid\n");
		return;
	}
	unit->channel = channel;
	voice_changer_init(&unit->ct, channel, sample_rate, unit->param.pitch_ratio, unit->param.formant_ratio);//512
}

void AudioEffectVoiceChangerApply(VoiceChangerUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	voice_changer_apply(&unit->ct, pcm_in, pcm_out);
}
#endif

#if CFG_AUDIO_EFFECT_GAIN_CONTROL_EN
void AudioEffectPregainInit(GainControlUnit *unit, uint8_t channel)
{
	if(unit->enable == FALSE)
	{
		DBG("Pregain invalid\n");
		return;
	}
	unit->channel = channel;
}

void AudioEffectPregainApply(GainControlUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	int32_t s;
	int32_t pregain;

	if(unit->enable)
	{
		pregain = unit->param.mute? 0 : unit->param.gain;
		for(s = 0; s < n; s++)
		{
			if(unit->channel == 2)
			{
				pcm_out[2 * s + 0] = __nds32__clips((((int32_t)pcm_in[2 * s + 0] * pregain + 2048) >> 12), 16-1);
				pcm_out[2 * s + 1] = __nds32__clips((((int32_t)pcm_in[2 * s + 1] * pregain + 2048) >> 12), 16-1);
			}
			else
			{
				pcm_out[s] = __nds32__clips((((int32_t)pcm_in[s] * pregain + 2048) >> 12), 16-1);
			}
		}
	}
}
#ifdef CFG_AUDIO_WIDTH_24BIT
void AudioEffectPregainApply24(GainControlUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	int32_t s;
	int32_t pregain;

	if(unit->enable)
	{
		pregain = unit->param.mute? 0 : unit->param.gain;
		for(s = 0; s < n; s++)
		{
			if(unit->channel == 2)
			{
				pcm_out[2 * s + 0] = __nds32__clips((((int64_t)pcm_in[2 * s + 0] * pregain + 2048) >> 12), 24-1);
				pcm_out[2 * s + 1] = __nds32__clips((((int64_t)pcm_in[2 * s + 1] * pregain + 2048) >> 12), 24-1);
			}
			else
			{
				pcm_out[s] = __nds32__clips((((int64_t)pcm_in[s] * pregain + 2048) >> 12), 24-1);
			}
		}
	}
}
#endif
#endif

#if CFG_AUDIO_EFFECT_VB_CLASS_EN
void AudioEffectVBClassInit(VBClassUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("VB Class invalid\n");
		return;
	}
	unit->channel = channel;
	vb_classic_init(&unit->ct, channel, sample_rate, unit->param.f_cut);
}

void AudioEffectVBClassApply(VBClassUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
    {
    	return;
    }
	vb_classic_apply(&unit->ct, pcm_in, pcm_out, n, unit->param.intensity);
}
#ifdef CFG_AUDIO_WIDTH_24BIT
void AudioEffectVBClassApply24(VBClassUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
    {
    	return;
    }
	vb_classic_apply24(&unit->ct, pcm_in, pcm_out, n, unit->param.intensity);
}
#endif
#endif

#if CFG_AUDIO_EFFECT_VOCAL_CUT_EN
void AudioEffectVocalCutInit(VocalCutUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("VocalCut invalid\n");
		return;
	}
	unit->channel = channel;
	vocal_cut_init(&unit->ct, sample_rate);
}
void AudioEffectVocalCutApply(VocalCutUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	vocal_cut_apply(&unit->ct, pcm_in, pcm_out, n, unit->param.wetdrymix);
}
#endif

#if CFG_AUDIO_EFFECT_PLATE_REVERB_EN
void AudioEffectPlateReverbInit(PlateReverbUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("PlateReverb invalid\n");
		return;
	}
	unit->channel = channel;
	plate_reverb_init(&unit->ct, channel, sample_rate, unit->param.highcut_freq, unit->param.modulation_en);
	plate_reverb_configure(&unit->ct, unit->param.predelay, unit->param.diffusion, unit->param.decay, unit->param.damping, unit->param.wetdrymix);
}
void AudioEffectPlateReverbConfig(PlateReverbUnit *unit)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	plate_reverb_configure(&unit->ct, unit->param.predelay, unit->param.diffusion, unit->param.decay, unit->param.damping, unit->param.wetdrymix);
}

void AudioEffectPlateReverbApply(PlateReverbUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	plate_reverb_apply(&unit->ct, pcm_in, pcm_out, n);
}
#endif

#if CFG_AUDIO_EFFECT_REVERB_PRO_EN
//#if defined(CFG_CHIP_BP1048P2)  || defined(CFG_CHIP_BP1048P4) ||defined(CFG_CHIP_BP1064A2)
void AudioEffectReverbProInit(ReverbProUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("ReverbPro invalid\n");
		return;
	}
	unit->channel = channel;
	reverb_pro_init(unit->ct, sample_rate, unit->param.dry, unit->param.wet, unit->param.erwet, unit->param.erfactor,
					unit->param.erwidth, unit->param.ertolate, unit->param.rt60, unit->param.delay, unit->param.width,unit->param.wander,
					unit->param.spin, unit->param.inputlpf, unit->param.damplpf, unit->param.basslpf, unit->param.bassb, unit->param.outputlpf);

}
void AudioEffectReverbProApply(ReverbProUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		DBG("ReverbPro invalid\n");
		return;
	}
	reverb_pro_apply(unit->ct, pcm_in, pcm_out, n);
}
#endif

#if CFG_AUDIO_EFFECT_VOICE_CHANGER_PRO_EN
//帧长只能为256或者128
void AudioEffectVoiceChangerProInit(VoiceChangerProUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("VoiceChanger Rro invalid\n");
		return;
	}
	unit->channel = channel;
	voice_changer_pro_init(&unit->ct, sample_rate, 256, unit->param.pitch_ratio, unit->param.formant_ratio);///256
}

void AudioEffectVoiceChangerProApply(VoiceChangerProUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	voice_changer_pro_apply(&unit->ct, pcm_in, pcm_out);
}
#endif

#if CFG_AUDIO_EFFECT_PHASE_CONTROL_EN
//相位偏差180°，apply一定是要单声道，故init的时候，channel选择为1
void AudioEffectPhaseInit(PhaseControlUnit *unit, uint8_t channel)
{
	if(unit->enable == FALSE)
	{
		DBG("Phase invalid\n");
		return;
	}
	unit->channel = channel;
}

void AudioEffectPhaseApply(PhaseControlUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	int32_t s;
	if(unit->enable == FALSE)
	{
		return;
	}
	if(unit->param.phase_difference)
	{
		for(s=0; s < n * unit->channel; s++)
		{
			pcm_in[s] = __nds32__clips(((int32_t)pcm_in[s] * (-1)), (16)-1);
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_VOCAL_REMOVE_EN
//这个音效默认被关闭，只有pro版本可以使用
void AudioEffectVocalRemoveInit(VocalRemoveUnit *unit,  uint8_t channel, uint32_t sample_rate)
{
	uint16_t step_size = 256;
	if(unit->enable == FALSE)
	{
		DBG("VocalRemove invalid\n");
		return;
	}
	unit->channel = channel;
	vocal_remover_init(&unit->ct, sample_rate, unit->param.lower_freq, unit->param.higher_freq, step_size);
}

void AudioEffectVocalRemoveApply(VocalRemoveUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
    vocal_remover_apply(&unit->ct, pcm_in, pcm_out);
}
#endif

#if CFG_AUDIO_EFFECT_PITCH_SHIFTER_PRO_EN
//消耗MCPS，建议使用512帧长，后续帧长配置需要做进一步的处理
void AudioEffectPitchShifterProInit(PitchShifterProUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	//需要修改参数
	if(unit->enable == FALSE)
	{
		DBG("PitchShifter invalid\n");
		return;
	}
	unit->channel = channel;
	//pitch_shifter_pro_init(unit->ct, channel, sample_rate, unit->semitone_steps, CFG_MUSIC_PITCH_SHIFTER_PRO_FRAME_SIZE);
	pitch_shifter_pro_init(&unit->ct, channel, sample_rate, unit->param.semitone_steps, 512);//帧长的配置
}

void AudioEffectPitchShifterProApply(PitchShifterProUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	pitch_shifter_pro_apply(&unit->ct, pcm_in, pcm_out);
}
#endif

#if CFG_AUDIO_EFFECT_PCM_DELAY_EN
void AudioEffectPcmDelayInit(PcmDelayUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	uint16_t max_delay_samples = 0;//需要计算
	if(unit->enable == FALSE)
	{
		DBG("PcmDelay invalid\n");
		return;
	}
	unit->channel = channel;
	max_delay_samples = unit->param.max_delay * gCtrlVars.sample_rate/1000;
	pcm_delay_init(&unit->ct, channel, max_delay_samples, unit->param.high_quality, unit->s_buf);
}

void AudioEffectPcmDelayApply(PcmDelayUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	uint16_t delay_samples = 0;
	if(unit->enable == FALSE)
	{
		return;
	}
	delay_samples = (unit->param.delay*gCtrlVars.sample_rate)/1000;
	pcm_delay_apply(&unit->ct, pcm_in, pcm_out, n, delay_samples);
}

void AudioEffectPcmDelayApply24(PcmDelayUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	uint32_t delay_samples = 0;
	if(unit->enable == FALSE)
	{
		return;
	}
	delay_samples = (unit->param.delay*gCtrlVars.sample_rate)/1000;
	pcm_delay_apply24(&unit->ct, pcm_in, pcm_out, n, delay_samples);
}
#endif

#if CFG_AUDIO_EFFECT_EXCITER_EN
void AudioEffectExciterInit(ExciterUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("Exciter invalid\n");
		return;
	}
	unit->channel = channel;
	exciter_init(&unit->ct, channel, sample_rate, unit->param.f_cut);
}

void AudioEffectExciterApply(ExciterUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	exciter_apply(&unit->ct, pcm_in, pcm_out, n, unit->param.dry, unit->param.wet);
}
#ifdef CFG_AUDIO_WIDTH_24BIT
void AudioEffectExciterApply24(ExciterUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	exciter_apply24(&unit->ct, pcm_in, pcm_out, n, unit->param.dry, unit->param.wet);
}
#endif
#endif

#if CFG_AUDIO_EFFECT_CHORUS_EN
void AudioEffectChorusInit(ChorusUnit *unit,  uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("Chorus invalid\n");
		return;
	}
	unit->channel = channel;
	chorus_init(&unit->ct, sample_rate, unit->param.delay_length, unit->param.mod_depth, unit->param.mod_rate);
}

void AudioEffectChorusApply(ChorusUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	chorus_apply(&unit->ct, pcm_in, pcm_out, n, unit->param.feedback, unit->param.dry, unit->param.wet, unit->param.mod_rate);
}
#endif

#if CFG_AUDIO_EFFECT_AUTOWAH_EN
void AudioEffectAutoWahInit(AutoWahUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("AutoWah invalid\n");
		return;
	}
	unit->channel = channel;
	auto_wah_init(&unit->ct, sample_rate, unit->param.modulation_rate, unit->param.min_frequency, unit->param.max_frequency, unit->param.depth);
}

void AudioEffectAutoWahApply(AutoWahUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	auto_wah_apply(&unit->ct, pcm_in, pcm_out, n, unit->param.dry, unit->param.wet, unit->param.modulation_rate);
}
#endif

#if CFG_AUDIO_EFFECT_STEREO_WIDEN_EN
void AudioEffectStereoWidenerInit(StereoWidenerUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("StereoWidener invalid\n");
		return;
	}
	unit->channel = channel;
	stereo_widener_init(&unit->ct, sample_rate, unit->param.shaping);
}

void AudioEffectStereoWidenerApply(StereoWidenerUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	stereo_widener_apply(&unit->ct, pcm_in, pcm_out, n);
}
#ifdef CFG_AUDIO_WIDTH_24BIT
void AudioEffectStereoWidenerApply24(StereoWidenerUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	stereo_widener_apply24(&unit->ct, pcm_in, pcm_out, n);
}
#endif
#endif

#if CFG_AUDIO_EFFECT_PINGPONG_EN
void AudioEffectPingPongInit(PingPongUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	uint16_t max_delay_samples = 0;
	if(unit->enable == FALSE)
	{
		DBG("PingPong invalid\n");
		return;
	}
	unit->channel = channel;
	max_delay_samples = unit->param.max_delay * gCtrlVars.sample_rate/1000;
	pingpong_init(&unit->ct, (int32_t)max_delay_samples, (int32_t)unit->param.high_quality_enable, unit->s);
}

void AudioEffectPingPongApply(PingPongUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	uint16_t delay_samples;//需要计算
	if(unit->enable == FALSE)
	{
		return;
	}
	delay_samples = (unit->param.delay * gCtrlVars.sample_rate) / 1000;//需要计算
	pingpong_apply(&unit->ct, pcm_in, pcm_out, n, unit->param.attenuation, (int32_t)delay_samples, (int32_t)unit->param.wetdrymix);
}
#endif

#if CFG_AUDIO_EFFECT_3D_PLUS_EN
void AudioEffectThreeDPlusInit(ThreeDPlusUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("ThreeD Plus invalid\n");
		return;
	}
	unit->channel = channel;
	three_d_plus_init(&unit->ct, sample_rate);
}

void AudioEffectThreeDPlusApply(ThreeDPlusUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
    {
    	return;
    }
	three_d_plus_apply(&unit->ct, pcm_in, pcm_out, n, unit->param.intensity);
}
#endif

#if CFG_AUDIO_EFFECT_NS_BLUE_EN
//采样32K固定
void AudioEffectNSBlueInit(NSBlueUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("NS blue invalid\n");
		return;
	}
	unit->channel = channel;
	blue_ns_init_256(&unit->ct);
}

void AudioEffectNSBlueApply(NSBlueUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
    {
    	return;
    }
	blue_ns_run_256(&unit->ct, pcm_in , pcm_out, unit->param.level);
}
#endif

#if CFG_AUDIO_EFFECT_FLANGER_EN
//Flanger主循环处理函数 for mono
void AudioEffectFlangerInit(FlangerUnit *unit,int32_t sample_rate)
{
	unit->channel = 1;
	if(unit->enable == FALSE)
	{
		DBG("FlangerUnit invalid\n");
		return;
	}
	flanger_init(&unit->ct, sample_rate, unit->param.delay_length, unit->param.mod_depth, unit->param.mod_rate);
}

void AudioEffectFlangerApply(FlangerUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	flanger_apply(&unit->ct, pcm_in, pcm_out, n, unit->param.feedback, unit->param.dry, unit->param.wet, unit->param.mod_rate);
}
#endif

#if CFG_AUDIO_EFFECT_FREQ_SHIFTER_FINE_EN
//单声道音效
void AudioEffectFreqShifterFineInit(FreqShifterFineUnit *unit,int32_t sample_rate)
{
	unit->channel = 1;
	if(unit->enable == FALSE)
	{
		DBG("FreqShifter invalid\n");
		return;
	}
	freqshifter_fine_init(&unit->ct,sample_rate,unit->param.deltaf);
}

void AudioEffectFreqShifterFineApply(FreqShifterFineUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	freqshifter_fine_apply(&unit->ct, pcm_in, pcm_out, n);
}
#endif

#if	CFG_AUDIO_EFFECT_OVERDRIVE_EN
void AudioEffectOverdriveInit(OverdriveUnit *unit,int32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("FreqShifter invalid\n");
		return;
	}
	overdrive_init(&unit->ct,sample_rate,unit->param.threshold_compression);
}

void AudioEffectOverdriveApply(OverdriveUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	overdrive_apply(&unit->ct, pcm_in, pcm_out, n);
}
#endif
#if	CFG_AUDIO_EFFECT_DISTORTION_EN
void AudioEffectDistortionInit(DistortionUnit *unit,int32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("Distortion invalid\n");
		return;
	}
	distortion_exp_init(&unit->ct,sample_rate,unit->param.gain);
}

void AudioEffectDistortionApply(DistortionUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	distortion_exp_apply(&unit->ct, pcm_in, pcm_out, n, unit->param.gain, unit->param.dry, unit->param.wet);
}
#endif

#if	CFG_AUDIO_EFFECT_EQDRC_EN
void AudioEffectEQDRCInit(EQDRCUnit *unit, uint8_t channel, int32_t sample_rate)
{
	uint32_t i, filter_count = 0;;
	int16_t *eq_pp = NULL;//&pnode->efft_param[2]
	//位宽对齐
	int32_t fc[2] = {unit->param.param_drc.fc[0], unit->param.param_drc.fc[1]};
	int32_t threshold[4] = {unit->param.param_drc.threshold[0], unit->param.param_drc.threshold[1], unit->param.param_drc.threshold[2], unit->param.param_drc.threshold[3]};
	int32_t ratio[4] = {unit->param.param_drc.ratio[0], unit->param.param_drc.ratio[1], unit->param.param_drc.ratio[2], unit->param.param_drc.ratio[3]};
	int32_t attack_tc[4] = {unit->param.param_drc.attack_tc[0], unit->param.param_drc.attack_tc[1], unit->param.param_drc.attack_tc[2], unit->param.param_drc.attack_tc[3]};
	int32_t release_tc[4] = {unit->param.param_drc.release_tc[0], unit->param.param_drc.release_tc[1], unit->param.param_drc.release_tc[2], unit->param.param_drc.release_tc[3]};
	if(unit->enable == FALSE)
	{
		return;
	}
	eq_pp = (int16_t *)unit->param.param_eq.eq_params;

	memset(tempEqParam, 0x00, sizeof(tempEqParam));
	for(i=0; i<10; i++, eq_pp+=5)
	{
		if(eq_pp[0])
		{
			tempEqParam[filter_count].type = eq_pp[1];
			tempEqParam[filter_count].f0 = eq_pp[2];
			tempEqParam[filter_count].Q = eq_pp[3];
			tempEqParam[filter_count].gain = eq_pp[4];
			filter_count ++;
		}
	}
	unit->channel = channel;

	eq_drc_init(&unit->ct,channel, sample_rate, tempEqParam,filter_count,
			unit->param.param_drc.mode, unit->param.param_drc.cf_type, unit->param.param_drc.q_l, unit->param.param_drc.q_h, fc, threshold, ratio, attack_tc, release_tc);
}

void AudioEffectEQDRCConfig(EQDRCUnit *unit,int32_t sample_rate)
{
	uint32_t i, filter_count = 0;;
	int16_t *eq_pp = NULL;

	if(unit->enable == FALSE)
	{
		return;
	}
	eq_pp = (int16_t *)unit->param.param_eq.eq_params;

	for(i=0; i<10; i++, eq_pp+=5)
	{
		if(eq_pp[0])
		{
			tempEqParam[filter_count].type = eq_pp[1];
			tempEqParam[filter_count].f0 = eq_pp[2];
			tempEqParam[filter_count].Q = eq_pp[3];
			tempEqParam[filter_count].gain = eq_pp[4];
			filter_count ++;
		}
	}

	eq_drc_configure_filters(&unit->ct, sample_rate, tempEqParam, filter_count);
}

void AudioEffectEQDRCApply(EQDRCUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	int32_t pregain[4] = {unit->param.param_drc.pregain[0], unit->param.param_drc.pregain[1], unit->param.param_drc.pregain[2], unit->param.param_drc.pregain[3]};
	if(unit->enable == FALSE)
	{
		return;
	}

	eq_drc_apply(&unit->ct, pcm_in, pcm_out, n, pregain);
}

#ifdef CFG_AUDIO_WIDTH_24BIT
void AudioEffectEQDRCApply24(EQDRCUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	int32_t pregain[4] = {unit->param.param_drc.pregain[0], unit->param.param_drc.pregain[1], unit->param.param_drc.pregain[2], unit->param.param_drc.pregain[3]};
	if(unit->enable == FALSE)
	{
		return;
	}

	eq_drc_apply24(&unit->ct, pcm_in, pcm_out, n, pregain);
}
#endif
#endif

#if CFG_AUDIO_EFFECT_AEC_EN
void AudioEffectAECInit(AECUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		DBG("AEC invalid\n");
		return;
	}
	unit->channel = channel;
	//blue_aec_init(&unit->ct, unit->param.es_level, unit->param.ns_level);
	blue_aec_init(&unit->ct, unit->param.es_level);
}

void AudioEffectAECApply(AECUnit *unit, int16_t *u_pcm_in, int16_t *d_pcm_in, int16_t *pcm_out, uint32_t n)
{
	uint16_t fram, i/*,samples*/;
	fram = n/AEC_BLK_LEN;
	if(fram == 0)
	{
		fram = 1;
	}
	if(unit->enable == FALSE)
	{
		return;
	}
	if((unit->enable))
	{
		for(i = 0; i < fram; i++)
		{
			blue_aec_run(&unit->ct,  (int16_t *)(u_pcm_in + i * AEC_BLK_LEN), (int16_t *)(d_pcm_in + i * AEC_BLK_LEN), (int16_t *)(pcm_out + i * AEC_BLK_LEN));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_DISTORTION_DS1_EN
void AudioEffectDistortionDS1Init(DistortionDS1Unit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	unit->channel = channel;
	distortion_ds1_init(&unit->ct, sample_rate, unit->param.distortion_level);
}
void AudioEffectDistortionDS1Apply(DistortionDS1Unit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	distortion_ds1_apply(&unit->ct, pcm_in, pcm_out, n, unit->param.out_level);
}
#ifdef CFG_AUDIO_WIDTH_24BIT
void AudioEffectDistortionDS1Apply24(DistortionDS1Unit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	//need complete later
}
#endif
#endif

#if CFG_AUDIO_EFFECT_OVERDRIVE_POLY_EN
void AudioEffectOverdrivePolyInit(OverdrivePolyUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	unit->channel = channel;
	overdrive_poly_init(&unit->ct, sample_rate);
}
void AudioEffectOverdrivePolyApply(OverdrivePolyUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	overdrive_poly_apply(&unit->ct, pcm_in, pcm_out, n, unit->param.gain, unit->param.out_level);
}
#ifdef CFG_AUDIO_WIDTH_24BIT
void AudioEffectOverdrivePolyApply24(OverdrivePolyUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	//need complete later
}
#endif
#endif

#if	CFG_AUDIO_EFFECT_COMPANDER_EN
void AudioEffectCompanderInit(CompanderUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	unit->channel = channel;
	compander_init(&unit->ct, channel, sample_rate, unit->param.threshold, unit->param.slope_below,
					unit->param.slope_above, unit->param.alpha_attack, unit->param.alpha_release);
}

#define SCALING_Q12_MAX (0x1000)
#define SCALING_Q12_MAX_HALF (SCALING_Q12_MAX/2)
uint32_t roboeffect_db_to_scaling(float db, uint32_t scaling_max)
{
	// printf("-->%0.2f\n", db);
	return (uint32_t)roundf(powf(10.0f,((float)db/20.0f)) * scaling_max);
}

void AudioEffectCompanderApply(CompanderUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	int32_t pregain = roboeffect_db_to_scaling(((float)unit->param.pregain)/100, SCALING_Q12_MAX);
	compander_apply(&unit->ct, pcm_in, pcm_out, n, pregain);
}

#ifdef CFG_AUDIO_WIDTH_24BIT
void AudioEffectCompanderApply24(CompanderUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	int32_t pregain = roboeffect_db_to_scaling(((float)unit->param.pregain)/100, SCALING_Q12_MAX);
	compander_apply24(&unit->ct, pcm_in, pcm_out, n, pregain);
}
#endif
#endif

#if	CFG_AUDIO_EFFECT_LOW_LEVEL_COMPRESSOR_EN
void AudioEffectLowLevelCompressorInit(LowLevelCompressorUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	unit->channel = channel;
	low_level_compressor_init(&unit->ct, channel, sample_rate, unit->param.threshold, unit->param.gain, unit->param.alpha_attack, unit->param.alpha_release);
}

void AudioEffectLowLevelCompressorApply(LowLevelCompressorUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	low_level_compressor_apply(&unit->ct, pcm_in, pcm_out, n);
}

#ifdef CFG_AUDIO_WIDTH_24BIT
void AudioEffectLowLevelCompressorApply24(LowLevelCompressorUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	if(unit->enable == FALSE)
	{
		return;
	}
	low_level_compressor_apply24(&unit->ct, pcm_in, pcm_out, n);
}
#endif
#endif

#if CFG_AUDIO_EFFECT_HOWLING_SUPPRESSOR_FINE_EN
void AudioEffectHowlingSuppressorFineInit(HowlingFineUnit *unit,uint32_t sample_rate)
{
	if(!unit->enable)
	{
		return;
	}
	unit->channel = 1;
	if((unit->enable) && (&unit->ct != NULL))
	{
	  howling_suppressor_fine_init(&unit->ct, sample_rate);
	}
}

void AudioEffectHowlingSuppressorFineApply(HowlingFineUnit *unit, int16_t *pcm_in, int16_t *pcm_out, int32_t n)
{
	if((unit->enable) && (&unit->ct != NULL))
	{
		howling_suppressor_fine_apply(&unit->ct,  pcm_in, pcm_out, n,unit->param.q_min,unit->param.q_max);
	}
}
#endif

#endif

