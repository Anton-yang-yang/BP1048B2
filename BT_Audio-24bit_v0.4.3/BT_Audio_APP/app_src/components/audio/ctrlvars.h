/**
 **************************************************************************************
 * @file    ctrlvars.h
 * @brief   Control Variables Definition
 * 
 * @author  Aissen Li
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
 
#ifndef __CTRLVARS_H__
#define __CTRLVARS_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include <stdint.h>
#include "audio_effect_api.h"
#include "audio_effect_library.h"
#include "app_config.h"
#include "bt_config.h"
#include "audio_core_api.h"
#include "blue_aec.h"
#include "vocalcut.h"

#define  CTRL_VARS_SYNC_WORD            (0x43564347)

#define  MIN_VOLUME                     (0)
#define  MAX_VOLUME                     (63)
#define  VOLUME_COUNT                   (MAX_VOLUME - MIN_VOLUME + 1)
#define  MIN_BASS_TREB_GAIN             (0)
#define  MAX_BASS_TREB_GAIN             (15)
#define  MIN_MIC_DIG_STEP               (0)
#define  MAX_MIC_DIG_STEP               (32)
#define  MIN_MIC_EFFECT_DELAY_STEP      (0)
#define  MAX_MIC_EFFECT_DELAY_STEP      (32)
#define  MIN_MUSIC_DIG_STEP             (0)
#define  MAX_MUSIC_DIG_STEP             (32)
#define  BASS_TREB_GAIN_STEP            (1)
#define  MIN_AUTOTUNE_STEP              (0)
#define  MAX_AUTOTUNE_STEP              (11)

	
typedef enum _EFFECT_MODE
{
	//0-9的音效编号存储于flash固定区域中
	EFFECT_MODE_FLASH_HFP_AEC = 0,
	EFFECT_MODE_FLASH_USBPHONE0_AEC = 1,
	EFFECT_MODE_FLASH_Music=2,
	EFFECT_MODE_FLASH_Movie,
	EFFECT_MODE_FLASH_News,
	EFFECT_MODE_FLASH_Game=5,
//	EFFECT_MODE_FLASH_NORMAL6,
//	EFFECT_MODE_FLASH_NORMAL7,
//	EFFECT_MODE_FLASH_NORMAL8,
//	EFFECT_MODE_FLASH_NORMAL9,
    EFFECT_MODE_NORMAL=10,//固化到代码中的第一个音效
	EFFECT_MODE_ECHO, 
	EFFECT_MODE_REVERB,
	EFFECT_MODE_ECHO_REVERB,
    EFFECT_MODE_PITCH_SHIFTER,    
    EFFECT_MODE_VOICE_CHANGER,   
    EFFECT_MODE_AUTO_TUNE, 
    EFFECT_MODE_BOY,
    EFFECT_MODE_GIRL,
    EFFECT_MODE_KTV,
    EFFECT_MODE_BaoYin,
    EFFECT_MODE_HunXiang,    
    EFFECT_MODE_DianYin,
    EFFECT_MODE_MoYin,
    EFFECT_MODE_HanMai,
    EFFECT_MODE_NanBianNv,
    EFFECT_MODE_NvBianNan,
    EFFECT_MODE_WaWaYin,   
    #ifdef BT_TWS_SUPPORT
	EFFECT_MODE_NORMAL_SLAVE,
	EFFECT_MODE_HunXiang_Slave,    
    EFFECT_MODE_DianYin_Slave,
    EFFECT_MODE_MoYin_Slave,
    EFFECT_MODE_HanMai_Slave,
    EFFECT_MODE_NanBianNv_Slave,
    EFFECT_MODE_NvBianNan_Slave,
    EFFECT_MODE_WaWaYin_Slave,
	#endif
	EFFECT_MODE_YuanSheng,
	EFFECT_MODE_HFP_AEC,
    //User can add other effect mode
} EFFECT_MODE;

//实际有效的音效轮转模式数量
#if (BT_HFP_SUPPORT == ENABLE)
#define	EFFECT_MODE_NUM_ACTIVCE 	2
#else
#define	EFFECT_MODE_NUM_ACTIVCE 	1
#endif

typedef enum _REMIND_TYPE
{
    REMIND_TYPE_KEY,
	REMIND_TYPE_BACKGROUND, 

} REMIND_TYPE;

typedef enum _EQ_MODE
{
    EQ_MODE_FLAT,
	EQ_MODE_CLASSIC,	
	EQ_MODE_POP,
	EQ_MODE_ROCK,
	EQ_MODE_JAZZ,
	EQ_MODE_VOCAL_BOOST,
} EQ_MODE;


#define ANA_INPUT_CH_NONE    0
#define ANA_INPUT_CH_LINEIN1 1
#define ANA_INPUT_CH_LINEIN2 2
#define ANA_INPUT_CH_LINEIN3 3
#define ANA_INPUT_CH_LINEIN4 4
#define ANA_INPUT_CH_LINEIN5 5

//原音效多组参数
typedef struct
{
	uint8_t       	eff_mode;
	const uint8_t*	EffectParamas;
	uint16_t	  	len;
	const uint8_t*	CommParam;
	const char* 	EffectNameStr;
}AUDIO_EFF_PARAMAS;

//for ADC0 PGA      0x03
typedef struct _ADC0PGAContext
{
    uint16_t            pga0_line1_l_en;
    uint16_t            pga0_line1_r_en;
    uint16_t            pga0_line2_l_en;
    uint16_t            pga0_line2_r_en;
    uint16_t            pga0_line4_l_en;
    uint16_t            pga0_line4_r_en;
    uint16_t            pga0_line5_l_en;
    uint16_t            pga0_line5_r_en;
    uint16_t            pga0_line1_l_gain;
    uint16_t            pga0_line1_r_gain;
    uint16_t            pga0_line2_l_gain;
    uint16_t            pga0_line2_r_gain;
    uint16_t            pga0_line4_l_gain;
    uint16_t            pga0_line4_r_gain;
    uint16_t            pga0_line5_l_gain;
    uint16_t            pga0_line5_r_gain;
	uint16_t            pga0_diff_mode;
	uint16_t            pga0_diff_l_gain;
	uint16_t            pga0_diff_r_gain;
}ADC0PGAContext;

//for ADC0 DIGITAL  0x04
typedef struct _ADC0DigitalContext
{
    uint16_t         	adc0_channel_en;
	uint16_t  			adc0_mute;
    uint16_t  			adc0_dig_l_vol;
    uint16_t  			adc0_dig_r_vol;
	uint16_t  			adc0_lr_swap;
	uint16_t            adc0_dc_blocker;
	uint16_t            adc0_dc_blocker_en;
}ADC0DigitalContext;

typedef struct _ADC1PGAContext
{
	uint16_t             line3_l_mic1_en;
	uint16_t             line3_r_mic2_en;
    uint16_t             line3_l_mic1_gain;
	uint16_t             line3_l_mic1_boost;
    uint16_t             line3_r_mic2_gain;
    uint16_t             line3_r_mic2_boost;
    uint16_t             mic_or_line3;
}ADC1PGAContext;

typedef struct _ADC1DigitalContext
{
    uint16_t         	adc1_channel_en;
	uint16_t  			adc1_mute;
    uint16_t  			adc1_dig_l_vol;
    uint16_t  			adc1_dig_r_vol;
	uint16_t  			adc1_lr_swap;
	uint16_t            adc1_dc_blocker;
	uint16_t            adc1_dc_blocker_en;
}ADC1DigitalContext;

typedef struct _ADC1AGCContext
{
	uint16_t            adc1_agc_mode;
	uint16_t            adc1_agc_max_level;
	uint16_t            adc1_agc_target_level;
	uint16_t            adc1_agc_max_gain;
	uint16_t            adc1_agc_min_gain;
	uint16_t            adc1_agc_gainoffset;
	uint16_t            adc1_agc_fram_time;
	uint16_t            adc1_agc_hold_time;
	uint16_t            adc1_agc_attack_time;
	uint16_t            adc1_agc_decay_time;
	uint16_t            adc1_agc_noise_gate_en;
	uint16_t            adc1_agc_noise_threshold;
	uint16_t            adc1_agc_noise_gate_mode;
	uint16_t            adc1_agc_noise_time;
}ADC1AGCContext;

typedef struct _DAC0Context
{
	uint16_t            dac0_enable;
	uint16_t            dac0_dig_mute;
	uint16_t            dac0_dig_l_vol;
	uint16_t            dac0_dig_r_vol;
	uint16_t            dac0_dither;
	uint16_t            dac0_scramble;
	uint16_t            dac0_out_mode;
}DAC0Context;

typedef struct _DAC1Context
{
	uint16_t            dac1_enable;
	uint16_t            dac1_dig_mute;
	uint16_t            dac1_dig_l_vol;
	uint16_t            dac1_dither;
	uint16_t            dac1_scramble;
	//uint16_t            dac1_out_mode;
}DAC1Context;

//-----system var--------------------------//
typedef struct _ControlVariablesContext
{
	//for system control 0x01
	//for System status 0x02
	uint16_t 			AutoRefresh;//调音时音效参数发生改变，上位机会自动读取音效数据，1=允许上位读，0=不需要上位机读取

	//for ADC0 PGA      0x03
	ADC0PGAContext		ADC0PGACt;
    //for ADC0 DIGITAL  0x04
	ADC0DigitalContext	ADC0DigitalCt;
	
	//for AGC0 ADC0     0x05
	//BP10无该参数
	
	//for ADC1 PGA      0x06 
	ADC1PGAContext		ADC1PGACt;

	//for ADC1 DIGITAL  0x07
	ADC1DigitalContext	ADC1DigitalCt;

	//for AGC1  ADC1    0x08
	ADC1AGCContext		ADC1AGCCt;

	//for DAC0          0x09
	DAC0Context			DAC0Ct;
	
	//for DAC1          0x0a
	DAC1Context			DAC1Ct;

    //for system define

	uint16_t            SamplesPerFrame;
	uint16_t            sample_rate_index;
	uint32_t            sample_rate;

	#ifdef CFG_FUNC_DETECT_MIC_SEG_EN
	uint8_t             MicSegment;
	#endif
	#ifdef CFG_FUNC_DETECT_PHONE_EN
	uint8_t             EarPhoneOnlin;
	#endif
}ControlVariablesContext;

extern ControlVariablesContext gCtrlVars;

extern const uint8_t MIC_BOOST_LIST[5];

void CtrlVarsInit(void);
void DefaultParamgsInit(void);
void Line3MicPinSet(void);
void UsbLoadAudioMode(uint16_t len,uint8_t *buff);
void AudioLineSelSet(void);
void AudioAnaChannelSet(int8_t ana_input_ch);
void AudioLine3MicSelect(void);

//音效参数更新之后同步更新模拟Gain和数字Vol
//只更新增益相关参数，其他参数比如通道选择不会同步更新，必须由SDK代码来实现
void AudioCodecGainUpdata(void);

extern const uint8_t AutoTuneKeyTbl[13];
extern const uint8_t AutoTuneSnapTbl[3];
extern const int16_t DeltafTable[8];
extern const AUDIO_EFF_PARAMAS EFFECT_TAB[EFFECT_MODE_NUM_ACTIVCE];
#ifdef CFG_EFFECT_PARAM_IN_FLASH_EN
extern const AUDIO_EFF_PARAMAS EFFECT_TAB_FLASH[CFG_EFFECT_PARAM_IN_FLASH_ACTIVCE_NUM];
#endif

#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
void MusicBassTrebAjust(int16_t 	BassGain,	int16_t TrebGain);
#endif
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
extern const unsigned char Eq_Flat[];
extern const unsigned char Eq_Classic[];
extern const unsigned char Eq_Pop[];
extern const unsigned char Eq_Rock[];
extern const unsigned char Eq_Jazz[];
extern const unsigned char Eq_VocalBoost[];
void LoadEqMode(const uint8_t *buff);
#endif

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__CTRLVARS_H__

