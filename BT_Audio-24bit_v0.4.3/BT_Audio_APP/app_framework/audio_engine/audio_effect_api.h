/*******************************************************
 *         MVSilicon Audio Effects Parameters
 *
 *                All Right Reserved
 *******************************************************/

#ifndef __AUDIO_EFFECT_API_H__
#define __AUDIO_EFFECT_API_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "rtos_api.h"
#include "eq.h"
#include "pcm_delay.h"
#include "drc.h"
#include "virtual_bass.h"
#include "virtual_bass_classic.h"
#include "blue_aec.h"
#include "expander.h"
#include "silence_detector.h"
#include "three_d.h"
#include "three_d_plus.h"
#include "echo.h"
#include "pingpong.h"
#include "stereo_widener.h"
#include "auto_wah.h"
#include "chorus.h"
#include "exciter.h"
#include "pitch_shifter_pro.h"
#include "vocal_remover.h"
#include "voice_changer.h"
#include "voice_changer_pro.h"
#include "reverb_pro.h"
#include "plate_reverb.h"
#include "vocalcut.h"
#include "reverb.h"
#include "pitch_shifter.h"
#include "howling_suppressor.h"
#include "freqshifter.h"
#include "auto_tune.h"
#include "blue_ns.h"
#include "freqshifter_fine.h"
#include "flanger.h"
#include "overdrive.h"
#include "distortion_exp.h"
#include "eq_drc.h"
#include "compander.h"
#include "low_level_compressor.h"
#include "distortion_ds1.h"
#include "overdrive_poly.h"
#include "howling_suppressor_fine.h"

#include "audio_effect_user_config.h"

////以下宏考虑到需要经常修改，配置到 audio_effect_user_config.h 中
//////****************************************************************************************
//////。。。说明。。。
//#define CFG_AUDIO_EFFECT_AUTO_TUNE_EN					(1)//mic
//#define	CFG_AUDIO_EFFECT_DRC_EN							(1)
//#define CFG_AUDIO_EFFECT_ECHO_EN						(1)//mic
//#define	CFG_AUDIO_EFFECT_EQ_EN							(1)
//#define CFG_AUDIO_EFFECT_NOISE_SUPPRESSOR_EN			(1)//expander
//#define	CFG_AUDIO_EFFECT_FREQ_SHIFTER_EN				(1)//mic
//#define	CFG_AUDIO_EFFECT_HOWLING_SUPPRESSOR_EN			(1)//mic
//#define	CFG_AUDIO_EFFECT_PITCH_SHIFTER_EN				(1)//mic
//#define	CFG_AUDIO_EFFECT_REVERB_EN						(1)//mic
//#define	CFG_AUDIO_EFFECT_SILENCE_DECTOR_EN				(1)
//#define CFG_AUDIO_EFFECT_3D_EN							(1)
//#define CFG_AUDIO_EFFECT_VB_EN							(1)
//#define CFG_AUDIO_EFFECT_VOICE_CHANGER_EN				(1)//mic
//#define	CFG_AUDIO_EFFECT_GAIN_CONTROL_EN				(1)
//#define CFG_AUDIO_EFFECT_VOCAL_CUT_EN					(1)//mic
//#define CFG_AUDIO_EFFECT_PLATE_REVERB_EN				(1)//mic
//#define CFG_AUDIO_EFFECT_REVERB_PRO_EN					(0)
//#define CFG_AUDIO_EFFECT_VOICE_CHANGER_PRO_EN			(1)//mic
//#define CFG_AUDIO_EFFECT_PHASE_CONTROL_EN				(0)
//#define CFG_AUDIO_EFFECT_VOCAL_REMOVE_EN				(0)
//#define CFG_AUDIO_EFFECT_PITCH_SHIFTER_PRO_EN			(0)
//#define CFG_AUDIO_EFFECT_VB_CLASS_EN					(1)
//#define CFG_AUDIO_EFFECT_PCM_DELAY_EN					(0)
//#define CFG_AUDIO_EFFECT_EXCITER_EN						(1)
//#define CFG_AUDIO_EFFECT_CHORUS_EN						(0)
//#define CFG_AUDIO_EFFECT_AUTOWAH_EN						(0)
//#define CFG_AUDIO_EFFECT_STEREO_WIDEN_EN				(1)
//#define CFG_AUDIO_EFFECT_PINGPONG_EN					(0)
//#define CFG_AUDIO_EFFECT_3D_PLUS_EN               		(0)//此音效只适用于BP1048P2或BP1064型号芯片
//#define CFG_AUDIO_EFFECT_AEC_EN		               		(1)
//#define CFG_AUDIO_EFFECT_NS_BLUE_EN		               	(0)
//#define	CFG_AUDIO_EFFECT_FLANGER_EN			            (0)
//#define	CFG_AUDIO_EFFECT_FREQ_SHIFTER_FINE_EN			(0)
//#define	CFG_AUDIO_EFFECT_OVERDRIVE_EN		            (0)
//#define	CFG_AUDIO_EFFECT_DISTORTION_EN					(0)
//#define	CFG_AUDIO_EFFECT_EQDRC_EN						(1)

