#ifndef __AUDIO_EFFECT_USER_CONFIG_H__
#define __AUDIO_EFFECT_USER_CONFIG_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"
#include "app_config.h"

#ifdef CFG_FUNC_MIC_KARAOKE_EN
//****************************************************************************************
//。。。说明。。。
#define CFG_AUDIO_EFFECT_AUTO_TUNE_EN					(1)//mic
#define	CFG_AUDIO_EFFECT_DRC_EN							(1)
#define CFG_AUDIO_EFFECT_ECHO_EN						(1)//mic
#define	CFG_AUDIO_EFFECT_EQ_EN							(1)
#define CFG_AUDIO_EFFECT_NOISE_SUPPRESSOR_EN			(1)//expander
#define	CFG_AUDIO_EFFECT_FREQ_SHIFTER_EN				(1)//mic
#define	CFG_AUDIO_EFFECT_HOWLING_SUPPRESSOR_EN			(1)//mic
#define	CFG_AUDIO_EFFECT_PITCH_SHIFTER_EN				(1)//mic
#define	CFG_AUDIO_EFFECT_REVERB_EN						(1)//mic
#define	CFG_AUDIO_EFFECT_SILENCE_DECTOR_EN				(1)
#define CFG_AUDIO_EFFECT_3D_EN							(1)
#define CFG_AUDIO_EFFECT_VB_EN							(1)
#define CFG_AUDIO_EFFECT_VOICE_CHANGER_EN				(1)//mic
#define	CFG_AUDIO_EFFECT_GAIN_CONTROL_EN				(1)
#define CFG_AUDIO_EFFECT_VOCAL_CUT_EN					(1)//mic
#define CFG_AUDIO_EFFECT_PLATE_REVERB_EN				(1)//mic
#define CFG_AUDIO_EFFECT_REVERB_PRO_EN					(0)
#define CFG_AUDIO_EFFECT_VOICE_CHANGER_PRO_EN			(1)//mic
#define CFG_AUDIO_EFFECT_PHASE_CONTROL_EN				(0)
#define CFG_AUDIO_EFFECT_VOCAL_REMOVE_EN				(0)
#define CFG_AUDIO_EFFECT_PITCH_SHIFTER_PRO_EN			(0)
#define CFG_AUDIO_EFFECT_VB_CLASS_EN					(1)
#define CFG_AUDIO_EFFECT_PCM_DELAY_EN					(0)
#define CFG_AUDIO_EFFECT_EXCITER_EN						(1)
#define CFG_AUDIO_EFFECT_CHORUS_EN						(0)
#define CFG_AUDIO_EFFECT_AUTOWAH_EN						(0)
#define CFG_AUDIO_EFFECT_STEREO_WIDEN_EN				(1)
#define CFG_AUDIO_EFFECT_PINGPONG_EN					(0)
#define CFG_AUDIO_EFFECT_3D_PLUS_EN               		(0)//此音效只适用于BP1048P2或BP1064型号芯片
#define CFG_AUDIO_EFFECT_AEC_EN		               		(1)
#define CFG_AUDIO_EFFECT_NS_BLUE_EN		               	(0)
#define	CFG_AUDIO_EFFECT_FLANGER_EN			            (0)
#define	CFG_AUDIO_EFFECT_FREQ_SHIFTER_FINE_EN			(0)
#define	CFG_AUDIO_EFFECT_OVERDRIVE_EN		            (0)
#define	CFG_AUDIO_EFFECT_DISTORTION_EN					(0)
#define	CFG_AUDIO_EFFECT_EQDRC_EN						(1)
#else
//****************************************************************************************
//。。。说明。。。
#define CFG_AUDIO_EFFECT_AUTO_TUNE_EN					(0)//mic
#define	CFG_AUDIO_EFFECT_DRC_EN							(1)
#define CFG_AUDIO_EFFECT_ECHO_EN						(0)//mic
#define	CFG_AUDIO_EFFECT_EQ_EN							(1)
#define CFG_AUDIO_EFFECT_NOISE_SUPPRESSOR_EN			(1)//expander
#define	CFG_AUDIO_EFFECT_FREQ_SHIFTER_EN				(0)//mic
#define	CFG_AUDIO_EFFECT_HOWLING_SUPPRESSOR_EN			(0)//mic
#define	CFG_AUDIO_EFFECT_PITCH_SHIFTER_EN				(0)//mic
#define	CFG_AUDIO_EFFECT_REVERB_EN						(0)//mic
#define	CFG_AUDIO_EFFECT_SILENCE_DECTOR_EN				(1)
#define CFG_AUDIO_EFFECT_3D_EN							(1)
#define CFG_AUDIO_EFFECT_VB_EN							(1)
#define CFG_AUDIO_EFFECT_VOICE_CHANGER_EN				(0)//mic
#define	CFG_AUDIO_EFFECT_GAIN_CONTROL_EN				(1)
#define CFG_AUDIO_EFFECT_VOCAL_CUT_EN					(0)//mic
#define CFG_AUDIO_EFFECT_PLATE_REVERB_EN				(0)
#define CFG_AUDIO_EFFECT_REVERB_PRO_EN					(0)
#define CFG_AUDIO_EFFECT_VOICE_CHANGER_PRO_EN			(0)//mic
#define CFG_AUDIO_EFFECT_PHASE_CONTROL_EN				(0)
#define CFG_AUDIO_EFFECT_VOCAL_REMOVE_EN				(0)
#define CFG_AUDIO_EFFECT_PITCH_SHIFTER_PRO_EN			(0)
#define CFG_AUDIO_EFFECT_VB_CLASS_EN					(1)
#define CFG_AUDIO_EFFECT_PCM_DELAY_EN					(0)
#define CFG_AUDIO_EFFECT_EXCITER_EN						(1)
#define CFG_AUDIO_EFFECT_CHORUS_EN						(0)
#define CFG_AUDIO_EFFECT_AUTOWAH_EN						(0)
#define CFG_AUDIO_EFFECT_STEREO_WIDEN_EN				(1)
#define CFG_AUDIO_EFFECT_PINGPONG_EN					(0)
#define CFG_AUDIO_EFFECT_3D_PLUS_EN               		(0)//此音效只适用于BP1048P2或BP1064型号芯片
//#define CFG_AUDIO_EFFECT_AEC_EN		               		(1)
#define CFG_AUDIO_EFFECT_NS_BLUE_EN		               	(0)
#define	CFG_AUDIO_EFFECT_FLANGER_EN			            (0)
#define	CFG_AUDIO_EFFECT_FREQ_SHIFTER_FINE_EN			(0)
#define	CFG_AUDIO_EFFECT_OVERDRIVE_EN		            (0)
#define	CFG_AUDIO_EFFECT_DISTORTION_EN					(0)
#define	CFG_AUDIO_EFFECT_EQDRC_EN						(1)
#define	CFG_AUDIO_EFFECT_AEC_EN							(1)
#define	CFG_AUDIO_EFFECT_DISTORTION_DS1_EN				(0)
#define	CFG_AUDIO_EFFECT_OVERDRIVE_POLY_EN				(0)
#define	CFG_AUDIO_EFFECT_COMPANDER_EN					(1)
#define	CFG_AUDIO_EFFECT_LOW_LEVEL_COMPRESSOR_EN		(1)
#define CFG_AUDIO_EFFECT_HOWLING_SUPPRESSOR_FINE_EN		(0)
#endif

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__AUDIO_EFFECT_USER_CONFIG_H__
