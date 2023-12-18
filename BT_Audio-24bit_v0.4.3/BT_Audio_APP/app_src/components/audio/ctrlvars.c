/**
 **************************************************************************************
 * @file    ctrlvars.c
 * @brief   Control Variables Definition
 * 
 * @author  Cecilia Wang
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include <string.h>
#include <stdint.h>
#include "debug.h"
#include "app_config.h"
#include "audio_effect_api.h"
#include "audio_effect_library.h"
#include "clk.h"
#include "ctrlvars.h"
#include "spi_flash.h"
#include "timeout.h"
#include "delay.h"
#include "breakpoint.h"
#include "nds32_intrinsic.h"
#include "audio_adc.h"
#include "dac.h"
#include "comm_param.h"


ControlVariablesContext gCtrlVars;

#ifdef CFG_EFFECT_PARAM_IN_FLASH_EN
const AUDIO_EFF_PARAMAS EFFECT_TAB_FLASH[CFG_EFFECT_PARAM_IN_FLASH_ACTIVCE_NUM]={
	{EFFECT_MODE_FLASH_HFP_AEC,   		NULL,   sizeof(AECParam_HFP),	(uint8_t *)AudioEffectAECParam_HFP,	"0:HFP_AEC_FLASH"},
	{EFFECT_MODE_FLASH_USBPHONE0_AEC,   NULL,	sizeof(AECParam_HFP),	(uint8_t *)AudioEffectAECParam_HFP,	"1:USBPHONE_AEC_FLASH"},//预留目前未使用
	{EFFECT_MODE_FLASH_Music,    	 	NULL, 	sizeof(CommParam), 		(uint8_t *)AudioEffectCommParam,	"2:Music_FLASH"},
	{EFFECT_MODE_FLASH_Movie,    	 	NULL, 	sizeof(CommParam), 		(uint8_t *)AudioEffectCommParam,	"3:Movie_FLASH"},
	{EFFECT_MODE_FLASH_News,    	 	NULL, 	sizeof(CommParam), 		(uint8_t *)AudioEffectCommParam,	"4:News_FLASH"},
	{EFFECT_MODE_FLASH_Game,    	 	NULL, 	sizeof(CommParam), 		(uint8_t *)AudioEffectCommParam,	"5:Game_FLASH"},
};
#endif

//当前SDK支持的音效参数列表
const AUDIO_EFF_PARAMAS EFFECT_TAB[EFFECT_MODE_NUM_ACTIVCE]={
	{EFFECT_MODE_NORMAL,          CommParam,      	sizeof(CommParam), 		(uint8_t *)AudioEffectCommParam,	"Music"},
#if (BT_HFP_SUPPORT == ENABLE)
	{EFFECT_MODE_HFP_AEC,         AECParam_HFP,     sizeof(AECParam_HFP),	(uint8_t *)AudioEffectAECParam_HFP,	"HFP_AEC"},
#endif
};

const uint8_t AutoTuneKeyTbl[13] ={ 
 'a','A','b','B','C','d','D','e','E','F','g','G','X',
};
 
const uint8_t AutoTuneSnapTbl[3] ={ 
'n','u','l',
};

const int16_t DeltafTable[8] = {
 -7,-5,-3,3,5,7,0,
};

const uint8_t MIC_BOOST_LIST[5]={
	4,//dis
	0,//0db
	1,//6db
	2,//12db
	3,//20db
};

const uint8_t PlateReverbRoomTab[16]={

	0,10,11,13,14,15,20,25,30,35,40,45,48,50,52,70,

};
const uint8_t ReverbRoomTab[16]={
	
	0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,80,

};
	
const int16_t EchoDlyTab_16[16] =
{
    0,		10,	    50,	    100,	120,	140,	160,	180,  
    200,	210,	220,	230,	240,	260,	280,	320,
};

const int16_t DigVolTab_16[16] =
{
	0,  	332,	460,	541,	636,	748,	880,	1035,  
	1218,	1433,	1686,	1984,	2434,	2946,	3500,	4096
};

const int16_t DigVolTab_32[32] =
{
	0,		31,		36,		42,		49,		58,		67,		78,
	91,		107,	125,	147,	173,	204,	240,	282,
	332,	391,	460,	541,	636,	748,	880,	1035,
	1218,	1433,	1686,	1984,	2434,	2946,	3500,	4096
};

const int16_t DigVolTab_64[64] =
{
      0,    3,    4,    4,    5,    5,    6,    6,    7,    8,    9,   10,   12,   13,   15,   16, 
     18,   21,   23,   26,   29,   33,   36,   41,   46,   52,   58,   65,   73,   82,   92,  103, 
    115,  129,  145,  163,  183,  205,  230,  258,  290,  325,  365,  410,  459,  516,  578,  649, 
    728,  817,  917, 1029, 1154, 1295, 1453, 1630, 1829, 2052, 2303, 2584, 2899, 3253, 3650, 4095, 
};

const uint16_t DigVolTab_256[256] =
{
    0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
    0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
    0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
    0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x001,
    0x001, 0x001, 0x001, 0x001, 0x001, 0x001, 0x001, 0x001, 0x001, 0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x002,
    0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x003, 0x003, 0x003, 0x003, 0x003, 0x003, 0x003, 0x003,
    0x003, 0x003, 0x003, 0x003, 0x004, 0x004, 0x004, 0x004, 0x004, 0x005, 0x005, 0x005, 0x006, 0x006, 0x006, 0x007,
    0x007, 0x008, 0x008, 0x009, 0x009, 0x00A, 0x00A, 0x00B, 0x00B, 0x00C, 0x00C, 0x00D, 0x00D, 0x00E, 0x00E, 0X00F,
    0x010, 0x011, 0x012, 0x013, 0x014, 0x015, 0x016, 0x017, 0x018, 0x019, 0x01a, 0x01b, 0x01d, 0x01e, 0x01f, 0x021,
    0x022, 0x023, 0x025, 0x027, 0x028, 0x02a, 0x02c, 0x02e, 0x030, 0x032, 0x034, 0x037, 0x039, 0x03c, 0x03e, 0x041,
    0x044, 0x047, 0x04a, 0x04d, 0x051, 0x054, 0x058, 0x05c, 0x060, 0x064, 0x068, 0x06d, 0x072, 0x077, 0x07c, 0x082,
    0x087, 0x08d, 0x093, 0x09a, 0x0a1, 0x0a8, 0x0af, 0x0b7, 0x0bf, 0x0c7, 0x0d0, 0x0d9, 0x0e3, 0x0ed, 0x0f8, 0x102,
    0x10e, 0x11a, 0x126, 0x133, 0x141, 0x14f, 0x15e, 0x16d, 0x17d, 0x18e, 0x1a0, 0x1b2, 0x1c5, 0x1d9, 0x1ee, 0x204,
    0x21a, 0x232, 0x24b, 0x265, 0x280, 0x29c, 0x2ba, 0x2d8, 0x2f9, 0x31a, 0x33d, 0x362, 0x388, 0x3b0, 0x3d9, 0x405,
    0x432, 0x462, 0x493, 0x4c7, 0x4fd, 0x535, 0x570, 0x5ad, 0x5ed, 0x630, 0x676, 0x6bf, 0x70b, 0x75b, 0x7ae, 0x805,
    0x85f, 0x8be, 0x921, 0x988, 0x9f3, 0xa64, 0xad9, 0xb54, 0xbd4, 0xc59, 0xce5, 0xd76, 0xe0e, 0xead, 0xf53, 0xfff
};

const int16_t BassTrebGainTable[16] =
{
    0xf900, //-7dB
    0xfa00, //-6dB
    0xfb00, //-5dB
    0xfc00, //-4dB
    0xfd00, //-3dB
    0xfe00, //-2dB
    0xff00, //-1dB
    0x0000, //+0dB
    0x0100, //+1dB
    0x0200, //+2dB
    0x0300, //+3dB
    0x0400, //+4dB
    0x0500, //+5dB
    0x0600, //+6dB
    0x0700, //+7dB
    0x0800, //+8dB
};

const uint32_t SupportSampleRateList[13]={
	8000,
    11025,
    12000,
    16000,
    22050,
    24000,
    32000,
    44100,
    48000,
//////i2s//////////////////
    88200,
    96000,
    176400,
    192000,
};
extern const uint16_t HPCList[3];

uint16_t SampleRateIndexGet(uint32_t SampleRate)
{
	volatile uint32_t i;
	for(i = 0; i < sizeof(SupportSampleRateList)/sizeof(SupportSampleRateList[0]); i++)
	{
		if(SampleRate == SupportSampleRateList[i])
		{
			break;
		}
	}
	if(i == 13)
	{
		i =0;
	}
	return i;
}
//系统变量初始化
void CtrlVarsInit(void)
{
	APP_DBG("[SYS]: Loading control vars as default\n");

	//音频系统硬件模块变量初始化
	DefaultParamgsInit();

	//用户特殊音效参数默认值初始化
	//DefaultParamgsUserInit();
	//从flash固定区域导出硬件参数并验证数据合法性
	//
}

//各个模块默认参数设置函数
void DefaultParamgsInit(void)
{
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	memset(&gCtrlVars,  0, sizeof(gCtrlVars));
	//for system control 0x01
	gCtrlVars.AutoRefresh         = 1;//调音时音效参数发生改变，上位机会自动读取音效数据，1=允许上位读，0=不需要上位机读取

	//for ADC0 PGA      0x03
    gCtrlVars.ADC0PGACt.pga0_line1_l_en     = 0;
    gCtrlVars.ADC0PGACt.pga0_line1_r_en     = 0;
    gCtrlVars.ADC0PGACt.pga0_line2_l_en     = 0;
    gCtrlVars.ADC0PGACt.pga0_line2_r_en     = 0;
    gCtrlVars.ADC0PGACt.pga0_line4_l_en     = 0;
    gCtrlVars.ADC0PGACt.pga0_line4_r_en     = 0;
    gCtrlVars.ADC0PGACt.pga0_line5_l_en     = 0;
    gCtrlVars.ADC0PGACt.pga0_line5_r_en     = 0;
    gCtrlVars.ADC0PGACt.pga0_line1_l_gain = 17;//0DB//63;
    gCtrlVars.ADC0PGACt.pga0_line1_r_gain = 17;//0DB//63;
    gCtrlVars.ADC0PGACt.pga0_line2_l_gain = 17;//0DB//63;
    gCtrlVars.ADC0PGACt.pga0_line2_r_gain = 17;//0DB//63;
    gCtrlVars.ADC0PGACt.pga0_line4_l_gain = 19;//0DB//63;
    gCtrlVars.ADC0PGACt.pga0_line4_r_gain = 19;//0DB//63;
    gCtrlVars.ADC0PGACt.pga0_line5_l_gain = 19;//0DB//63;
    gCtrlVars.ADC0PGACt.pga0_line5_r_gain = 19;//0DB//63;
    gCtrlVars.ADC0PGACt.pga0_diff_mode = 0;
    gCtrlVars.ADC0PGACt.pga0_diff_l_gain = 0;
    gCtrlVars.ADC0PGACt.pga0_diff_r_gain = 0;
	
    //for ADC0 DIGITAL  0x04
    gCtrlVars.ADC0DigitalCt.adc0_channel_en = 3;
    gCtrlVars.ADC0DigitalCt.adc0_mute = 0;
    gCtrlVars.ADC0DigitalCt.adc0_dig_l_vol = 0x1000;
    gCtrlVars.ADC0DigitalCt.adc0_dig_r_vol = 0x1000;
    gCtrlVars.ADC0DigitalCt.adc0_lr_swap = 0;
    gCtrlVars.ADC0DigitalCt.adc0_dc_blocker = 0;
    gCtrlVars.ADC0DigitalCt.adc0_dc_blocker_en = 1;
	//for AGC0 ADC0     0x05
    //BP10无该参数

	//for ADC1 PGA      0x06
    gCtrlVars.ADC1PGACt.line3_l_mic1_en = 0;
    gCtrlVars.ADC1PGACt.line3_r_mic2_en = 0;
    gCtrlVars.ADC1PGACt.line3_l_mic1_gain = 31;
    gCtrlVars.ADC1PGACt.line3_l_mic1_boost = 0;
    gCtrlVars.ADC1PGACt.line3_r_mic2_gain = 31;
    gCtrlVars.ADC1PGACt.line3_r_mic2_boost = 0;
    //0= mic1,mic2;1 = line3; 2 = mic1,line3_r;3 = mic2,line3_l,仅仅用于和上位机显示
    gCtrlVars.ADC1PGACt.mic_or_line3 = 0;

	//for ADC1 DIGITAL  0x07
    gCtrlVars.ADC1DigitalCt.adc1_channel_en = 3;
    gCtrlVars.ADC1DigitalCt.adc1_mute = 0;
    gCtrlVars.ADC1DigitalCt.adc1_dig_l_vol = 0x1000;
    gCtrlVars.ADC1DigitalCt.adc1_dig_r_vol = 0x1000;
    gCtrlVars.ADC1DigitalCt.adc1_lr_swap = 0;
    gCtrlVars.ADC1DigitalCt.adc1_dc_blocker = 0;
    gCtrlVars.ADC1DigitalCt.adc1_dc_blocker_en = 1;

	//for AGC1  ADC1    0x08
    gCtrlVars.ADC1AGCCt.adc1_agc_mode = 0;
    gCtrlVars.ADC1AGCCt.adc1_agc_max_level = 0;
    gCtrlVars.ADC1AGCCt.adc1_agc_target_level = 0;
    gCtrlVars.ADC1AGCCt.adc1_agc_max_gain = 0;
    gCtrlVars.ADC1AGCCt.adc1_agc_min_gain = 0;
    gCtrlVars.ADC1AGCCt.adc1_agc_gainoffset = 0;
    gCtrlVars.ADC1AGCCt.adc1_agc_fram_time = 10;
    gCtrlVars.ADC1AGCCt.adc1_agc_hold_time = 10;
    gCtrlVars.ADC1AGCCt.adc1_agc_attack_time = 10;
    gCtrlVars.ADC1AGCCt.adc1_agc_decay_time = 10;
    gCtrlVars.ADC1AGCCt.adc1_agc_noise_gate_en = 0;
    gCtrlVars.ADC1AGCCt.adc1_agc_noise_threshold = 5;
    gCtrlVars.ADC1AGCCt.adc1_agc_noise_gate_mode = 0;
    gCtrlVars.ADC1AGCCt.adc1_agc_noise_time = 0;

	//for DAC0          0x09
    gCtrlVars.DAC0Ct.dac0_enable = 3;
    gCtrlVars.DAC0Ct.dac0_dig_mute = 0;
    gCtrlVars.DAC0Ct.dac0_dig_l_vol = 0x1000;
    gCtrlVars.DAC0Ct.dac0_dig_r_vol = 0x1000;
    gCtrlVars.DAC0Ct.dac0_dither = 0;
    gCtrlVars.DAC0Ct.dac0_scramble = 0;
    gCtrlVars.DAC0Ct.dac0_out_mode = 0;

	//for DAC1          0x0a
    gCtrlVars.DAC1Ct.dac1_enable = 1;
    gCtrlVars.DAC1Ct.dac1_dig_mute = 0;
    gCtrlVars.DAC1Ct.dac1_dig_l_vol = 0x1000;
    gCtrlVars.DAC1Ct.dac1_dither = 0;
    gCtrlVars.DAC1Ct.dac1_scramble = 0;

	//for i2s0          0x0b
	//for i2s1          0x0c
    //SPDIF

    //system define
	gCtrlVars.sample_rate		= CFG_PARA_SAMPLE_RATE;
	gCtrlVars.SamplesPerFrame	= CFG_PARA_SAMPLES_PER_FRAME;
	gCtrlVars.sample_rate_index = SampleRateIndexGet(gCtrlVars.sample_rate);
#endif
}

void AudioLineSelSet(void)
{
	//模拟通道先配置为NONE，防止上次配置通道残留，然后再配置需要的模拟通道 

	AudioADC_PGASel(ADC0_MODULE, CHANNEL_LEFT,  LINEIN_NONE);
	AudioADC_PGASel(ADC0_MODULE, CHANNEL_RIGHT, LINEIN_NONE);

    //AudioADC_AnaInit();
	//AudioADC_VcomConfig(1);//MicBias en
	//AudioADC_MicBias1Enable(1);
	//AudioADC_DynamicElementMatch(ADC0_MODULE, TRUE, TRUE);

	//--------------------line 1-----------------------------------------//
	if(gCtrlVars.ADC0PGACt.pga0_line1_l_en)
	{
		//APP_DBG("LINE 1L En\n");    	 		  
		AudioADC_PGASel(ADC0_MODULE, CHANNEL_LEFT,LINEIN1_LEFT);
		AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_LEFT,  LINEIN1_LEFT,  31 - gCtrlVars.ADC0PGACt.pga0_line1_l_gain, 4);
	}
	if(gCtrlVars.ADC0PGACt.pga0_line1_r_en)
	{
		//APP_DBG("LINE 1R En\n");
		AudioADC_PGASel(ADC0_MODULE, CHANNEL_RIGHT,LINEIN1_RIGHT);
		AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_RIGHT, LINEIN1_RIGHT,  31 - gCtrlVars.ADC0PGACt.pga0_line1_r_gain, 4);
	}
	//---------------line 2-------------------------------------------//
	if(gCtrlVars.ADC0PGACt.pga0_line2_l_en)
	{
		// APP_DBG("LINE 2L En\n");		
		AudioADC_PGASel(ADC0_MODULE, CHANNEL_LEFT,LINEIN2_LEFT);
		AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_LEFT,  LINEIN2_LEFT,  31 - gCtrlVars.ADC0PGACt.pga0_line2_l_gain, 4);
	}
	if(gCtrlVars.ADC0PGACt.pga0_line2_r_en)
	{
		//APP_DBG("LINE 2R En\n");		
		AudioADC_PGASel(ADC0_MODULE, CHANNEL_RIGHT,LINEIN2_RIGHT);
		AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_RIGHT, LINEIN2_RIGHT,  31 - gCtrlVars.ADC0PGACt.pga0_line2_r_gain, 4);
	}
	//----------------line 4------------------------------------------//
	if(gCtrlVars.ADC0PGACt.pga0_line4_l_en)
	{
		//APP_DBG("LINE4_L En\n");
		#ifndef CFG_FUNC_SW_DEBUG_EN
		GPIO_PortBModeSet(GPIOB0, 6);
		#endif
		AudioADC_PGASel(ADC0_MODULE, CHANNEL_LEFT,LINEIN4_LEFT);
		AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_LEFT, LINEIN4_LEFT,  63 - gCtrlVars.ADC0PGACt.pga0_line4_l_gain, 4);
	}
	if(gCtrlVars.ADC0PGACt.pga0_line4_r_en)
	{
		//APP_DBG("LINE4_R En\n");
		#ifndef CFG_FUNC_SW_DEBUG_EN
		GPIO_PortBModeSet(GPIOB1,6);
		#endif
		AudioADC_PGASel(ADC0_MODULE, CHANNEL_RIGHT,LINEIN4_RIGHT);
		AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_RIGHT, LINEIN4_RIGHT,  63 - gCtrlVars.ADC0PGACt.pga0_line4_r_gain, 4);
	}
	//-----------------line 5--------------------------------------------//
	if(gCtrlVars.ADC0PGACt.pga0_line5_l_en)
	{
		//APP_DBG("LINE5_L En\n");  
		GPIO_PortBModeSet(GPIOB3, 5);
		AudioADC_PGASel(ADC0_MODULE, CHANNEL_LEFT,LINEIN5_LEFT);
		AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_LEFT, LINEIN5_LEFT,  63 - gCtrlVars.ADC0PGACt.pga0_line5_l_gain, 4);
	}
	if(gCtrlVars.ADC0PGACt.pga0_line5_r_en)
	{
		//APP_DBG("LINE5_R En\n");  
		GPIO_PortBModeSet(GPIOB2, 5);
		AudioADC_PGASel(ADC0_MODULE, CHANNEL_RIGHT,LINEIN5_RIGHT);
		AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_RIGHT, LINEIN5_RIGHT,  63 - gCtrlVars.ADC0PGACt.pga0_line5_r_gain, 4);
	}
	//APP_DBG("channel: %08lx\n ",*ch_reg);
	//APP_DBG("gain: %08lx\n ",*gain_reg);
}

void AudioLine3MicSelect(void)
{
	if(gCtrlVars.ADC1PGACt.line3_l_mic1_en)
	{
		AudioADC_PGASel(ADC1_MODULE, CHANNEL_LEFT,LINEIN3_LEFT_OR_MIC1);
		AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_LEFT,  LINEIN3_LEFT_OR_MIC1, 31 - gCtrlVars.ADC1PGACt.line3_l_mic1_gain, MIC_BOOST_LIST[gCtrlVars.ADC1PGACt.line3_l_mic1_boost]);//0db bypass
	}
	else
	{
		AudioADC_PGASel(ADC1_MODULE, CHANNEL_LEFT,LINEIN_NONE);
	}

	if(gCtrlVars.ADC1PGACt.line3_r_mic2_en)
	{
		AudioADC_PGASel(ADC1_MODULE, CHANNEL_RIGHT,LINEIN3_RIGHT_OR_MIC2);
		AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2,  31 - gCtrlVars.ADC1PGACt.line3_r_mic2_gain, MIC_BOOST_LIST[gCtrlVars.ADC1PGACt.line3_r_mic2_boost]);
	}
	else
	{
		AudioADC_PGASel(ADC1_MODULE, CHANNEL_RIGHT,LINEIN_NONE);
	}
}

void AudioAnaChannelSet(int8_t ana_input_ch)
{
	gCtrlVars.ADC0PGACt.pga0_line1_l_en = 0;
	gCtrlVars.ADC0PGACt.pga0_line1_r_en = 0;
	gCtrlVars.ADC0PGACt.pga0_line2_l_en = 0;
	gCtrlVars.ADC0PGACt.pga0_line2_r_en = 0;
	#if (CFG_RES_MIC_SELECT == 0)//作为linein3应用
	gCtrlVars.ADC1PGACt.line3_l_mic1_en = 0;
	gCtrlVars.ADC1PGACt.line3_r_mic2_en = 0;
	#endif
	gCtrlVars.ADC0PGACt.pga0_line4_l_en = 0;
	gCtrlVars.ADC0PGACt.pga0_line4_r_en = 0;
	gCtrlVars.ADC0PGACt.pga0_line5_l_en = 0;
	gCtrlVars.ADC0PGACt.pga0_line5_r_en = 0;

	//gCtrlVars.line_num        = ana_input_ch;

	if(ANA_INPUT_CH_LINEIN1 == ana_input_ch)
	{
		gCtrlVars.ADC0PGACt.pga0_line1_l_en = 1;
		gCtrlVars.ADC0PGACt.pga0_line1_r_en = 1;
		AudioLineSelSet();
	}

	if(ANA_INPUT_CH_LINEIN2 == ana_input_ch)
	{
		gCtrlVars.ADC0PGACt.pga0_line2_l_en = 1;
		gCtrlVars.ADC0PGACt.pga0_line2_r_en = 1;
		AudioLineSelSet();
	}
	
    #if (CFG_RES_MIC_SELECT == 0)//作为linein3应用
	if(ANA_INPUT_CH_LINEIN3 == ana_input_ch)
	{
		gCtrlVars.ADC1PGACt.line3_l_mic1_en = 1;
		gCtrlVars.ADC1PGACt.line3_r_mic2_en = 1;
		AudioLine3MicSelect();
	}
	#endif
	
	if(ANA_INPUT_CH_LINEIN4 == ana_input_ch)
	{
		gCtrlVars.ADC0PGACt.pga0_line4_l_en = 1;
		gCtrlVars.ADC0PGACt.pga0_line4_r_en = 1;
		AudioLineSelSet();
	}
	
	if(ANA_INPUT_CH_LINEIN5 == ana_input_ch)
	{
		gCtrlVars.ADC0PGACt.pga0_line5_l_en = 1;
		gCtrlVars.ADC0PGACt.pga0_line5_r_en = 1;
		AudioLineSelSet();
	}	
}

//音效参数更新之后同步更新模拟Gain和数字Vol
//只更新增益相关参数，其他参数比如通道选择不会同步更新，必须由SDK代码来实现
void AudioCodecGainUpdata(void)
{
	AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_LEFT,  LINEIN1_LEFT,  31 - gCtrlVars.ADC0PGACt.pga0_line1_l_gain, 4);
	AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_RIGHT, LINEIN1_RIGHT,  31 - gCtrlVars.ADC0PGACt.pga0_line1_r_gain, 4);
	AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_LEFT,  LINEIN2_LEFT,  31 - gCtrlVars.ADC0PGACt.pga0_line2_l_gain, 4);
	AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_RIGHT, LINEIN2_RIGHT,  31 - gCtrlVars.ADC0PGACt.pga0_line2_r_gain, 4);
	AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_LEFT,  LINEIN3_LEFT_OR_MIC1, 31 - gCtrlVars.ADC1PGACt.line3_l_mic1_gain, MIC_BOOST_LIST[gCtrlVars.ADC1PGACt.line3_l_mic1_boost]);//0db bypass
	AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2,  31 - gCtrlVars.ADC1PGACt.line3_r_mic2_gain, MIC_BOOST_LIST[gCtrlVars.ADC1PGACt.line3_r_mic2_boost]);
	AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_LEFT, LINEIN4_LEFT,  63 - gCtrlVars.ADC0PGACt.pga0_line4_l_gain, 4);
	AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_RIGHT, LINEIN4_RIGHT,  63 - gCtrlVars.ADC0PGACt.pga0_line4_r_gain, 4);
	AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_LEFT, LINEIN5_LEFT,  63 - gCtrlVars.ADC0PGACt.pga0_line5_l_gain, 4);
	AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_RIGHT, LINEIN5_RIGHT,  63 - gCtrlVars.ADC0PGACt.pga0_line5_r_gain, 4);

	AudioADC_HighPassFilterConfig(ADC0_MODULE, HPCList[gCtrlVars.ADC0DigitalCt.adc0_dc_blocker]);
	AudioADC_HighPassFilterSet(ADC0_MODULE, gCtrlVars.ADC0DigitalCt.adc0_dc_blocker_en);
	AudioADC_ChannelSwap(ADC0_MODULE, gCtrlVars.ADC0DigitalCt.adc0_lr_swap);
	AudioADC_VolSet(ADC0_MODULE, gCtrlVars.ADC0DigitalCt.adc0_dig_l_vol, gCtrlVars.ADC0DigitalCt.adc0_dig_l_vol);
	AudioADC_ChannelSwap(ADC0_MODULE, gCtrlVars.ADC0DigitalCt.adc0_mute);

	AudioADC_HighPassFilterConfig(ADC1_MODULE, HPCList[gCtrlVars.ADC1DigitalCt.adc1_dc_blocker]);
	AudioADC_HighPassFilterSet(ADC1_MODULE, gCtrlVars.ADC1DigitalCt.adc1_dc_blocker_en);
	AudioADC_ChannelSwap(ADC1_MODULE, gCtrlVars.ADC1DigitalCt.adc1_lr_swap);
	AudioADC_VolSet(ADC1_MODULE, gCtrlVars.ADC1DigitalCt.adc1_dig_l_vol, gCtrlVars.ADC1DigitalCt.adc1_dig_l_vol);
	AudioADC_ChannelSwap(ADC1_MODULE, gCtrlVars.ADC1DigitalCt.adc1_mute);

	AudioDAC_VolSet(DAC0, gCtrlVars.DAC0Ct.dac0_dig_l_vol, gCtrlVars.DAC0Ct.dac0_dig_r_vol);
#ifndef CFG_AUDIO_WIDTH_24BIT
	AudioDAC_DoutModeSet(DAC0, gCtrlVars.DAC0Ct.dac0_out_mode, WIDTH_16_BIT);
#else
	AudioDAC_DoutModeSet(DAC0, gCtrlVars.DAC0Ct.dac0_out_mode, WIDTH_24_BIT_2);
#endif
	if(gCtrlVars.DAC0Ct.dac0_scramble)
	{
		AudioDAC_ScrambleEnable(DAC0);
	}
	else
	{
		AudioDAC_ScrambleDisable(DAC0);
	}
	if(gCtrlVars.DAC0Ct.dac0_dither)
	{
		AudioDAC_DitherEnable(DAC0);
	}
	else
	{
		AudioDAC_DitherDisable(DAC0);
	}

	AudioDAC_VolSet(DAC1, gCtrlVars.DAC1Ct.dac1_dig_l_vol, 0xFFF);
	if(gCtrlVars.DAC1Ct.dac1_scramble)
	{
		AudioDAC_ScrambleEnable(DAC1);
	}
	else
	{
		AudioDAC_ScrambleDisable(DAC1);
	}
	if(gCtrlVars.DAC1Ct.dac1_dither)
	{
		AudioDAC_DitherEnable(DAC1);
	}
	else
	{
		AudioDAC_DitherDisable(DAC1);
	}
}