typedef enum _EffectType
{
	AUTO_TUNE = 0x00,
	DC_BLOCK,//未使用
	DRC,
	ECHO,
	EQ,
	EXPANDER,//5,Noise Suppressor
	FREQ_SHIFTER,
	HOWLING_SUPPRESSOR,
	NOISE_GATE,//未使用
	PITCH_SHIFTER,
	REVERB,//10
	SILENCE_DETECTOR,
	THREE_D,
	VIRTUAL_BASS,
	VOICE_CHANGER,
	GAIN_CONTROL,//15
	VOCAL_CUT,
	PLATE_REVERB,
	REVERB_PRO,
	VOICE_CHANGER_PRO,
	PHASE_CONTROL,//20
	VOCAL_REMOVE,
	PITCH_SHIFTER_PRO,
	VIRTUAL_BASS_CLASSIC,
	PCM_DELAY,
	EXCITER,//25
	CHORUS,
	AUTO_WAH,
	STEREO_WIDENER,
	PINGPONG,
	THREE_D_PLUS,//30
	SINE_GENERATOR,
	NOISE_Suppressor_Blue,
	FLANGER,
	FREQ_SHIFTER_FINE,
	OVERDRIVE,//35
	DISTORTION,
	EQ_DRC,
	AEC,
	DISTORTION_DS1,
	OVERDRIVE_POLY,//40
	COMPANDER,
	LOW_LEVEL_COMPRESSOR,
	HOWLING_FINE,
	CUSTOMER = 100,////用户自定义音效，固定为100
	//注意，用户自定义音效，参数往后增加
	EFFECT_NUM_MAX
} EffectType;

//--------audio effect unit define-----------------------------------//
typedef struct __AutoTuneParam
{
	uint16_t 		 key;
	uint16_t  		 snap_mode;
	uint16_t         pitch_accuracy;
} AutoTuneParam;
typedef struct __AutoTuneUnit
{
	AutoTuneContext ct;
	AutoTuneParam	param;
	uint8_t			enable;
	uint8_t			channel;
} AutoTuneUnit;

typedef struct __DRCParam
{
	uint16_t		mode;
	uint16_t		cf_type;
	uint16_t		q_l;
	uint16_t		q_h;
	int16_t			fc[2];
	int16_t			threshold[4];
	int16_t			ratio[4];
	int16_t			attack_tc[4];
	int16_t			release_tc[4];
	int16_t			pregain[4];
} DRCParam;
typedef struct __DRCUnit
{
	DRCContext 		ct;
	DRCParam		param;
	uint8_t			enable;
	uint8_t			channel;
} DRCUnit;

typedef struct __EchoParam
{
	int16_t  		 fc;
	int16_t  		 attenuation;
	int16_t  		 delay;
	int16_t			 reserved;
	int16_t  		 max_delay;
	int16_t  		 high_quality_enable;
	int16_t  		 dry;
	int16_t			 wet;
} EchoParam;
typedef struct __EchoUnit
{
	EchoContext		ct;
	EchoParam		param;
	uint8_t			enable;
	uint8_t			channel;
	uint8_t*		buf;
} EchoUnit;

typedef struct __FilterParams
{
	uint16_t		enable;
	int16_t			type;           /**< filter type, @see EQ_FILTER_TYPE_SET                               */
	uint16_t		f0;             /**< center frequency (peak) or mid-point frequency (shelf)             */
	int16_t			Q;              /**< quality factor (peak) or slope (shelf), format: Q6.10              */
	int16_t			gain;           /**< Gain in dB, format: Q8.8 */
} FilterParams;
typedef struct __EQParam
{
	int16_t			pregain;
	uint16_t		calculation_type;
	FilterParams	eq_params[10];
} EQParam;
typedef struct __EQUnit
{
	EQContext		ct;
	EQParam			param;
	uint8_t			enable;
	uint8_t			channel;
} EQUnit;

typedef struct __ExpanderParam
{
	int16_t			threshold;
	int16_t			ratio;
	int16_t			attack_time;
	int16_t			release_time;
} ExpanderParam;
typedef struct __ExpanderUnit
{
	ExpanderContext	ct;
	ExpanderParam	param;
	uint8_t			enable;
	uint8_t			channel;
} ExpanderUnit;

typedef struct __FreqShifterParam
{
	int16_t 		deltaf;
} FreqShifterParam;
typedef struct __FreqShifterUnit
{
	FreqShifterContext	ct;
	FreqShifterParam	param;
	uint8_t				enable;
	uint8_t				channel;
} FreqShifterUnit;

typedef struct __HowlingParam
{
	int16_t 		suppression_mode;
} HowlingParam;
typedef struct __HowlingUnit
{
	HowlingContext		ct;
	HowlingParam		param;
	uint8_t				enable;
	uint8_t				channel;
} HowlingUnit;

typedef struct __PitchShifterParam
{
	int16_t 		semitone_steps;
} PitchShifterParam;
typedef struct __PitchShifterUnit
{
	PSContext			ct;
	PitchShifterParam	param;
	uint8_t				enable;
	uint8_t				channel;
} PitchShifterUnit;

typedef struct __ReverbParam
{
	int16_t 		dry_scale;
	int16_t 		wet_scale;
	int16_t 		width_scale;
	int16_t 		roomsize_scale;
	int16_t 		damping_scale;
	uint16_t		mono;
} ReverbParam;
typedef struct __ReverbUnit
{
	ReverbContext	ct;
	ReverbParam		param;
	uint8_t			enable;
	uint8_t			channel;
} ReverbUnit;

typedef struct __SilenceDetectorParam
{
	uint16_t			level;
} SilenceDetectorParam;
typedef struct __SilenceDetectorUnit
{
	SilenceDetectorContext	ct;
	SilenceDetectorParam	param;
	uint8_t					enable;
	uint8_t					channel;
} SilenceDetectorUnit;

typedef struct __ThreeDParam
{
	int16_t			intensity;
} ThreeDParam;
typedef struct __ThreeDUnit
{
	ThreeDContext	ct;
	ThreeDParam		param;
	uint8_t			enable;
	uint8_t			channel;
} ThreeDUnit;

typedef struct __VBParam
{
	int16_t			f_cut;
	int16_t			intensity;
	int16_t			enhanced;
} VBParam;
typedef struct __VBUnit
{
	VBContext 		ct;
	VBParam			param;
	uint8_t			enable;
	uint8_t			channel;
} VBUnit;

typedef struct __VoiceChangerParam
{
	int16_t			pitch_ratio;
	int16_t			formant_ratio;
} VoiceChangerParam;
typedef struct __VoiceChangerUnit
{
	VoiceChangerContext	ct;
	VoiceChangerParam	param;
	uint8_t				enable;
	uint8_t				channel;
} VoiceChangerUnit;

typedef struct __GainControlParam
{
	uint16_t      mute;
	uint16_t      gain;
} GainControlParam;
typedef struct __GainControlUnit
{
	GainControlParam	param;
	uint8_t				enable;
	uint8_t				channel;
} GainControlUnit;

typedef struct __VocalCutParam
{
	int16_t 	wetdrymix;
} VocalCutParam;
typedef struct __VocalCutUnit
{
	VocalCutContext 	ct;
	VocalCutParam		param;
	uint8_t				enable;
	uint8_t				channel;
} VocalCutUnit;

typedef struct __PlateReverbParam
{
	int16_t 	highcut_freq;
	int16_t 	modulation_en;
	int16_t 	predelay;
	int16_t 	diffusion;
	int16_t 	decay;
	int16_t 	damping;
	int16_t 	wetdrymix;
} PlateReverbParam;
typedef struct __PlateReverbUnit
{
	PlateReverbContext 	ct;
	PlateReverbParam	param;
	uint8_t				enable;
	uint8_t				channel;
} PlateReverbUnit;

typedef struct __ReverbProParam
{
	int16_t dry;
	int16_t wet;
	int16_t erwet;
	int16_t erfactor;
	int16_t erwidth;
	int16_t ertolate;
	int16_t rt60;
	int16_t delay;
	int16_t width;
	int16_t wander;
	int16_t spin;
	int16_t inputlpf;
	int16_t damplpf;
	int16_t basslpf;
	int16_t bassb;
	int16_t outputlpf;
} ReverbProParam;
typedef struct __ReverbProUnit
{
	uint8_t*			ct;
	ReverbProParam		param;
	uint8_t				enable;
	uint8_t				channel;
} ReverbProUnit;

typedef struct __VoiceChangerProParam
{
	int16_t 			 pitch_ratio;
	int16_t 		     formant_ratio;
} VoiceChangerProParam;
typedef struct __VoiceChangerProUnit
{
	VoiceChangerProContext	ct;
	VoiceChangerProParam	param;
	uint8_t					enable;
	uint8_t					channel;
} VoiceChangerProUnit;

typedef struct __PhaseControlParam//20
{
	uint16_t		phase_difference;
} PhaseControlParam;
typedef struct __PhaseControlUnit
{
	PhaseControlParam	param;
	uint8_t				enable;
	uint8_t				channel;
} PhaseControlUnit;

typedef struct __VocalRemoverParam
{
	int16_t			lower_freq;
	int16_t			higher_freq;
} VocalRemoverParam;
typedef struct __VocalRemoveUnit
{
	VocalRemoverContext 	ct;
	VocalRemoverParam		param;
	uint8_t					enable;
	uint8_t					channel;
} VocalRemoveUnit;

typedef struct __PitchShifterProParam
{
	int16_t			semitone_steps;
} PitchShifterProParam;
typedef struct __PitchShifterProUnit
{
	PitchShifterProContext 	ct;
	PitchShifterProParam	param;
	uint8_t					enable;
	uint8_t					channel;
} PitchShifterProUnit;

typedef struct __VBClassicParam
{
	int16_t			f_cut;
	int16_t			intensity;
} VBClassicParam;
typedef struct __VBClassUnit
{
	VBClassicContext	ct;
	VBClassicParam		param;
	uint8_t				enable;
	uint8_t				channel;
} VBClassUnit;

typedef struct __PcmDelayParam
{
	int16_t			delay;
	int16_t			max_delay;
	int16_t			high_quality;
} PcmDelayParam;
typedef struct __PcmDelayUnit
{
	PCMDelay 		ct;
	PcmDelayParam	param;
	uint8_t			enable;
	uint8_t			channel;
	uint8_t			*s_buf;
} PcmDelayUnit;

typedef struct __ExciterParam//25
{
	int16_t 			f_cut;
	int16_t 			dry;
	int16_t 			wet;
} ExciterParam;
typedef struct __ExciterUnit
{
	ExciterContext 		ct;
	ExciterParam		param;
	uint8_t				enable;
	uint8_t				channel;
} ExciterUnit;

typedef struct __ChorusParam
{
	int16_t 			 delay_length;
	int16_t 		     mod_depth;
	uint16_t   	 		 mod_rate;
	uint16_t   	 		 feedback;
	uint16_t   	 		 dry;
	uint16_t   	 		 wet;
} ChorusParam;
typedef struct __ChorusUnit
{
	ChorusContext 		ct;
	ChorusParam			param;
	uint8_t				enable;
	uint8_t				channel;
} ChorusUnit;

typedef struct __AutoWahParam
{
	uint16_t         modulation_rate;
	uint16_t         min_frequency;
	uint16_t         max_frequency;
	uint16_t         depth;
	uint16_t 		 dry;
	uint16_t 		 wet;
} AutoWahParam;
typedef struct __AutoWahUnit
{
	AutoWahContext 		ct;
	AutoWahParam		param;
	uint8_t				enable;
	uint8_t				channel;
} AutoWahUnit;

typedef struct __StereoWidenerParam
{
	int16_t          shaping;
} StereoWidenerParam;
typedef struct __StereoWidenerUnit
{
	StereoWidenerContext 	ct;
	StereoWidenerParam		param;
	uint8_t					enable;
	uint8_t					channel;
} StereoWidenerUnit;

typedef struct __PingPongParam
{
	int16_t          attenuation;
	int16_t          delay;
	int16_t          high_quality_enable;
	int16_t          wetdrymix;
	int16_t  		 max_delay;
} PingPongParam;
typedef struct __PingPongUnit
{
	PingPongContext	ct;
	PingPongParam	param;
	uint8_t			enable;
	uint8_t			channel;
	uint8_t			*s;
} PingPongUnit;

typedef struct __ThreeDPlusParam
{
	int16_t			intensity;
} ThreeDPlusParam;
typedef struct __ThreeDPlusUnit
{
	ThreeDPlusContext	ct;
	ThreeDPlusParam		param;
	uint8_t				enable;
	uint8_t				channel;
} ThreeDPlusUnit;

typedef struct __NoiseSuppressorBlueParam
{
	int16_t				level;
} NSBlueParam;
typedef struct __NSBlueUnit
{
	BlueNSContext256	ct;
	NSBlueParam			param;
	uint8_t				enable;
	uint8_t				channel;
} NSBlueUnit;

typedef struct __FlangerParam
{
	int16_t				delay_length;
	int16_t				mod_depth;
	int16_t				mod_rate;
	int16_t				feedback;
	int16_t				dry;
	int16_t				wet;
} FlangerParam;
typedef struct __FlangerUnit
{
	FlangerContext	ct;
	FlangerParam	param;
	uint8_t			enable;
	uint8_t			channel;
} FlangerUnit;

typedef struct __FreqShifterFineParam
{
	int16_t 		deltaf;
} FreqShifterFineParam;
typedef struct __FreqShifterFineUnit
{
	FreqShifterFineContext	ct;
	FreqShifterFineParam	param;
	uint8_t					enable;
	uint8_t					channel;
} FreqShifterFineUnit;

typedef struct _OverdriveParam
{
	int16_t			threshold_compression;
}OverdriveParam;
typedef struct _OverdriveUnit
{
	OverdriveContext	ct;
	OverdriveParam		param;
	uint8_t				enable;
	uint8_t				channel;
}OverdriveUnit;

typedef struct __DistortionParam
{
	int16_t				gain;
	int16_t				dry;
	int16_t				wet;
} DistortionParam;
typedef struct __DistortionUnit
{
	DistortionExpContext	ct;
	DistortionParam			param;
	uint8_t					enable;
	uint8_t					channel;
} DistortionUnit;

typedef struct __EQDRCParam
{
	EQParam			param_eq;
	DRCParam		param_drc;
} EQDRCParam;
typedef struct __EQDRCUnit
{
	EQDRCContext			ct;
	EQDRCParam				param;
	uint8_t					enable;
	uint8_t					channel;
} EQDRCUnit;

#if 0
#pragma pack(1)
typedef struct __AECParam
{
	uint8_t				param_cnt;
	uint8_t				param1_name_len;//长度是固定值
	char				param1_name[9];
	uint8_t 			param1_state;
	uint16_t 			param1_value_min;
	uint16_t 			param1_value_max;
	uint16_t 			es_level;
	uint8_t				param2_name_len;//长度是固定值
	char				param2_name[10];
	uint8_t 			param2_state;
	uint16_t 			param2_value_min;
	uint16_t 			param2_value_max;
	uint16_t 			ns_level;
} AECParam;
#pragma pack()
typedef struct __AECUnit
{
	BlueAECContext	ct;
	AECParam		param;
	uint8_t			enable;
	uint8_t			channel;
} AECUnit;
#endif
//AEC 从 v2.16.0开始需要使用新的音效参数
typedef struct __AECParam
{
	uint16_t		es_level;
	uint16_t 		ns_level;
} AECParam;
typedef struct __AECUnit
{
	BlueAECContext	ct;
	AECParam		param;
	uint8_t			enable;
	uint8_t			channel;
} AECUnit;

typedef struct __DistortionDS1Param
{
	int16_t distortion_level;
	int16_t out_level;
} DistortionDS1Param;
typedef struct __DistortionDS1Unit
{
	DistortionDS1Context	ct;
	DistortionDS1Param		param;
	uint8_t					enable;
	uint8_t					channel;
} DistortionDS1Unit;

typedef struct __OverdrivePolyParam
{
	int16_t gain;
	int16_t out_level;
} OverdrivePolyParam;
typedef struct __OverdrivePolyUnit
{
	OverdrivePolyContext	ct;
	OverdrivePolyParam		param;
	uint8_t					enable;
	uint8_t					channel;
} OverdrivePolyUnit;

typedef struct __CompanderParam
{
	int16_t threshold;
	int16_t slope_below;
	int16_t slope_above;
	int16_t alpha_attack;
	int16_t alpha_release;
	int16_t pregain;
} CompanderParam;
typedef struct __CompanderUnit
{
	CompanderContext	ct;
	CompanderParam		param;
	uint8_t				enable;
	uint8_t				channel;
} CompanderUnit;

typedef struct __LowLevelCompressorParam
{
	int16_t threshold;
	int16_t gain;
	int16_t alpha_attack;
	int16_t alpha_release;
} LLCompressorParam;
typedef struct __LowLevelCompressorUnit
{
	LowLevelCompressorContext	ct;
	LLCompressorParam			param;
	uint8_t						enable;
	uint8_t						channel;
} LowLevelCompressorUnit;

typedef struct __HowlingFineParam
{
	int16_t 		q_min;
	int16_t 		q_max;
} HowlingFineParam;
typedef struct __HowlingFineUnit
{
	HowlingFineContext	ct;
	HowlingFineParam	param;
	uint8_t				enable;
	uint8_t				channel;
} HowlingFineUnit;

#ifdef FUNC_OS_EN
extern osMutexId AudioEffectMutex;
#endif

void AudioEffectAutoTuneInit(AutoTuneUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectAutoTuneApply(AutoTuneUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectDRCInit(DRCUnit *DRCunit, uint8_t channel, uint32_t sample_rate);
void AudioEffectDRCApply(DRCUnit *DRCunit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectDRCApply24(DRCUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectEchoInit(EchoUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectEchoApply(EchoUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectEQInit(EQUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectEQPregainConfig(EQUnit *unit);
void AudioEffectEQFilterConfig(EQUnit *unit, uint32_t sample_rate);
void AudioEffectEQApply(EQUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectEQApply24(EQUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectExpanderInit(ExpanderUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectExpanderThresholdConfig(ExpanderUnit *unit);
void AudioEffectExpanderApply(ExpanderUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectExpanderApply24(ExpanderUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectFreqShifterInit(FreqShifterUnit *unit);
void AudioEffectFreqShifterApply(FreqShifterUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectHowlingSuppressorInit(HowlingUnit *unit);
void AudioEffectHowlingSuppressorApply(HowlingUnit *unit, int16_t *pcm_in, int16_t *pcm_out, int32_t n);

void AudioEffectPitchShifterInit(PitchShifterUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectPitchShifterConfig(PitchShifterUnit *unit);
void AudioEffectPitchShifterApply(PitchShifterUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectReverbInit(ReverbUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectReverbConfig(ReverbUnit *unit);
void AudioEffectReverbApply(ReverbUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectSilenceDetectorInit(SilenceDetectorUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectSilenceDetectorApply(SilenceDetectorUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectSilenceDetectorApply24(SilenceDetectorUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectThreeDInit(ThreeDUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectThreeDApply(ThreeDUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectThreeDApply24(ThreeDUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectVBInit(VBUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectVBApply(VBUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectVBApply24(VBUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectVoiceChangerInit(VoiceChangerUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectVoiceChangerApply(VoiceChangerUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectPregainInit(GainControlUnit *unit, uint8_t channel);
void AudioEffectPregainApply(GainControlUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectPregainApply24(GainControlUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectVocalCutInit(VocalCutUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectVocalCutApply(VocalCutUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectPlateReverbInit(PlateReverbUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectPlateReverbConfig(PlateReverbUnit *unit);
void AudioEffectPlateReverbApply(PlateReverbUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectReverbProInit(ReverbProUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectReverbProApply(ReverbProUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectVoiceChangerProInit(VoiceChangerProUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectVoiceChangerProApply(VoiceChangerProUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectPhaseInit(PhaseControlUnit *unit, uint8_t channel);
void AudioEffectPhaseApply(PhaseControlUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectVocalRemoveInit(VocalRemoveUnit *unit,  uint8_t channel, uint32_t sample_rate);
void AudioEffectVocalRemoveApply(VocalRemoveUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectPitchShifterProInit(PitchShifterProUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectPitchShifterProApply(PitchShifterProUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectVBClassInit(VBClassUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectVBClassApply(VBClassUnit * unit,int16_t * pcm_in,int16_t * pcm_out,uint32_t n);
void AudioEffectVBClassApply24(VBClassUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectPcmDelayInit(PcmDelayUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectPcmDelayApply(PcmDelayUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectPcmDelayApply24(PcmDelayUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectExciterInit(ExciterUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectExciterApply(ExciterUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectExciterApply24(ExciterUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectChorusInit(ChorusUnit *unit,  uint8_t channel, uint32_t sample_rate);
void AudioEffectChorusApply(ChorusUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectAutoWahInit(AutoWahUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectAutoWahApply(AutoWahUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectStereoWidenerInit(StereoWidenerUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectStereoWidenerApply(StereoWidenerUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectStereoWidenerApply24(StereoWidenerUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectPingPongInit(PingPongUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectPingPongApply(PingPongUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectThreeDPlusInit(ThreeDPlusUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectThreeDPlusApply(ThreeDPlusUnit * unit,int16_t * pcm_in,int16_t * pcm_out,uint32_t n);

void AudioEffectNSBlueInit(NSBlueUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectNSBlueApply(NSBlueUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectFlangerInit(FlangerUnit *unit,int32_t sample_rate);
void AudioEffectFlangerApply(FlangerUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectFreqShifterFineInit(FreqShifterFineUnit *unit,int32_t sample_rate);
void AudioEffectFreqShifterFineApply(FreqShifterFineUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectOverdriveInit(OverdriveUnit *unit,int32_t sample_rate);
void AudioEffectOverdriveApply(OverdriveUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectDistortionInit(DistortionUnit *unit,int32_t sample_rate);
void AudioEffectDistortionApply(DistortionUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectEQDRCInit(EQDRCUnit *unit, uint8_t channel, int32_t sample_rate);
void AudioEffectEQDRCConfig(EQDRCUnit *unit,int32_t sample_rate);
void AudioEffectEQDRCApply(EQDRCUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectEQDRCApply24(EQDRCUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

//AEC为特殊处理音效，具体见process函数
void AudioEffectAECInit(AECUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectAECApply(AECUnit *unit, int16_t *u_pcm_in, int16_t *d_pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectDistortionDS1Init(DistortionDS1Unit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectDistortionDS1Apply(DistortionDS1Unit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
//void AudioEffectDistortionDS1Apply24(DistortionDS1Unit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectOverdrivePolyInit(OverdrivePolyUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectOverdrivePolyApply(OverdrivePolyUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
//void AudioEffectOverdrivePolyApply24(OverdrivePolyUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectCompanderInit(CompanderUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectCompanderApply(CompanderUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectCompanderApply24(CompanderUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectLowLevelCompressorInit(LowLevelCompressorUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectLowLevelCompressorApply(LowLevelCompressorUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectLowLevelCompressorApply24(LowLevelCompressorUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectHowlingSuppressorFineInit(HowlingFineUnit *unit,uint32_t sample_rate);
void AudioEffectHowlingSuppressorFineApply(HowlingFineUnit *unit, int16_t *pcm_in, int16_t *pcm_out, int32_t n);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__AUDIO_EFFECT_API_H__
