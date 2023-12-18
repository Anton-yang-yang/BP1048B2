#include <stdint.h>
#include <string.h>
#include <nds32_intrinsic.h>
#include "uarts.h"
#include "dma.h"
#include "uarts_interface.h"
#include "timeout.h"
#include "debug.h"
#include "app_config.h"
#include "i2s.h"
#include "i2s_interface.h"
#include "clk.h"
#include "ctrlvars.h"
#include "audio_effect_api.h"
#include "audio_adc.h"
#include "dac.h"
#include "communication.h"
#include "audio_effect_library.h"
#include "rtos_api.h"
#include "watchdog.h"
#include "delay.h"
#include "main_task.h"
#include "audio_effect.h"
#include "audio_effect_api.h"
#include "comm_param.h"
#include "audio_effect_flash_param.h"

#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
uint8_t  EqMode_Addr; 		
#endif

bool IsEffectChange = 0;
bool IsEffectChangeReload = 0;

const uint16_t HPCList[3]={
	0xffe, //  48k 20Hz  -1.5db
	0xFFC, //  48k 40Hz  -1.5db
	0xFFD, //  32k 40Hz  -1.5db
};

#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
EffectComCt* AudioEffectParamCtAddr = NULL;
char TagBuf[32];//max = 32
uint8_t lenTag;
extern uint8_t  hid_tx_buf[];
extern uint32_t SysemMipsPercent;
#ifdef FUNC_OS_EN
osMutexId LoadAudioParamMutex = NULL;
#endif

#define  CTL_DATA_SIZE   2

uint8_t  tx_buf[256]     = {0xa5, 0x5a, 0x00, 0x00,};


//在线调音
uint8_t  effect_sum		 = 0;
uint16_t effect_list[AUDIO_EFFECT_SUM] = {0x0};
uint16_t effect_list_addr[AUDIO_EFFECT_SUM] = {0x0};
void* effect_list_param[AUDIO_EFFECT_SUM];
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
uint8_t effect_enable_list[AUDIO_EFFECT_SUM] = {0};
#endif

uint8_t  communic_buf[512];
uint32_t communic_buf_w = 0;
uint8_t  cbuf[8];

bool IsEffectSampleLenChange = 0;
//bool IsEffectChange = 1;

void Communication_Effect_Send(uint8_t *buf, uint32_t len)
{
#ifdef CFG_COMMUNICATION_BY_USB
	memcpy(hid_tx_buf, buf, 256);
#endif
}

void GetAudioLibVer(uint8_t *Buf)
{
	const char *AudioLibVer =  AUDIO_EFFECT_LIBRARY_VERSION;
	const char *ver;
	uint8_t count;
	ver = AudioLibVer;
	//------------------------------------//
	count = 0;
	while(*ver != '\0')
	{
	  if(*ver == '.')
	  {
		  count++;
		  ver++;
		  continue;
	  }
	  Buf[count] *= 10;
	  Buf[count] += (*ver-0x30);
	  ver++;
	}
}

void Communication_Effect_0x00(void)
{
    memset(tx_buf, 0, sizeof(tx_buf));
	tx_buf[0]  = 0xa5;
	tx_buf[1]  = 0x5a;
	tx_buf[2]  = 0x00;
	tx_buf[3]  = 0x07;
	tx_buf[4]  = 0x30;//20=kmic 21=O26  30=B1X
	tx_buf[5]  = CFG_EFFECT_MAJOR_VERSION;
	tx_buf[6]  = CFG_EFFECT_MINOR_VERSION;
	tx_buf[7]  = CFG_EFFECT_USER_VERSION;
	GetAudioLibVer(&tx_buf[8]);
	//-----------------------------------//
	tx_buf[11] = 0x16;
	Communication_Effect_Send(tx_buf, 12);
}

void Communication_Effect_0x01(uint8_t *buf, uint32_t len)
{
	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x01;
		tx_buf[3]  = 1+7*2;
		tx_buf[4]  = 0xff;
		tx_buf[10]  = 0x1;//System Sample Rate Enable
		memcpy(&tx_buf[11], &gCtrlVars.sample_rate_index, 2);//注意需要确认使用哪个变量Sam mask
		tx_buf[13]  = 0x1;//SDK MCLK为全局
		//memcpy(&tx_buf[17], &FrameSize, 2);//注意需要确认使用哪个变量Sam mask
		tx_buf[5 + 7*2] = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 7*2);
	}
	//else
	//{
		//do nothing
	//}
}

void Communication_Effect_0x02(void)///system ram
{
	uint16_t UsedRamSize = (320 * 1024 - osPortRemainMem())/1024;;
	uint16_t CpuMaxFreq = Clock_CoreClockFreqGet() / 1000000;
	uint16_t cpu_mips = (uint16_t)(((10000 - SysemMipsPercent) * (Clock_CoreClockFreqGet()/1000000)) / 10000);
	uint16_t CpuMaxRamSize = 320;//
	
	memset(tx_buf, 0, sizeof(tx_buf));

	tx_buf[0]  = 0xa5;
	tx_buf[1]  = 0x5a;
	tx_buf[2]  = 0x02;
	tx_buf[3]  = 1 +  1 + 2*4 ;
	tx_buf[4]  = 0xff;

	memcpy(&tx_buf[5], &UsedRamSize, 2);
	memcpy(&tx_buf[7], &cpu_mips, 2);
	//是否自动刷新数据，后续需要改进 Sam mask
	memcpy(&tx_buf[9], &gCtrlVars.AutoRefresh, 1);
	if(gCtrlVars.AutoRefresh)  gCtrlVars.AutoRefresh--;

	memcpy(&tx_buf[10], &CpuMaxFreq, 2);
	memcpy(&tx_buf[12], &CpuMaxRamSize, 2);
	tx_buf[14] = 0x16;
	Communication_Effect_Send(tx_buf, 15);
}

void Comm_PGA0_0x03(uint8_t * buf)
{
	uint16_t TmpData;

	switch(buf[0])///ADC0 PGA
	{
		case 0:///line1 Left en?
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0PGACt.pga0_line1_l_en = !!TmpData;
			AudioLineSelSet();
			break;
		case 1://line1 Right en?
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0PGACt.pga0_line1_r_en = !!TmpData;
			AudioLineSelSet();
			break;
		case 2:///line2 Left en?
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0PGACt.pga0_line2_l_en = !!TmpData;
			AudioLineSelSet();
			break;
		case 3://line2 Right en?
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0PGACt.pga0_line2_r_en = !!TmpData;
			AudioLineSelSet();
			break;
		case 4:///line4 Left en?
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0PGACt.pga0_line4_l_en = !!TmpData;
			AudioLineSelSet();
			break;
		case 5://line4 Right en?
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0PGACt.pga0_line4_r_en = !!TmpData;
			AudioLineSelSet();
		   break;		
		case 6:///line5 Left en?
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0PGACt.pga0_line5_l_en = !!TmpData;
			AudioLineSelSet();
			break;
		case 7://line5 Right en?
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0PGACt.pga0_line5_r_en = !!TmpData;
			AudioLineSelSet();
			break;
		case 8://line1 Left gain
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0PGACt.pga0_line1_l_gain = TmpData > 31? 31 : TmpData;
			AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_LEFT, LINEIN1_LEFT, 31 - gCtrlVars.ADC0PGACt.pga0_line1_l_gain, 4);
			break;
		case 9://line1 Right gain
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0PGACt.pga0_line1_r_gain = TmpData > 31? 31 : TmpData;
			AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_RIGHT, LINEIN1_RIGHT, 31 - gCtrlVars.ADC0PGACt.pga0_line1_r_gain, 4);
			break;
		case 10://line2 Left gain
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0PGACt.pga0_line2_l_gain = TmpData > 31? 31 : TmpData;
			AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_LEFT, LINEIN2_LEFT, 31 - gCtrlVars.ADC0PGACt.pga0_line2_l_gain, 4);
			break;
		case 11://line2 Right gain
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0PGACt.pga0_line2_r_gain = TmpData > 31? 31 : TmpData;
			AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_RIGHT, LINEIN2_RIGHT, 31 - gCtrlVars.ADC0PGACt.pga0_line2_r_gain, 4);
			break;		
		case 12://line4 Left gain
			memcpy(&TmpData, &buf[1], 2);
			TmpData = TmpData > 63? 63 : TmpData;
			if(gCtrlVars.ADC0PGACt.pga0_line5_l_en)
			{
				gCtrlVars.ADC0PGACt.pga0_line5_l_gain = TmpData;
				AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_LEFT, LINEIN5_LEFT, 63 - gCtrlVars.ADC0PGACt.pga0_line5_l_gain, 4);
			}
			if(gCtrlVars.ADC0PGACt.pga0_line4_l_en)
			{
				gCtrlVars.ADC0PGACt.pga0_line4_l_gain = TmpData;
				AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_LEFT, LINEIN4_LEFT, 63 - gCtrlVars.ADC0PGACt.pga0_line4_l_gain, 4);
			}
			break;
		case 13://line4 Right gain
			memcpy(&TmpData, &buf[1], 2);
			TmpData = TmpData > 63? 63 : TmpData;
			if(gCtrlVars.ADC0PGACt.pga0_line5_r_en)
			{
				gCtrlVars.ADC0PGACt.pga0_line5_r_gain = TmpData;
				AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_RIGHT, LINEIN5_RIGHT, 63 - gCtrlVars.ADC0PGACt.pga0_line5_r_gain, 4);
			}
			if(gCtrlVars.ADC0PGACt.pga0_line4_r_en)
			{
				gCtrlVars.ADC0PGACt.pga0_line4_r_gain = TmpData;
				AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_RIGHT, LINEIN4_RIGHT, 63 - gCtrlVars.ADC0PGACt.pga0_line4_r_gain, 4);
			}
			break;	
		case 14:///line5 Left gain
		    #if 0
			memcpy(&TmpData, &buf[1], 2);
			TmpData = TmpData > 63? 63 : TmpData;
			if(gCtrlVars.ADC0PGACt.pga0_line5_l_en)
			{
				gCtrlVars.ADC0PGACt.pga0_line5_l_gain = TmpData;
				AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_LEFT, LINEIN5_LEFT, 63 - gCtrlVars.ADC0PGACt.pga0_line5_l_gain, 4);
			}
			if(gCtrlVars.ADC0PGACt.pga0_line4_l_en)
			{
				gCtrlVars.ADC0PGACt.pga0_line4_l_gain = TmpData;
				AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_LEFT, LINEIN4_LEFT, 63 - gCtrlVars.ADC0PGACt.pga0_line4_l_gain, 4);
			}
			#endif
			break;
		case 15://line5 Right gain
			#if 0
			memcpy(&TmpData, &buf[1], 2);
			TmpData = TmpData > 63? 63 : TmpData;
			if(gCtrlVars.ADC0PGACt.pga0_line5_r_en)
			{
				gCtrlVars.ADC0PGACt.pga0_line5_r_gain = TmpData;
				AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_RIGHT, LINEIN5_RIGHT, 63 - gCtrlVars.ADC0PGACt.pga0_line5_r_gain, 4);
			}
			if(gCtrlVars.ADC0PGACt.pga0_line4_r_en)
			{
				gCtrlVars.ADC0PGACt.pga0_line4_r_gain = TmpData;
				AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_RIGHT, LINEIN4_RIGHT, 63 - gCtrlVars.ADC0PGACt.pga0_line4_r_gain, 4);
			}
			#endif
			break;
	   case 16:///PGA0 different mode set 0=L,R, 1=L1+R1,2=L2+R2 3 = xx
		    memcpy(&TmpData, &buf[1], 2);
		    gCtrlVars.ADC0PGACt.pga0_diff_mode = TmpData > 3 ? 3 : TmpData;

            if(gCtrlVars.ADC0PGACt.pga0_diff_mode == 1)///l diff input
		    {
		    	AudioADC_PGAMode(0, 1);
		    }
		    else if(gCtrlVars.ADC0PGACt.pga0_diff_mode == 2)///r diff input
		    {
		    	AudioADC_PGAMode(1, 0);
		    }
		    else if(gCtrlVars.ADC0PGACt.pga0_diff_mode == 3)///l+r diff input
		    {
		    	AudioADC_PGAMode(0, 0);
		    }	
			else
			{
				AudioADC_PGAMode(1, 1);
			}
		    break;
	   case 17:///PGA0 different left gain  0=0db 1=6db 2=10db 3=15db
		    memcpy(&TmpData, &buf[1], 2);
		    gCtrlVars.ADC0PGACt.pga0_diff_l_gain = TmpData > 3 ? 3 : TmpData;
		    AudioADC_PGADiffGainSel((uint8_t)gCtrlVars.ADC0PGACt.pga0_diff_l_gain, (uint8_t)gCtrlVars.ADC0PGACt.pga0_diff_r_gain);
		    break;
	   case 18:///PGA0 different right gain    0=0db 1=6db 2=10db 3=15db
		    memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0PGACt.pga0_diff_r_gain = TmpData > 3 ? 3 : TmpData;
		    AudioADC_PGADiffGainSel((uint8_t)gCtrlVars.ADC0PGACt.pga0_diff_l_gain, (uint8_t)gCtrlVars.ADC0PGACt.pga0_diff_r_gain);
		    break;
		default:
			break;
	}
}

void Communication_Effect_0x03(uint8_t *buf, uint32_t len)////ADC0 PGA
{
	uint16_t i,k;
	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]   = 0xa5;
		tx_buf[1]   = 0x5a;
		tx_buf[2]   = 0x03;//cmd
		tx_buf[3]   = 1+23*2;///1 + len * sizeof(int16)
		tx_buf[4]   = 0xff;///all paramgs,
		memcpy(&tx_buf[5],  &gCtrlVars.ADC0PGACt.pga0_line1_l_en, 2);
		memcpy(&tx_buf[7],  &gCtrlVars.ADC0PGACt.pga0_line1_r_en, 2);
		memcpy(&tx_buf[9],  &gCtrlVars.ADC0PGACt.pga0_line2_l_en, 2);
		memcpy(&tx_buf[11], &gCtrlVars.ADC0PGACt.pga0_line2_r_en, 2);
		memcpy(&tx_buf[13], &gCtrlVars.ADC0PGACt.pga0_line4_l_en, 2);
		memcpy(&tx_buf[15], &gCtrlVars.ADC0PGACt.pga0_line4_r_en, 2);
		memcpy(&tx_buf[17], &gCtrlVars.ADC0PGACt.pga0_line5_l_en, 2);
		memcpy(&tx_buf[19], &gCtrlVars.ADC0PGACt.pga0_line5_r_en, 2);
		memcpy(&tx_buf[21], &gCtrlVars.ADC0PGACt.pga0_line1_l_gain, 2);
		memcpy(&tx_buf[23], &gCtrlVars.ADC0PGACt.pga0_line1_r_gain, 2);
		memcpy(&tx_buf[25], &gCtrlVars.ADC0PGACt.pga0_line2_l_gain, 2);
		memcpy(&tx_buf[27], &gCtrlVars.ADC0PGACt.pga0_line2_r_gain, 2);

		if(gCtrlVars.ADC0PGACt.pga0_line5_l_en)
		{
			memcpy(&tx_buf[29], &gCtrlVars.ADC0PGACt.pga0_line5_l_gain, 2);
		}
		else
		{
			memcpy(&tx_buf[29], &gCtrlVars.ADC0PGACt.pga0_line4_l_gain, 2);
		}
		if(gCtrlVars.ADC0PGACt.pga0_line5_r_en)
		{
			memcpy(&tx_buf[31], &gCtrlVars.ADC0PGACt.pga0_line5_r_gain, 2);
		}
		else
		{
			memcpy(&tx_buf[31], &gCtrlVars.ADC0PGACt.pga0_line4_r_gain, 2);
		}
		memcpy(&tx_buf[33], &gCtrlVars.ADC0PGACt.pga0_line5_l_gain, 2);
		memcpy(&tx_buf[35], &gCtrlVars.ADC0PGACt.pga0_line5_r_gain, 2);
		memcpy(&tx_buf[37], &gCtrlVars.ADC0PGACt.pga0_diff_mode, 2);
		memcpy(&tx_buf[39], &gCtrlVars.ADC0PGACt.pga0_diff_l_gain, 2);
		memcpy(&tx_buf[41], &gCtrlVars.ADC0PGACt.pga0_diff_r_gain, 2);

		tx_buf[43] = 3;
		tx_buf[45] = 3;
		tx_buf[47] = 3;
		tx_buf[49] = 3;
		tx_buf[5 + 23*2]  = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 23*2);
	}
	else
	{
		switch(buf[0])///ADC0 PGA
		{
			case 0xff:
				buf++;
				for(i = 0; i < 19; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_PGA0_0x03(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_PGA0_0x03(buf);
				break;
		}
	}
}

void Comm_ADC0_0x04(uint8_t * buf)
{
	uint16_t TmpData;
	bool LeftEnable;
	bool RightEnable;
	bool LeftMute;
	bool RightMute;
	
	switch(buf[0])///adc0 digital channel en
	{
		case 0://ADC0 en?
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0DigitalCt.adc0_channel_en = TmpData > 3? 3 : TmpData;
			LeftEnable = gCtrlVars.ADC0DigitalCt.adc0_channel_en & 0x01;
			RightEnable = (gCtrlVars.ADC0DigitalCt.adc0_channel_en >> 1) & 0x01;
			AudioADC_LREnable(ADC0_MODULE, LeftEnable, RightEnable);
			break;
		case 1:///ADC0 mute select? 
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0DigitalCt.adc0_mute = TmpData > 3 ? 3 : TmpData;
			LeftMute = gCtrlVars.ADC0DigitalCt.adc0_mute & 0x01;
			RightMute = (gCtrlVars.ADC0DigitalCt.adc0_mute >> 1) & 0x01;
			AudioADC_DigitalMute(ADC0_MODULE, LeftMute, RightMute);
			break;
		case 2://adc0 dig vol left
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0DigitalCt.adc0_dig_l_vol = TmpData > 0x3fff ? 0x3fff : TmpData;
			AudioADC_VolSetChannel(ADC0_MODULE, CHANNEL_LEFT,(uint16_t)gCtrlVars.ADC0DigitalCt.adc0_dig_l_vol);
			break;
		case 3://adc0 dig vol right
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0DigitalCt.adc0_dig_r_vol = TmpData > 0x3fff? 0x3fff : TmpData;
			AudioADC_VolSetChannel(ADC0_MODULE, CHANNEL_RIGHT, (uint16_t)gCtrlVars.ADC0DigitalCt.adc0_dig_r_vol);
			break;
		case 4://adc0 sample rate
			break;	
		case 5://adc0 LR swap
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC0DigitalCt.adc0_lr_swap = !!TmpData;//不能直接传参数？需要通过if/else来实现
			AudioADC_ChannelSwap(ADC0_MODULE, gCtrlVars.ADC0DigitalCt.adc0_lr_swap);
			break;	
		case 6://adc0 hight pass
			memcpy(&gCtrlVars.ADC0DigitalCt.adc0_dc_blocker, &buf[1], 2);
			gCtrlVars.ADC0DigitalCt.adc0_dc_blocker = TmpData > 2? 2: TmpData;
			AudioADC_HighPassFilterConfig(ADC0_MODULE, HPCList[gCtrlVars.ADC0DigitalCt.adc0_dc_blocker]);
			break;
		case 7://adc0 fade time
			break;
		case 8://adc0 mclk src
		    break;
		case 9://hpc0 en
			break;			
		 default:
		   break;
	}
}

void Communication_Effect_0x04(uint8_t *buf, uint32_t len)////ADC0 DIGITAL
{
	uint16_t i,k;

	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x04;
		tx_buf[3]  = 1+10*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;
		memcpy(&tx_buf[5], &gCtrlVars.ADC0DigitalCt.adc0_channel_en, 2);
		memcpy(&tx_buf[7], &gCtrlVars.ADC0DigitalCt.adc0_mute, 2);
		memcpy(&tx_buf[9], &gCtrlVars.ADC0DigitalCt.adc0_dig_l_vol, 2);
		memcpy(&tx_buf[11], &gCtrlVars.ADC0DigitalCt.adc0_dig_r_vol, 2);
		memcpy(&tx_buf[13], &gCtrlVars.sample_rate_index, 2);//定义一个全局结构体，用于上传PC
		memcpy(&tx_buf[15], &gCtrlVars.ADC0DigitalCt.adc0_lr_swap,2);
		memcpy(&tx_buf[17], &gCtrlVars.ADC0DigitalCt.adc0_dc_blocker, 2);
		tx_buf[19] = 5;
		tx_buf[21] = 0;
		memcpy(&tx_buf[23], &gCtrlVars.ADC0DigitalCt.adc0_dc_blocker_en, 2);////adc0 hpc en
		tx_buf[25] = 0x16;
		Communication_Effect_Send(tx_buf, 26);///25+3*4+1
	}
	else
	{
		switch(buf[0])///ADC0 PGA
		{
			case 0xff:
				buf++;
				for(i = 0; i < 10; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_ADC0_0x04(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_ADC0_0x04(buf);
				break;
		}
	}
}

void Comm_PGA1_0x06(uint8_t * buf)
{
	uint16_t TmpData;

	switch(buf[0])//ADC1 PGA
	{
		case 0:///mic1 en?
		    memcpy(&TmpData, &buf[1], 2);
		    gCtrlVars.ADC1PGACt.line3_l_mic1_en = !!TmpData;
			AudioLine3MicSelect();
			break;
		case 1:////mic2 en ?
		    memcpy(&TmpData, &buf[1], 2);
		    gCtrlVars.ADC1PGACt.line3_r_mic2_en = !!TmpData;
			AudioLine3MicSelect();
			break;
		case 2://mic1 gain
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1PGACt.line3_l_mic1_gain = TmpData > 31 ? 31 : TmpData;
			AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_LEFT, LINEIN3_LEFT_OR_MIC1, 31-gCtrlVars.ADC1PGACt.line3_l_mic1_gain, MIC_BOOST_LIST[gCtrlVars.ADC1PGACt.line3_l_mic1_boost]);
			break;
		case 3://mic1 gain boost
		    memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1PGACt.line3_l_mic1_boost = TmpData > 4 ? 4 : TmpData;
			AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_LEFT, LINEIN3_LEFT_OR_MIC1, 31-gCtrlVars.ADC1PGACt.line3_l_mic1_gain, MIC_BOOST_LIST[gCtrlVars.ADC1PGACt.line3_l_mic1_boost]);
			break;		
		case 4://mic2 gain
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1PGACt.line3_r_mic2_gain = TmpData > 31 ? 31 : TmpData;
			AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2, 31-gCtrlVars.ADC1PGACt.line3_r_mic2_gain, MIC_BOOST_LIST[gCtrlVars.ADC1PGACt.line3_r_mic2_boost]);
			break;
		case 5://mic2 gain boost
		    memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1PGACt.line3_r_mic2_boost = TmpData > 4 ? 4 : TmpData;
			AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2, 31-gCtrlVars.ADC1PGACt.line3_r_mic2_gain, MIC_BOOST_LIST[gCtrlVars.ADC1PGACt.line3_r_mic2_boost]);
			break;
		default:
			break;
	}
}

void Communication_Effect_0x06(uint8_t *buf, uint32_t len)////ADC1 PGA
{
	uint16_t i,k;
	
	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x06;//cmd
		tx_buf[3]  = 1 + 9*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;///all paramgs
		memcpy(&tx_buf[5], &gCtrlVars.ADC1PGACt.line3_l_mic1_en, 2);
		memcpy(&tx_buf[7], &gCtrlVars.ADC1PGACt.line3_r_mic2_en, 2);
		memcpy(&tx_buf[9], &gCtrlVars.ADC1PGACt.line3_l_mic1_gain, 2);
		memcpy(&tx_buf[11], &gCtrlVars.ADC1PGACt.line3_l_mic1_boost, 2);
		memcpy(&tx_buf[13], &gCtrlVars.ADC1PGACt.line3_r_mic2_gain, 2);
		memcpy(&tx_buf[15], &gCtrlVars.ADC1PGACt.line3_r_mic2_boost, 2);
		memcpy(&tx_buf[17], &gCtrlVars.ADC1PGACt.mic_or_line3, 2);
		tx_buf[19] = 1;
		tx_buf[21] = 1;
		tx_buf[5 + 9*2]  = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 9*2);
	}
	else
	{
		switch(buf[0])///adc1 digital set
		{
			case 0xff:
				buf++;
				for(i = 0; i < 9; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_PGA1_0x06(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_PGA1_0x06(buf);
				break;
		}
	}
}

void Comm_ADC1_0x07(uint8_t * buf)
{
	uint16_t TmpData;
	bool LeftEnable;
	bool RightEnable;
	bool LeftMute;
	bool RightMute;
	
	switch(buf[0])///adc1 digital channel en
	{
		case 0://ADC1 en?
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1DigitalCt.adc1_channel_en = TmpData > 3? 3 : TmpData;
			LeftEnable = gCtrlVars.ADC1DigitalCt.adc1_channel_en & 01;
			RightEnable = (gCtrlVars.ADC1DigitalCt.adc1_channel_en >> 1) & 01;
			AudioADC_LREnable(ADC1_MODULE, LeftEnable, RightEnable);
			break;
		case 1:///ADC1 mute select? 
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1DigitalCt.adc1_mute = TmpData > 3? 3 : TmpData;
			LeftMute = gCtrlVars.ADC1DigitalCt.adc1_mute & 0x01;
			RightMute = (gCtrlVars.ADC1DigitalCt.adc1_mute >> 1) & 0x01;
			AudioADC_DigitalMute(ADC1_MODULE, LeftMute, RightMute);
			break;
		case 2://adc1 dig vol left
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1DigitalCt.adc1_dig_l_vol = TmpData > 0x3fff? 0x3fff : TmpData;
			AudioADC_VolSetChannel(ADC1_MODULE, CHANNEL_LEFT,(uint16_t)gCtrlVars.ADC1DigitalCt.adc1_dig_l_vol);
			break;
		case 3://adc1 dig vol right
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1DigitalCt.adc1_dig_r_vol = TmpData > 0x3fff? 0x3fff : TmpData;
			AudioADC_VolSetChannel(ADC1_MODULE, CHANNEL_RIGHT, (uint16_t)gCtrlVars.ADC1DigitalCt.adc1_dig_r_vol);
			break;
		case 4://adc1 sample rate
			break;	
		case 5://adc1 LR swap
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1DigitalCt.adc1_lr_swap = !!TmpData;
			AudioADC_ChannelSwap(ADC1_MODULE, gCtrlVars.ADC1DigitalCt.adc1_lr_swap);
			break;	
		case 6://adc1 hight pass
			memcpy(&gCtrlVars.ADC1DigitalCt.adc1_dc_blocker, &buf[1], 2);
			gCtrlVars.ADC1DigitalCt.adc1_dc_blocker = TmpData > 2? 2: TmpData;
			AudioADC_HighPassFilterConfig(ADC1_MODULE, HPCList[gCtrlVars.ADC1DigitalCt.adc1_dc_blocker]);
			break;
		case 7://adc1 fade time
			break;
		case 8://adc1 mclk src
		    break;
		case 9://hpc1 en
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1DigitalCt.adc1_dc_blocker_en = !!TmpData;
			AudioADC_HighPassFilterSet(ADC1_MODULE, gCtrlVars.ADC1DigitalCt.adc1_dc_blocker_en);
			break;			
		 default:
		   break;
	}
}

void Communication_Effect_0x07(uint8_t *buf, uint32_t len)///ADC1 DIGITAL
{
	uint16_t i,k;

	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x07;
		tx_buf[3]  = 1+10*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;
		memcpy(&tx_buf[5], &gCtrlVars.ADC1DigitalCt.adc1_channel_en, 2);
		memcpy(&tx_buf[7], &gCtrlVars.ADC1DigitalCt.adc1_mute, 2);
		memcpy(&tx_buf[9], &gCtrlVars.ADC1DigitalCt.adc1_dig_l_vol, 2);
		memcpy(&tx_buf[11], &gCtrlVars.ADC1DigitalCt.adc1_dig_r_vol, 2);
		memcpy(&tx_buf[13], &gCtrlVars.sample_rate_index, 2);
		memcpy(&tx_buf[15],  &gCtrlVars.ADC1DigitalCt.adc1_lr_swap,2);
		memcpy(&tx_buf[17], &gCtrlVars.ADC1DigitalCt.adc1_dc_blocker, 2);
		tx_buf[19] = 5;
		tx_buf[21] = 0;
		memcpy(&tx_buf[23], &gCtrlVars.ADC1DigitalCt.adc1_dc_blocker_en, 2);////adc0 hpc en
		tx_buf[25] = 0x16;
		Communication_Effect_Send(tx_buf, 26);///25+3*4+1
	}
	else
	{
		switch(buf[0])///ADC0 PGA
		{
			case 0xff:
				buf++;
				for(i = 0; i < 10; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_ADC1_0x07(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_ADC1_0x07(buf);
				break;
		}
	}
}

void Comm_AGC1_0x08(uint8_t * buf)
{
	uint16_t TmpData;
	bool	AgcLeftEn;
	bool	AgcRightEn;
	switch(buf[0])//ADC1 AGC
	{
		case 0://AGC {buf[1]=0 dis} {buf[1]=1 left en} {buf[1]=2 right en} {buf[1]=3 left+right en}
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1AGCCt.adc1_agc_mode = TmpData > 3? 3 : TmpData;
			AgcLeftEn = gCtrlVars.ADC1AGCCt.adc1_agc_mode & 0x01;
			AgcRightEn = (gCtrlVars.ADC1AGCCt.adc1_agc_mode >> 1) & 0x01;
			AudioADC_AGCChannelSel(ADC1_MODULE, AgcLeftEn, AgcRightEn);
			break;
		case 1://MAX level 
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1AGCCt.adc1_agc_max_level = TmpData > 31? 31 : TmpData;
			AudioADC_AGCMaxLevel(ADC1_MODULE,(uint8_t)gCtrlVars.ADC1AGCCt.adc1_agc_max_level);
			break;
		case 2://target level
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1AGCCt.adc1_agc_target_level = TmpData > 255? 255 : TmpData;
			AudioADC_AGCTargetLevel(ADC1_MODULE,(uint8_t)gCtrlVars.ADC1AGCCt.adc1_agc_target_level);
			break;
		case 3://max gain
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1AGCCt.adc1_agc_max_gain =  TmpData > 31? 31 : TmpData;
			TmpData = 31 - gCtrlVars.ADC1AGCCt.adc1_agc_max_gain;
			AudioADC_AGCMaxGain(ADC1_MODULE,(uint8_t)TmpData);
			break;
		case 4://min gain
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1AGCCt.adc1_agc_min_gain = TmpData > 31? 31 : TmpData;
			TmpData = 31 - gCtrlVars.ADC1AGCCt.adc1_agc_min_gain;
			AudioADC_AGCMinGain(ADC1_MODULE,(uint8_t)TmpData);
			break;
		case 5://gain offset
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1AGCCt.adc1_agc_gainoffset = TmpData > 15? 15 : TmpData;
			AudioADC_AGCGainOffset(ADC1_MODULE,(uint8_t)gCtrlVars.ADC1AGCCt.adc1_agc_gainoffset);
			break;
		case 6://fram time
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1AGCCt.adc1_agc_fram_time = TmpData > 4096? 4096 : TmpData;
			AudioADC_AGCFrameTime(ADC1_MODULE,(uint16_t)gCtrlVars.ADC1AGCCt.adc1_agc_fram_time);
			break;
		case 7://hold time
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1AGCCt.adc1_agc_hold_time = TmpData > 4096? 4096 : TmpData;
			AudioADC_AGCHoldTime(ADC1_MODULE,(uint32_t)gCtrlVars.ADC1AGCCt.adc1_agc_hold_time);
			break;
		case 8://attack time
			memcpy(&TmpData, &buf[1], 2);
			TmpData = TmpData > 4096? 4096 : TmpData;
			gCtrlVars.ADC1AGCCt.adc1_agc_attack_time = TmpData;
			AudioADC_AGCAttackStepTime(ADC1_MODULE,(uint16_t)gCtrlVars.ADC1AGCCt.adc1_agc_attack_time);
			break;
		case 9://dacay time
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1AGCCt.adc1_agc_decay_time = TmpData > 4096? 4096 : TmpData;
			AudioADC_AGCDecayStepTime(ADC1_MODULE,(uint16_t)gCtrlVars.ADC1AGCCt.adc1_agc_decay_time);
			break;
		case 10://nosie gain en
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1AGCCt.adc1_agc_noise_gate_en = TmpData > 1? 1 : TmpData;
			AudioADC_AGCNoiseGateEnable(ADC1_MODULE,(bool)gCtrlVars.ADC1AGCCt.adc1_agc_noise_gate_en);
			break;
		case 11://nosie thershold
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1AGCCt.adc1_agc_noise_threshold = TmpData > 31? 31 : TmpData;
			AudioADC_AGCNoiseThreshold(ADC1_MODULE,(uint8_t)gCtrlVars.ADC1AGCCt.adc1_agc_noise_threshold);
			break;
		case 12://nosie gate mode
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1AGCCt.adc1_agc_noise_gate_mode = TmpData > 1? 1 : TmpData;
			AudioADC_AGCNoiseGateMode(ADC1_MODULE,(uint8_t)gCtrlVars.ADC1AGCCt.adc1_agc_noise_gate_mode);
			break;
		case 13://nosie gate hold time
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.ADC1AGCCt.adc1_agc_noise_time = TmpData > 4096? 4096 : TmpData;
			AudioADC_AGCNoiseHoldTime(ADC1_MODULE,(uint8_t)gCtrlVars.ADC1AGCCt.adc1_agc_noise_time);
			break;
		default:
			break;
	}
}

void Communication_Effect_0x08(uint8_t *buf, uint32_t len)////ADC1 AGC
{
	uint16_t i,k;
	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x08;
		tx_buf[3]  = 1 + 14*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;
        memcpy(&tx_buf[5], &gCtrlVars.ADC1AGCCt.adc1_agc_mode, 2);
		memcpy(&tx_buf[7], &gCtrlVars.ADC1AGCCt.adc1_agc_max_level, 2);
		memcpy(&tx_buf[9], &gCtrlVars.ADC1AGCCt.adc1_agc_target_level, 2);
		memcpy(&tx_buf[11], &gCtrlVars.ADC1AGCCt.adc1_agc_max_gain, 2);
		memcpy(&tx_buf[13], &gCtrlVars.ADC1AGCCt.adc1_agc_min_gain, 2);
		memcpy(&tx_buf[15], &gCtrlVars.ADC1AGCCt.adc1_agc_gainoffset, 2);
		memcpy(&tx_buf[17], &gCtrlVars.ADC1AGCCt.adc1_agc_fram_time, 2);
		memcpy(&tx_buf[19], &gCtrlVars.ADC1AGCCt.adc1_agc_hold_time, 2);
		memcpy(&tx_buf[21], &gCtrlVars.ADC1AGCCt.adc1_agc_attack_time, 2);
		memcpy(&tx_buf[23], &gCtrlVars.ADC1AGCCt.adc1_agc_decay_time, 2);
		memcpy(&tx_buf[25], &gCtrlVars.ADC1AGCCt.adc1_agc_noise_gate_en, 2);
		memcpy(&tx_buf[27], &gCtrlVars.ADC1AGCCt.adc1_agc_noise_threshold, 2);
		memcpy(&tx_buf[29], &gCtrlVars.ADC1AGCCt.adc1_agc_noise_gate_mode, 2);
		memcpy(&tx_buf[31], &gCtrlVars.ADC1AGCCt.adc1_agc_noise_time, 2);
		tx_buf[33] = 0x16;
		Communication_Effect_Send(tx_buf, 34);
	}
	else
	{
		switch(buf[0])///ADC1 AGC
		{
			case 0xff:
				buf++;
				for(i = 0; i < 14; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_AGC1_0x08(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_AGC1_0x08(buf);
				break;
		}
	}
}

void Comm_DAC0_0x09(uint8_t * buf)
{
    uint16_t TmpData;
	bool leftMute;
	bool rightMute;

	switch(buf[0])////DAC0 set
	{
		case 0://DAC0 en
			break;	
        case 1://dac0 sample rate 0~8
			break;	
        case 2:///dac0 mute
        	memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.DAC0Ct.dac0_dig_mute = TmpData > 3? 3 : TmpData;
			leftMute = gCtrlVars.DAC0Ct.dac0_dig_mute & 0x01;
			rightMute = (gCtrlVars.DAC0Ct.dac0_dig_mute >> 1) & 0x01;
			AudioDAC_DigitalMute(DAC0, leftMute, rightMute);
			break;
		case 3:////dac0 L volume
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.DAC0Ct.dac0_dig_l_vol = TmpData > 0x3fff? 0x3fff : TmpData;
			AudioDAC_VolSet(DAC0, gCtrlVars.DAC0Ct.dac0_dig_l_vol, gCtrlVars.DAC0Ct.dac0_dig_r_vol);
			break;
		case 4:////dac0 R volume
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.DAC0Ct.dac0_dig_r_vol = TmpData > 0x3fff? 0x3fff : TmpData;
			AudioDAC_VolSet(DAC0, gCtrlVars.DAC0Ct.dac0_dig_l_vol, gCtrlVars.DAC0Ct.dac0_dig_r_vol);
			break;
        case 5:///DAC0 dither
            memcpy(&TmpData, &buf[1], 2);
            gCtrlVars.DAC0Ct.dac0_dither = !!TmpData;
			if(gCtrlVars.DAC0Ct.dac0_dither) AudioDAC_DitherEnable(DAC0);
			else                      AudioDAC_DitherDisable(DAC0);              
            break;
		case 6:///dac0 scramble
            memcpy(&TmpData, &buf[1], 2);
            gCtrlVars.DAC0Ct.dac0_scramble = TmpData > 4? 4 : TmpData;
			if(gCtrlVars.DAC0Ct.dac0_scramble == 0)
			{
				AudioDAC_ScrambleDisable(DAC0);
			}
			else
			{
				AudioDAC_ScrambleEnable(DAC0);
				AudioDAC_ScrambleModeSet(DAC0,(SCRAMBLE_MODULE)gCtrlVars.DAC0Ct.dac0_scramble);
			}
            break;
        case 7:///dac0 stere mode
            memcpy(&TmpData, &buf[1], 2);
            gCtrlVars.DAC0Ct.dac0_out_mode = TmpData > 3? 3 : TmpData;
#ifndef CFG_AUDIO_WIDTH_24BIT
			AudioDAC_DoutModeSet(DAC0, (DOUT_MODE)gCtrlVars.DAC0Ct.dac0_out_mode, WIDTH_16_BIT);//注意DAC位宽，一般情况下使用不到
#else
            AudioDAC_DoutModeSet(DAC0, (DOUT_MODE)gCtrlVars.DAC0Ct.dac0_out_mode, WIDTH_24_BIT_2);//注意DAC位宽，一般情况下使用不到
#endif
            break; 
		default:
			break;
	}
}

void Communication_Effect_0x09(uint8_t *buf, uint32_t len)///DAC0
{
	uint16_t i,k;
	
	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x09;
		tx_buf[3]  = 1 + 14*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;
 	    tx_buf[5] = 3;		//L+R enable
		memcpy(&tx_buf[7], &gCtrlVars.sample_rate_index, 2);
        memcpy(&tx_buf[9], &gCtrlVars.DAC0Ct.dac0_dig_mute, 2);
		memcpy(&tx_buf[11], &gCtrlVars.DAC0Ct.dac0_dig_l_vol, 2);
		memcpy(&tx_buf[13], &gCtrlVars.DAC0Ct.dac0_dig_r_vol, 2);
		memcpy(&tx_buf[15], &gCtrlVars.DAC0Ct.dac0_dither, 2);
		memcpy(&tx_buf[17], &gCtrlVars.DAC0Ct.dac0_scramble, 2);
		memcpy(&tx_buf[19], &gCtrlVars.DAC0Ct.dac0_out_mode, 2);
        tx_buf[27] = 5;
		tx_buf[33] = 0x16;
		Communication_Effect_Send(tx_buf,34);
	}
	else
	{
		switch(buf[0])///dac0
		{
			case 0xff:
				buf++;
				for(i = 0; i < 14; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_DAC0_0x09(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_DAC0_0x09(buf);
				break;
		}
	}
}

void Comm_DAC1_0x0A(uint8_t * buf)
{
     uint16_t TmpData;
	
	switch(buf[0])////DAC1 set
	{
		case 0://DAC1 en
			break;	
        case 1://dac1 sample rate 0~8
			break;	
        case 2:///dac1 mute
        	memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.DAC1Ct.dac1_dig_mute = !!TmpData;
			AudioDAC_DigitalMute(DAC1, gCtrlVars.DAC1Ct.dac1_dig_mute, gCtrlVars.DAC1Ct.dac1_dig_mute);
			break;					
		case 3:////dac1 L volume
			memcpy(&TmpData, &buf[1], 2);
			gCtrlVars.DAC1Ct.dac1_dig_l_vol = TmpData > 0x3fff? 0x3fff : TmpData;
			AudioDAC_VolSet(DAC1, gCtrlVars.DAC1Ct.dac1_dig_l_vol, gCtrlVars.DAC1Ct.dac1_dig_l_vol);
			break;
		case 4:////dac1 R volume
			break;
        case 5:///DAC1 dither
            memcpy(&TmpData, &buf[1], 2);
            gCtrlVars.DAC1Ct.dac1_dither = !!TmpData;
			if(gCtrlVars.DAC1Ct.dac1_dither) AudioDAC_DitherEnable(DAC1);
			else                      AudioDAC_DitherDisable(DAC1);              
            break;
		case 6:///dac1 scramble
            memcpy(&TmpData, &buf[1], 2);
            gCtrlVars.DAC1Ct.dac1_scramble = TmpData > 4? 4 : TmpData;
			if(gCtrlVars.DAC1Ct.dac1_scramble == 0)
			{
				AudioDAC_ScrambleDisable(DAC1);
			}
			else
			{
				AudioDAC_ScrambleEnable(DAC1);
				AudioDAC_ScrambleModeSet(DAC1,(SCRAMBLE_MODULE)gCtrlVars.DAC1Ct.dac1_scramble);
			}
            break;
        case 7:///dac1 stere mode
//            memcpy(&TmpData, &buf[1], 2);
//			TmpData = TmpData > 3? 3 : TmpData;
//            gCtrlVars.dac1_out_mode = TmpData;
//			AudioDAC_DoutModeSet(DAC1, (DOUT_MODE)gCtrlVars.dac1_out_mode, WIDTH_16_BIT);//
            break;
		default:
			break;
	}
}

void Communication_Effect_0x0A(uint8_t *buf, uint32_t len)//DACX
{
	uint16_t i,k;
	
	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x0a;
		tx_buf[3]  = 1 + 14*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;
		tx_buf[5]  = 1;
		memcpy(&tx_buf[7], &gCtrlVars.sample_rate_index, 2);
        memcpy(&tx_buf[9], &gCtrlVars.DAC1Ct.dac1_dig_mute, 2);
		memcpy(&tx_buf[11], &gCtrlVars.DAC1Ct.dac1_dig_l_vol, 2);
		memcpy(&tx_buf[13], &gCtrlVars.DAC1Ct.dac1_dig_l_vol, 2);
		memcpy(&tx_buf[15], &gCtrlVars.DAC1Ct.dac1_dither, 2);
		memcpy(&tx_buf[17], &gCtrlVars.DAC1Ct.dac1_scramble, 2);
		//memcpy(&tx_buf[19], &gCtrlVars.dac1_out_mode, 2);
        tx_buf[27] = 5;
		tx_buf[33] = 0x16;
		Communication_Effect_Send(tx_buf,34);
	}
	else
	{
		switch(buf[0])///dac0
		{
			case 0xff:
				buf++;
				for(i = 0; i < 14; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_DAC1_0x0A(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_DAC1_0x0A(buf);
				break;
		}
	}
}

void Communication_Effect_0x80(uint8_t *buf, uint32_t len)
{
	uint16_t i;
	uint16_t len_str;
	char *pName = NULL;

	memset(tx_buf, 0, sizeof(tx_buf));
    if(buf[0] == 0)
	{
		tx_buf[0] = 0xa5;
		tx_buf[1] = 0x5a;
		tx_buf[2] = 0x80;
		tx_buf[3] = effect_sum * 2 + 2;
		tx_buf[4]  = 0x00;///index
		tx_buf[5] = effect_sum;
		memcpy(&tx_buf[6], effect_list, effect_sum * 2);
		tx_buf[6 + effect_sum * 2] = 0x16;
		Communication_Effect_Send(tx_buf, 7 + effect_sum * 2);
	}
	else
	{
		if( (buf[0] <= effect_sum ) && (buf[0] > 0 ) )///////(A5 5A 80 01 00 16=EFF_0)(A5 5A 80 01 01 16=EFF_1)
		{
			pName = AudioEffectParamCtAddr[buf[0]-1].EffectName;

			len_str = strlen(pName);
			if(len > EFFECT_LIST_NAME_MAX)
			{
				len = EFFECT_LIST_NAME_MAX;
			}

			tx_buf[0]  = 0xa5;
			tx_buf[1]  = 0x5a;
			tx_buf[2]  = 0x80;
			tx_buf[3]  = len_str + 1;//25 * 1 + 1;//len
			tx_buf[4]  = buf[0];///audio effect number
			tx_buf[len_str + 1 + 4]  = 0x16;
			for(i = 0; i < len_str; i++)
			{
				tx_buf[i + 5] = *pName;//上传音效名称，最大25个字符
				pName++;
			}
			Communication_Effect_Send(tx_buf, len_str + 1 + 4 + 1);
		}
	}
}

//32个字节
void Communication_Effect_0xfc(uint8_t *buf, uint8_t len)//user define tag
{
	memset(tx_buf, 0, sizeof(tx_buf));
	tx_buf[0]  = 0xa5;
	tx_buf[1]  = 0x5a;
	tx_buf[2]  = 0xfc;//control
	if(len == 0)
	{
		tx_buf[3]  = lenTag;//1+ 4;//len=data_size+index
		//tx_buf[4]  = 0xff;///index
		memcpy(&tx_buf[4], TagBuf, lenTag);
		tx_buf[4+lenTag] = 0x16;
		Communication_Effect_Send(tx_buf, 4+lenTag+1);
	}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
	else if(len == 1)
	{
		tx_buf[3]  = 1;
		tx_buf[4]  = buf[0];
		tx_buf[5]  = 0x16;
		Communication_Effect_Send(&tx_buf[2], 1);//返回控制字
		//EffectParamFlashIndex = tx_buf[4] - '0';
		//if(EffectParamFlashIndex >= 10)
		//{
		//	EffectParamFlashIndex = 0xFF;
		//}
	}
#endif
}

void Communication_Effect_0xfd(uint8_t *buf, uint8_t len)//user define tag
{
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
	memset(tx_buf, 0, sizeof(tx_buf));
	tx_buf[0]  = 0xa5;
	tx_buf[1]  = 0x5a;
	tx_buf[2]  = 0xfd;//control
	tx_buf[3]  = 0x16;
	Communication_Effect_Send(&tx_buf[2], 1);//返回控制字
	EffectParamFlashUpdataFlag = 1;
#endif
}

void Communication_Effect_0xff(uint8_t *buf, uint32_t len)
{
    uint32_t TmpData;
	memset(tx_buf, 0, sizeof(tx_buf));
	tx_buf[0] = 0xa5;
	tx_buf[1] = 0x5a;
	tx_buf[2] = 0xFF;
	tx_buf[3] = 3;
	if(len == 0)
	{
		tx_buf[4] = 0;//index0
		tx_buf[5] = CFG_COMMUNICATION_CRYPTO;
	}
	else
	{
		memcpy(&TmpData, &buf[0], 4);
		tx_buf[4] = 1;//index1	
		if(CFG_COMMUNICATION_PASSWORD != TmpData)
		{	   
			tx_buf[5] = 0;///passwrod err
		}
		else
		{
		    tx_buf[5] = 1;///passwrod ok		
		}
	}
	tx_buf[7] = 0x16;
	Communication_Effect_Send(tx_buf, 8);
}

/////////////////////////////Effect////////////////////////////
void Comm_Effect_Send(void *pEffect, uint8_t Enable, void* param, uint16_t Len, uint8_t Control)
{
	memset(tx_buf, 0, sizeof(tx_buf));
	tx_buf[0] = 0xa5;
	tx_buf[1] = 0x5a;
	tx_buf[2] = Control;
	tx_buf[3] = Len + 2 + 1;
	tx_buf[4] = 0xff;
	//if(pEffect != NULL)//资源没有申请，直接上传0//sam,20210104,不能直接上传0，否则导出C文件时参数会是0
	{
		tx_buf[5] = Enable;
		memcpy(&tx_buf[7], param, Len);
	}

	tx_buf[5+2+Len] = 0x16;
	Communication_Effect_Send(tx_buf, 6 + 2 + Len);
}

#if CFG_AUDIO_EFFECT_AUTO_TUNE_EN
void Comm_Effect_AutoTune(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
    uint16_t TmpData16;
    uint16_t i;
	AutoTuneUnit *p = (AutoTuneUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			for(i = 0; i < 13; i++)
			{
				if(buf[1] == AutoTuneKeyTbl[i])
				{
					if(TmpData16 != p->param.key)
					{
						p->param.key = TmpData16;
						AudioEffectAutoTuneInit(p, p->channel, gCtrlVars.sample_rate);
					}
					break;
				}
			}
			break;
		case 2:
			memcpy(&TmpData16, &buf[1], 2);
			for(i = 0; i < 3; i++)
			{
				if(buf[1] == AutoTuneSnapTbl[i])
				{
					p->param.snap_mode = TmpData16;
					break;
				}
			}
			break;
		case 3:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16>2)
			{
				TmpData16 = 2;
			}
			p->param.pitch_accuracy = TmpData16;
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(AutoTuneParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(AutoTuneParam));
			AudioEffectAutoTuneInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		 default:
		 	break;
	}
}

void Communication_Effect_AutoTune(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	AutoTuneUnit *p = (AutoTuneUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(AutoTuneParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_AutoTune(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(AutoTuneParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_DRC_EN
void Comm_Effect_DRC(EffectNode * pNode, uint8_t * buf, uint8_t index)
{
    uint16_t TmpData16,TmpData16_1;
	int16_t TmpDataS16;
	int16_t TmpData1S16;
	int16_t TmpData2S16;
	int16_t TmpData3S16;
	DRCUnit *p = (DRCUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
//					if(p->enable)
//					{
//						AudioEffectDRCInit(p, p->channel, gCtrlVars.sample_rate);
//					}
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 4)
			{
				TmpData16 = 4;
			}

			if(p->param.mode != TmpData16)
			{
				p->param.mode = TmpData16;
				AudioEffectDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 2:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 ==0)
			{
				TmpData16 = 1;
			}
			if(TmpData16 > 4)
			{
				TmpData16 = 4;
			}

			if(p->param.cf_type != TmpData16)
			{
				p->param.cf_type = TmpData16;
				AudioEffectDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 3:
			memcpy(&TmpDataS16, &buf[1], 2);

			if(TmpDataS16 != p->param.q_l)
			{
				p->param.q_l = TmpDataS16;
				AudioEffectDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 4:
			memcpy(&TmpDataS16, &buf[1], 2);

			if(TmpDataS16 != p->param.q_h)
			{
				p->param.q_h = TmpDataS16;
				AudioEffectDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 5:
			memcpy(&TmpData16, &buf[1], 2);
			memcpy(&TmpData16_1, &buf[3], 2);
            if(TmpData16 > 20000)
			{
				TmpData16 = 20000;
			}
            if(TmpData16 < 20)
			{
				TmpData16 = 20;
			}
            if(TmpData16_1 > 20000)
			{
            	TmpData16_1 = 20000;
			}
            if(TmpData16_1 < 20)
			{
            	TmpData16_1 = 20;
			}
			if((TmpData16 != p->param.fc[0]) || (TmpData16_1 != p->param.fc[1]))
			{
				p->param.fc[0] = TmpData16;
				p->param.fc[1] = TmpData16_1;
				AudioEffectDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 6:
			memcpy(&TmpDataS16, &buf[1], 2);
			memcpy(&TmpData1S16, &buf[3], 2);
			memcpy(&TmpData2S16, &buf[5], 2);
			memcpy(&TmpData3S16, &buf[7], 2);

			TmpDataS16 = TmpDataS16 < -9000? -9000 : TmpDataS16;
			TmpDataS16 = TmpDataS16 > 0? 0 : TmpDataS16;
			TmpData1S16 = TmpData1S16 < -9000? -9000 : TmpData1S16;
			TmpData1S16 = TmpData1S16 > 0? 0 : TmpData1S16;
			TmpData2S16 = TmpData2S16 < -9000? -9000 : TmpData2S16;
			TmpData2S16 = TmpData2S16 > 0? 0 : TmpData2S16;
			TmpData3S16 = TmpData3S16 < -9000? -9000 : TmpData3S16;
			TmpData3S16 = TmpData3S16 > 0? 0 : TmpData3S16;

			if((TmpDataS16 != p->param.threshold[0]) || (TmpData1S16 != p->param.threshold[1]) || (TmpData2S16 != p->param.threshold[2])|| (TmpData3S16 != p->param.threshold[3]))
			{
				p->param.threshold[0] = TmpDataS16;
				p->param.threshold[1] = TmpData1S16;
				p->param.threshold[2] = TmpData2S16;
				p->param.threshold[3] = TmpData3S16;
				AudioEffectDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 7:
			memcpy(&TmpDataS16, &buf[1], 2);
			memcpy(&TmpData1S16, &buf[3], 2);
			memcpy(&TmpData2S16, &buf[5], 2);
			memcpy(&TmpData3S16, &buf[7], 2);

			TmpDataS16  = TmpDataS16 < 1? 	   1 : TmpDataS16;
			TmpDataS16  = TmpDataS16 > 1000?	1000 : TmpDataS16;
			TmpData1S16 = TmpData1S16 < 1?	   1 : TmpData1S16;
			TmpData1S16 = TmpData1S16 > 1000? 1000 : TmpData1S16;
			TmpData2S16 = TmpData2S16 < 1?	   1 : TmpData2S16;
			TmpData2S16 = TmpData2S16 > 1000? 1000 : TmpData2S16;
			TmpData3S16 = TmpData3S16 < 1?	   1 : TmpData3S16;
			TmpData3S16 = TmpData3S16 > 1000? 1000 : TmpData3S16;

			if((TmpDataS16 != p->param.ratio[0]) || (TmpData1S16 != p->param.ratio[1]) || (TmpData2S16 != p->param.ratio[2])|| (TmpData3S16 != p->param.ratio[3]))
			{
				p->param.ratio[0] = TmpDataS16;
				p->param.ratio[1] = TmpData1S16;
				p->param.ratio[2] = TmpData2S16;
				p->param.ratio[3] = TmpData3S16;
				AudioEffectDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 8:
			memcpy(&TmpDataS16, &buf[1], 2);
			memcpy(&TmpData1S16, &buf[3], 2);
			memcpy(&TmpData2S16, &buf[5], 2);
			memcpy(&TmpData3S16, &buf[7], 2);

			TmpDataS16  = TmpDataS16 < 0? 	   0 : TmpDataS16;
			TmpDataS16  = TmpDataS16 > 7500?	7500 : TmpDataS16;
			TmpData1S16 = TmpData1S16 < 0?	   0 : TmpData1S16;
			TmpData1S16 = TmpData1S16 > 7500? 7500 : TmpData1S16;
			TmpData2S16 = TmpData2S16 < 0?	   0 : TmpData2S16;
			TmpData2S16 = TmpData2S16 > 7500? 7500 : TmpData2S16;
			TmpData3S16 = TmpData3S16 < 0?	   0 : TmpData3S16;
			TmpData3S16 = TmpData3S16 > 7500? 7500 : TmpData3S16;

			if((TmpDataS16 != p->param.attack_tc[0]) || (TmpData1S16 != p->param.attack_tc[1]) || (TmpData2S16 != p->param.attack_tc[2])|| (TmpData3S16 != p->param.attack_tc[3]))
			{
				p->param.attack_tc[0] = TmpDataS16;
				p->param.attack_tc[1] = TmpData1S16;
				p->param.attack_tc[2] = TmpData2S16;
				p->param.attack_tc[3] = TmpData3S16;
				AudioEffectDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 9:
			memcpy(&TmpDataS16, &buf[1], 2);
			memcpy(&TmpData1S16, &buf[3], 2);
			memcpy(&TmpData2S16, &buf[5], 2);
			memcpy(&TmpData3S16, &buf[7], 2);

			TmpDataS16  = TmpDataS16 < 0? 	   0 : TmpDataS16;
			TmpDataS16  = TmpDataS16 > 7500?	7500 : TmpDataS16;
			TmpData1S16 = TmpData1S16 < 0?	   0 : TmpData1S16;
			TmpData1S16 = TmpData1S16 > 7500? 7500 : TmpData1S16;
			TmpData2S16 = TmpData2S16 < 0?	   0 : TmpData2S16;
			TmpData2S16 = TmpData2S16 > 7500? 7500 : TmpData2S16;
			TmpData3S16 = TmpData3S16 < 0?	   0 : TmpData3S16;
			TmpData3S16 = TmpData3S16 > 7500? 7500 : TmpData3S16;

			if((TmpDataS16 != p->param.release_tc[0]) || (TmpData1S16 != p->param.release_tc[1]) || (TmpData2S16 != p->param.release_tc[2])|| (TmpData3S16 != p->param.release_tc[3]))
			{
				p->param.release_tc[0] = TmpDataS16;
				p->param.release_tc[1] = TmpData1S16;
				p->param.release_tc[2] = TmpData2S16;
				p->param.release_tc[3] = TmpData3S16;
				AudioEffectDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 10:
			memcpy(&TmpDataS16, &buf[1], 2);
			memcpy(&TmpData1S16, &buf[3], 2);
			memcpy(&TmpData2S16, &buf[5], 2);
			memcpy(&TmpData3S16, &buf[7], 2);

			TmpDataS16  = TmpDataS16 > 32536?	32536 : TmpDataS16;
			TmpData1S16 = TmpData1S16 > 32536?  32536 : TmpData1S16;
			TmpData2S16 = TmpData2S16 > 32536?  32536 : TmpData2S16;
			TmpData3S16 = TmpData3S16 > 32536?  32536 : TmpData3S16;

			p->param.pregain[0] = TmpDataS16;
			p->param.pregain[1] = TmpData1S16;
			p->param.pregain[2] = TmpData2S16;
			p->param.pregain[3] = TmpData3S16;
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(DRCParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(DRCParam));
			AudioEffectDRCInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			 break;
	}
}

void Communication_Effect_DRC(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	DRCUnit *p = (DRCUnit *)pNode->EffectUnit;
	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(DRCParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_DRC(pNode,buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(DRCParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_ECHO_EN
void Comm_Effect_Echo(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
    uint16_t TmpData16;
	int16_t TmpDataS16;
	EchoUnit *p = (EchoUnit *)pNode->EffectUnit;;
	EchoParam* pParam = NULL;

	if(p == NULL)
	{
		pParam = effect_list_param[index];
	}
	else
	{
		pParam = &p->param;
	}
	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 > 24000)
			{
				TmpDataS16 = 24000;
			}
			else if(TmpDataS16 < 0)
			{
				TmpDataS16 = 0;
			}
			if(pParam->fc != TmpDataS16)
			{
				pParam->fc = TmpDataS16;
				AudioEffectEchoInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 2:
			memcpy(&TmpData16, &buf[1], 2);
			TmpData16 = TmpData16 == 0? 1 : TmpData16;
			if(pParam->attenuation != TmpData16)
			{
				pParam->attenuation = TmpData16;
			}
			break;
		case 3:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > pParam->max_delay)
			{
				TmpData16 = pParam->max_delay;
			}
			if(pParam->delay != TmpData16)
			{
				pParam->delay = TmpData16;
			}
			break;
		case 4://reserved
			break;
		case 5://注意申请的buf size
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 3000)
			{
				TmpData16 = 3000;
			}

			if(pParam->max_delay != TmpData16)
			{
				pParam->max_delay = TmpData16;
				if(pParam->high_quality_enable)
				{
					if(pParam->max_delay > 1000)
					{
						pParam->max_delay = 1000;
					}
				}
				else
				{
					if(pParam->max_delay > 3000)
					{
						pParam->max_delay = 3000;
					}
				}
				pParam->delay = pParam->delay > pParam->max_delay? pParam->max_delay : pParam->delay;
				//IsEffectChange = 1;
			}
			break;
		case 6://注意申请的buf size
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 1)
			{
				TmpData16 = 1;
			}
			if(pParam->high_quality_enable != TmpData16)
			{
				pParam->high_quality_enable = TmpData16;
				if(pParam->high_quality_enable)
				{
					if(pParam->max_delay > 1000)
					{
						pParam->max_delay = 1000;
					}
				}
				else
				{
					if(pParam->max_delay > 3000)
					{
						pParam->max_delay = 3000;
					}
				}
				pParam->delay = pParam->delay > pParam->max_delay? pParam->max_delay : pParam->delay;
				//IsEffectChange = 1;
			}
			break;
		case 7:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 100;
			}
			if(pParam->dry != TmpData16)
			{
				pParam->dry = TmpData16;
			}
			break;
		case 8:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 100;
			}
			if(pParam->wet != TmpData16)
			{
				pParam->wet = TmpData16;
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(EchoParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(pParam, &buf[3], sizeof(EchoParam));
			//AudioEffectEchoInit(p, p->channel, gCtrlVars.sample_rate);
			IsEffectChange = 1;
			break;
		 default:
		 	break;
	}
}

void Communication_Effect_Echo(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	EchoUnit *p = (EchoUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(EchoParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_Echo(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(EchoParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_EQ_EN
void Comm_Effect_EQ(EffectNode * pNode, uint8_t * buf, uint8_t index)
{
	EQUnit *p = (EQUnit *)pNode->EffectUnit;
	int16_t TmpData, TmpDataS16;
	uint8_t filter_num;
	switch(buf[0])
	{
		case 0:
			memcpy(&TmpData, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData)
				{
					p->enable = TmpData;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(p->param.pregain != TmpDataS16)
			{
				p->param.pregain = TmpDataS16;
				AudioEffectEQPregainConfig(p);
			}
			break;
		case 2:
			memcpy(&TmpData, &buf[1], 2);
			TmpData &= 1;

			if(p->param.calculation_type != TmpData)
			{
				p->param.calculation_type = TmpData;
				AudioEffectEQInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 0xff:
			memcpy(&TmpData, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(EQParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData)
			{
				p->enable = TmpData;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(EQParam));
			AudioEffectEQInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			if(buf[0] < 53)
			{
				filter_num = (buf[0]- 3) / 5;
				memcpy(&TmpDataS16, &buf[1], 2);
				if(((buf[0]- 3) % 5) == 0)
				{
					p->param.eq_params[filter_num].enable = TmpDataS16;
				}
				else if(((buf[0]- 3) % 5) == 1)
				{
					p->param.eq_params[filter_num].type = TmpDataS16;
				}
				else if(((buf[0]- 3) % 5) == 2)
				{
					p->param.eq_params[filter_num].f0 = TmpDataS16;
				}
				else if(((buf[0]- 3) % 5) == 3)
				{
					p->param.eq_params[filter_num].Q = TmpDataS16;
				}
				else if(((buf[0]- 3) % 5) == 4)
				{
					p->param.eq_params[filter_num].gain = TmpDataS16;
				}
				AudioEffectEQInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
	}
}

void Communication_Effect_EQ(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	EQUnit *p = (EQUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(EQParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else//set
	{
		Comm_Effect_EQ(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(EQParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_NOISE_SUPPRESSOR_EN
void Comm_Effect_Expander(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
    uint16_t TmpData16;
	int16_t TmpDataS16;
	ExpanderUnit *p = (ExpanderUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 < -9000)
			{
				TmpDataS16 = -9000;
			}
			else if(TmpDataS16 > 0)
			{
				TmpDataS16 = 0;
			}
			if(p->param.threshold != TmpDataS16)
			{
				p->param.threshold = TmpDataS16;
				AudioEffectExpanderThresholdConfig(p);
			}
			break;
		case 2:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 > 1000)
			{
				TmpDataS16 = 1000;
			}
			else if(TmpDataS16 < 1)
			{
				TmpDataS16 = 1;
			}
			if(p->param.ratio != TmpDataS16)
			{
				p->param.ratio = TmpDataS16;
				AudioEffectExpanderInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 3:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 > 7500)
			{
				TmpDataS16 = 7500;
			}
			else if(TmpDataS16 < 0)
			{
				TmpData16 = 0;
			}
			if(p->param.attack_time != TmpDataS16)
			{
				p->param.attack_time = TmpDataS16;
				AudioEffectExpanderInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 4:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 > 7500)
			{
				TmpDataS16 = 7500;
			}
			else if(TmpDataS16 < 0)
			{
				TmpDataS16 = 0;
			}
			if(p->param.release_time != TmpDataS16)
			{
				p->param.release_time = TmpDataS16;
				AudioEffectExpanderInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(ExpanderParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(ExpanderParam));
			AudioEffectExpanderInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		 default:
		 	break;
	}
}

void Communication_Effect_Expander(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	ExpanderUnit *p = (ExpanderUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(ExpanderParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_Expander(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(ExpanderParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_FREQ_SHIFTER_EN
void Comm_Effect_FreqShifter(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
    uint16_t TmpData16;
	FreqShifterUnit *p = (FreqShifterUnit *)pNode->EffectUnit;;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			TmpData16 = TmpData16 > 8? 8:TmpData16;
			if(TmpData16 != p->param.deltaf)
			{
				p->param.deltaf = TmpData16 > 8 ? 8 : TmpData16;
				AudioEffectFreqShifterInit(p);
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(FreqShifterParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(FreqShifterParam));
			AudioEffectFreqShifterInit(p);
			break;
		 default:
		 	break;
	}
}

void Communication_Effect_FreqShifter(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	FreqShifterUnit *p = (FreqShifterUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(FreqShifterParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_FreqShifter(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(FreqShifterParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_HOWLING_SUPPRESSOR_EN
void Comm_Effect_HowlingSuppressor(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
    uint16_t TmpData16;
    HowlingUnit *p = (HowlingUnit *)pNode->EffectUnit;;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			TmpData16 = TmpData16 > 2? 2:TmpData16;
			if(p->param.suppression_mode != TmpData16)
			{
				p->param.suppression_mode = TmpData16;
				AudioEffectHowlingSuppressorInit(p);
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(HowlingParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(HowlingParam));
			AudioEffectHowlingSuppressorInit(p);
			break;
		 default:
		 	break;
	}
}

void Communication_Effect_HowlingSuppressor(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	HowlingUnit *p = (HowlingUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(HowlingParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_HowlingSuppressor(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(HowlingParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_PITCH_SHIFTER_EN
void Comm_Effect_PitchShifter(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
    uint16_t TmpData16;
    int16_t TmpDataS16;
    PitchShifterUnit *p = (PitchShifterUnit *)pNode->EffectUnit;;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpDataS16, &buf[1], 2);
			TmpDataS16 = TmpDataS16 > 120? 120:TmpDataS16;
			TmpDataS16 = TmpDataS16 < -120? -120:TmpDataS16;
			if(p->param.semitone_steps != TmpDataS16)
			{
				p->param.semitone_steps = TmpDataS16;
				AudioEffectPitchShifterConfig(p);
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(PitchShifterParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(PitchShifterParam));
			p->param.semitone_steps = p->param.semitone_steps > 120? 120:p->param.semitone_steps;
			p->param.semitone_steps = p->param.semitone_steps < -120? -120:p->param.semitone_steps;
			AudioEffectPitchShifterInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		 default:
		 	break;
	}
}

void Communication_Effect_PitchShifter(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	PitchShifterUnit *p = (PitchShifterUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(PitchShifterParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_PitchShifter(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(PitchShifterParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_REVERB_EN
void Comm_Effect_Reverb(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
    int16_t TmpData16;
    ReverbUnit *p = (ReverbUnit *)pNode->EffectUnit;;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 200)
			{
				TmpData16 = 200;
			}
			if(p->param.dry_scale != TmpData16 )
			{
				p->param.dry_scale = TmpData16;
				AudioEffectReverbConfig(p);
			}
			break;
		case 2:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 300)
			{
				TmpData16 = 300;
			}
			if(p->param.wet_scale != TmpData16)
			{
				p->param.wet_scale = TmpData16;
				AudioEffectReverbConfig(p);
			}
			break;

		case 3:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 100;
			}
			if(p->param.width_scale != TmpData16)
			{
				p->param.width_scale = TmpData16;
				AudioEffectReverbConfig(p);
			}
			break;

		case 4:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 100;
			}
			if(p->param.roomsize_scale != TmpData16)
			{
				p->param.roomsize_scale = TmpData16;
				AudioEffectReverbConfig(p);
			}
			break;

		case 5:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 100;
			}
			if( p->param.damping_scale != TmpData16)
			{
				p->param.damping_scale = TmpData16;
				AudioEffectReverbConfig(p);
			}
			break;

		case 6:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 1)
			{
				TmpData16 = 1;
			}
			if(p->param.mono != TmpData16)
			{
				p->param.mono = TmpData16;
				AudioEffectReverbConfig(p);
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(ReverbParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(ReverbParam));
			AudioEffectReverbInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		 default:
		 	break;
	}
}

void Communication_Effect_Reverb(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	ReverbUnit *p = (ReverbUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(ReverbParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_Reverb(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(ReverbParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_3D_EN
void Comm_Effect_ThreeD(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
	int16_t TmpData16;
	ThreeDUnit *p = (ThreeDUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 100;
			}
			if(p->param.intensity != TmpData16)
			{
				p->param.intensity = TmpData16;
				AudioEffectThreeDInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(ThreeDParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(ThreeDParam));
			AudioEffectThreeDInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			break;
	}
}
void Communication_Effect_ThreeD(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	ThreeDUnit *p = (ThreeDUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(ThreeDParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_ThreeD(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(ThreeDParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_VB_EN
void Comm_Effect_VB(EffectNode * pNode, uint8_t * buf, uint8_t index)
{
	int16_t TmpData16;
	VBUnit *p = (VBUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 300)
			{
				TmpData16 = 300;
			}
			else if(TmpData16 < 30)
			{
				TmpData16 = 30;
			}
			if(p->param.f_cut != TmpData16)
			{
				p->param.f_cut = TmpData16;
				AudioEffectVBInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 2:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 100;
			}
			if(p->param.intensity != TmpData16)
			{
				p->param.intensity = TmpData16;
				AudioEffectVBInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 3:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 1)
			{
				TmpData16 = 1;
			}
			if( p->param.enhanced != TmpData16)
			{
				p->param.enhanced = TmpData16;
				AudioEffectVBInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(VBParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(VBParam));
			AudioEffectVBInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			break;
	}
}

void Communication_Effect_VB(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	VBUnit *p = (VBUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(VBParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_VB(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(VBParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_VOICE_CHANGER_EN
void Comm_Effect_VoiceChanger(EffectNode * pNode, uint8_t * buf, uint8_t index)
{
	int16_t TmpData16;
	VoiceChangerUnit *p = (VoiceChangerUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			TmpData16 = TmpData16 > 300?300:TmpData16;
			TmpData16 = TmpData16 < 50?50:TmpData16;
			if(p->param.pitch_ratio != TmpData16)
			{
				p->param.pitch_ratio = TmpData16;
				AudioEffectVoiceChangerInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 2:
			memcpy(&TmpData16, &buf[1], 2);
			TmpData16 = TmpData16 > 200?200:TmpData16;
			TmpData16 = TmpData16 < 66? 66:TmpData16;
			if(p->param.formant_ratio != TmpData16)
			{
				p->param.formant_ratio = TmpData16;
				AudioEffectVoiceChangerInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(VoiceChangerParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(VoiceChangerParam));
			p->param.formant_ratio = p->param.formant_ratio < 66 ? 66: p->param.formant_ratio;
			p->param.formant_ratio = p->param.formant_ratio > 200 ? 200: p->param.formant_ratio;
			p->param.pitch_ratio = p->param.pitch_ratio < 50 ? 50: p->param.pitch_ratio;
			p->param.pitch_ratio = p->param.pitch_ratio > 300 ? 300: p->param.pitch_ratio;
			AudioEffectVoiceChangerInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			break;
	}
}

void Communication_Effect_VoiceChanger(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	VoiceChangerUnit *p = (VoiceChangerUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(VoiceChangerParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_VoiceChanger(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(VoiceChangerParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_GAIN_CONTROL_EN
void Comm_Effect_GainControl(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
	uint16_t TmpData16;
	GainControlUnit *p = (GainControlUnit *)pNode->EffectUnit;
	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
				IsEffectChange = 1;
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&p->param.mute, &buf[1], 2);
			break;
		case 2:
			memcpy(&TmpData16, &buf[1], 2);
			TmpData16 = TmpData16 > 0x3fff? 0x3fff : TmpData16;
			p->param.gain = TmpData16;
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(GainControlParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
			}
			memcpy(&p->param, &buf[3], sizeof(GainControlParam));
			break;
		default:
			break;
	}
}

void Communication_Effect_GainControl(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	GainControlUnit *p = (GainControlUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(GainControlParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_GainControl(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(GainControlParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_VOCAL_CUT_EN
void Comm_Effect_VocalCut(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
	int16_t TmpData16;
	VocalCutUnit *p = (VocalCutUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 100;
			}
			p->param.wetdrymix = TmpData16;
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(VocalCutParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(VocalCutParam));
			AudioEffectVocalCutInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			break;
	}
}
void Communication_Effect_VocalCut(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	VocalCutUnit *p = (VocalCutUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(VocalCutParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_VocalCut(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(VocalCutParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_PLATE_REVERB_EN
void Comm_Effect_PlateReverb(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
	uint16_t TmpData16;
	int16_t TmpDataS16;
	PlateReverbUnit *p = (PlateReverbUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > gCtrlVars.sample_rate/2)
			{
				TmpData16 = gCtrlVars.sample_rate/2;
			}
			if(p->param.highcut_freq != TmpData16)
			{
				p->param.highcut_freq = TmpData16;
				AudioEffectPlateReverbInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 2:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 > 1)
			{
				TmpDataS16 = 1;
			}
			if(TmpDataS16 < 0)
			{
				TmpData16 = 0;
			}
			if(p->param.modulation_en != TmpDataS16)
			{
				p->param.modulation_en = TmpDataS16;
				AudioEffectPlateReverbInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 3:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 4410)
			{
				TmpData16 = 4410;
			}
			if(p->param.predelay != TmpData16)
			{
				p->param.predelay = TmpData16;
				AudioEffectPlateReverbConfig(p);
			}
			break;
		case 4:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 100;
			}
			if(p->param.diffusion != TmpData16)
			{
				p->param.diffusion = TmpData16;
				AudioEffectPlateReverbConfig(p);
			}
			break;

		case 5:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 100;
			}
			if(p->param.decay != TmpData16)
			{
				p->param.decay = TmpData16;
				AudioEffectPlateReverbConfig(p);
			}
			break;

		case 6:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 10000)
			{
				TmpData16 = 10000;
			}
			if(p->param.damping != TmpData16)
			{
				p->param.damping = TmpData16;
				AudioEffectPlateReverbConfig(p);
			}
			break;

		case 7:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 100;
			}
			if(p->param.wetdrymix != TmpData16)
			{
				p->param.wetdrymix = TmpData16;
				AudioEffectPlateReverbConfig(p);
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(PlateReverbParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(PlateReverbParam));
			AudioEffectPlateReverbInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			break;
	}
}
void Communication_Effect_PlateReverb(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	PlateReverbUnit *p = (PlateReverbUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(PlateReverbParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_PlateReverb(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(PlateReverbParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_REVERB_PRO_EN
void Comm_Effect_ReverbPro(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
	uint16_t TmpData16;
	int16_t TmpDataS16;
	ReverbProUnit *p = (ReverbProUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1://dry -70 to +10db
			memcpy(&TmpDataS16, &buf[1], 2);
			p->param.dry = TmpDataS16;
			break;
		case 2://wet -70 to +10dbmemcpy
			memcpy(&TmpDataS16, &buf[1], 2);
			p->param.wet = TmpDataS16;
			break;
		case 3://erwet -70 to +10dbmemcpy
			memcpy(&TmpDataS16, &buf[1], 2);
			p->param.erwet = TmpDataS16;
			break;
		case 4://erfactor 50 to 250%memcpy
			memcpy(&TmpDataS16, &buf[1], 2);
			p->param.erfactor = TmpDataS16;
			break;
		case 5://erwitdh -100 to 100%memcpy
			memcpy(&TmpDataS16, &buf[1], 2);
			p->param.erwidth = TmpDataS16;
			break;
		case 6://ertolate 0 to 100%memcpy
			memcpy(&TmpDataS16, &buf[1], 2);
			p->param.ertolate = TmpDataS16;
			break;
		case 7://rt60 100 to 15000msmemcpy
			memcpy(&TmpDataS16, &buf[1], 2);
			p->param.rt60 = TmpDataS16;
			break;
		case 8://delay 0 to 100msmemcpy
			memcpy(&TmpDataS16, &buf[1], 2);
			p->param.delay = TmpDataS16;
			break;
		case 9://width 0 to 100%memcpy
			memcpy(&TmpDataS16, &buf[1], 2);
			p->param.width = TmpDataS16;
			break;
		case 10:////wander 10 to 60%
			memcpy(&TmpDataS16, &buf[1], 2);
			p->param.wander = TmpDataS16;
			break;
		case 11://spin 0 to 1000%
			memcpy(&TmpDataS16, &buf[1], 2);
			p->param.spin = TmpDataS16;
			break;
		case 12://inputlpf 200hz to 18000hz
			memcpy(&TmpDataS16, &buf[1], 2);
			p->param.inputlpf = TmpDataS16;
			break;
		case 13:////damplpf 200hz to 18000hz
			memcpy(&TmpDataS16, &buf[1], 2);
			p->param.damplpf = TmpDataS16;
			break;
		case 14:////basslpf 50hz to 1050hz
			memcpy(&TmpDataS16, &buf[1], 2);
			p->param.basslpf = TmpDataS16;
			break;
		case 15:////bassb 0 to 50%
			memcpy(&TmpDataS16, &buf[1], 2);
			p->param.bassb = TmpDataS16;
			break;
		case 16:////outputlpf 200hz to 18000hz
			memcpy(&TmpDataS16, &buf[1], 2);
			p->param.outputlpf = TmpDataS16;
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(ReverbProParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(ReverbProParam));
			//AudioEffectReverbProInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			break;
	}
	if((p->enable == TRUE) && (IsEffectChange != 1))
	{
		AudioEffectReverbProInit(p, p->channel, gCtrlVars.sample_rate);
	}
}
void Communication_Effect_ReverbPro(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	ReverbProUnit *p = (ReverbProUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(ReverbProParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_ReverbPro(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(ReverbProParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_VOICE_CHANGER_PRO_EN
void Comm_Effect_VoiceChangerPro(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
	uint16_t TmpData16;
	int16_t TmpDataS16;
	VoiceChangerProUnit *p = (VoiceChangerProUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpDataS16, &buf[1], 2);
			TmpDataS16 = TmpDataS16 > 300?300:TmpDataS16;
			TmpDataS16 = TmpDataS16 < 50?50:TmpDataS16;
			if(p->param.pitch_ratio  != TmpDataS16)
			{
				p->param.pitch_ratio = TmpDataS16;
				AudioEffectVoiceChangerProInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 2:
			memcpy(&TmpDataS16, &buf[1], 2);
			TmpDataS16 = TmpDataS16 > 200?200:TmpDataS16;
			TmpDataS16 = TmpDataS16 < 66? 66:TmpDataS16;

			if(p->param.formant_ratio != TmpDataS16)
			{
				p->param.formant_ratio = TmpDataS16;
				AudioEffectVoiceChangerProInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(VoiceChangerProParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(VoiceChangerProParam));
			AudioEffectVoiceChangerProInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			break;
	}
}
void Communication_Effect_VoiceChangerPro(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	VoiceChangerProUnit *p = (VoiceChangerProUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(VoiceChangerProParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_VoiceChangerPro(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(VoiceChangerProParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_PHASE_CONTROL_EN
void Comm_Effect_Phase(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
	uint16_t TmpData16;
	PhaseControlUnit *p = (PhaseControlUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 1)
			{
				TmpData16 = 0;
			}
			p->param.phase_difference = TmpData16;
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(PhaseControlParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(PhaseControlParam));
			break;
		default:
			break;
	}
}
void Communication_Effect_Phase(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	PhaseControlUnit *p = (PhaseControlUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(PhaseControlParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_Phase(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(PhaseControlParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_VOCAL_REMOVE_EN
void Comm_Effect_VocalRemove(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
	uint16_t TmpData16;
	VocalRemoveUnit *p = (VocalRemoveUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 20000)
			{
				TmpData16 = 200;
			}
			p->param.lower_freq = TmpData16;
			AudioEffectVocalRemoveInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		case 2:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 20000)
			{
				TmpData16 = 15000;
			}
			p->param.higher_freq = TmpData16;
			AudioEffectVocalRemoveInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(VocalRemoverParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(VocalRemoverParam));
			AudioEffectVocalRemoveInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			break;
	}
}
void Communication_Effect_VocalRemove(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	VocalRemoveUnit *p = (VocalRemoveUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(VocalRemoverParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_VocalRemove(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(VocalRemoverParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_PITCH_SHIFTER_PRO_EN
void Comm_Effect_PitchShifterPro(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
	uint16_t TmpData16;
	int16_t TmpDataS16;
	PitchShifterProUnit *p = (PitchShifterProUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpDataS16, &buf[1], 2);
			TmpDataS16 = TmpDataS16 > 120? 120:TmpDataS16;
			TmpDataS16 = TmpDataS16 < -120? -120:TmpDataS16;
			if(p->param.semitone_steps != TmpDataS16)
			{
				p->param.semitone_steps = TmpDataS16;
				AudioEffectPitchShifterProInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(PitchShifterProParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(PitchShifterProParam));
			AudioEffectPitchShifterProInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			break;
	}
}
void Communication_Effect_PitchShifterPro(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	PitchShifterProUnit *p = (PitchShifterProUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(PitchShifterProParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_PitchShifterPro(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(PitchShifterProParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_VB_CLASS_EN
void Comm_Effect_VBClass(EffectNode * pNode, uint8_t * buf, uint8_t index)
{
	int16_t TmpData16;
	VBClassUnit *p = (VBClassUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 300)
			{
				TmpData16 = 300;
			}
			else if(TmpData16 < 30)
			{
				TmpData16 = 30;
			}
			if(p->param.f_cut != TmpData16)
			{
				p->param.f_cut = TmpData16;
				AudioEffectVBClassInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 2:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 100;
			}
			if(p->param.intensity != TmpData16)
			{
				p->param.intensity = TmpData16;
				AudioEffectVBClassInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(VBClassicParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(VBClassicParam));
			AudioEffectVBClassInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			break;
	}
}

void Communication_Effect_VBClass(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	VBClassUnit *p = (VBClassUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(VBClassicParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_VBClass(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(VBClassicParam));
		}
	}
}
#endif

void Communication_Effect_SilenceDetector(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
    uint16_t TmpData16;

	EffectNode *pNode = addr;
	SilenceDetectorUnit *p = (SilenceDetectorUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(SilenceDetectorParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		switch(buf[0])//
		{
			case 0:
			case 0xFF:
				memcpy(&TmpData16, &buf[1], 2);
				if(p == NULL)
				{
					if(TmpData16 == 1)
					{
						pNode->Enable = 1;
						IsEffectChange = 1;
					}
				}
				else
				{
					if(p->enable != TmpData16)
					{
						p->enable = TmpData16;
						pNode->Enable = p->enable;
						IsEffectChange = 1;
					}
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[Control - 0x81] = pNode->Enable;
#endif
				break;
			default:
				break;
		}
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(SilenceDetectorParam));
		}
	}
}

#if CFG_AUDIO_EFFECT_PCM_DELAY_EN
void Comm_Effect_Delay(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
	int16_t TmpData16;
	PcmDelayUnit *p = (PcmDelayUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
//					if(p->enable)
//					{
//						IsEffectChange = 1;
//						//AudioEffectPcmDelayInit(p, p->channel, gCtrlVars.sample_rate);
//					}
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(p->param.delay != TmpData16)
			{
				p->param.delay = TmpData16;
				if(p->param.delay > p->param.max_delay)
				{
					p->param.delay = p->param.max_delay;
				}
			}
			break;
		case 2:
			memcpy(&TmpData16, &buf[1], 2);
			if(p->param.max_delay != TmpData16)
			{
				p->param.max_delay = TmpData16;
				if(p->param.high_quality)
				{
					if(p->param.max_delay > 1000)
					{
						p->param.max_delay = 1000;
					}
				}
				else
				{
					if(p->param.max_delay > 3000)
					{
						p->param.max_delay = 3000;
					}
				}
				p->param.delay = p->param.delay > p->param.max_delay? p->param.max_delay : p->param.delay;
				IsEffectChange = 1;
			}
			break;
		case 3:
			memcpy(&TmpData16, &buf[1], 2);
			if(p->param.high_quality != TmpData16)
			{
				p->param.high_quality = TmpData16;
				if(p->param.high_quality)
				{
					if(p->param.max_delay > 1000)
					{
						p->param.max_delay = 1000;
					}
				}
				else
				{
					if(p->param.max_delay > 3000)
					{
						p->param.max_delay = 3000;
					}
				}
				p->param.delay = p->param.delay > p->param.max_delay? p->param.max_delay : p->param.delay;
				IsEffectChange = 1;
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(PcmDelayParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(PcmDelayParam));
			IsEffectChange = 1;//重新load
			break;
		default:
			break;
	}
}
void Communication_Effect_Delay(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	PcmDelayUnit *p = (PcmDelayUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(PcmDelayParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_Delay(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(PcmDelayParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_EXCITER_EN
void Comm_Effect_HarmonicExciter(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
	int16_t TmpData16;
	ExciterUnit *p = (ExciterUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if((TmpData16 > EXCITER_MAX_CUTOFF_FREQ) || (TmpData16 < EXCITER_MIN_CUTOFF_FREQ))
			{
				TmpData16 = EXCITER_MIN_CUTOFF_FREQ;
			}
			if(p->param.f_cut != TmpData16)
			{
				p->param.f_cut = TmpData16;
				AudioEffectExciterInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 2:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 80;
			}
			if(p->param.dry != TmpData16)
			{
				p->param.dry = TmpData16;
			}
			break;
		case 3:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 80;
			}
			if(p->param.wet != TmpData16)
			{
				p->param.wet = TmpData16;
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(ExciterParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(ExciterParam));
			AudioEffectExciterInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			break;
	}
}
void Communication_Effect_HarmonicExciter(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	ExciterUnit *p = (ExciterUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(ExciterParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_HarmonicExciter(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(ExciterParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_CHORUS_EN
void Comm_Effect_Chorus(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
	int16_t TmpData16;
	ChorusUnit *p = (ChorusUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 25)
			{
				TmpData16 = 13;
			}
			p->param.delay_length = TmpData16;
			break;
		case 2:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 24)
			{
				TmpData16 = 3;
			}
			p->param.mod_depth = TmpData16;
			break;
		case 3:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 10;
			}
			p->param.mod_rate = TmpData16;
			break;
		case 4:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 50)
			{
				TmpData16 = 30;
			}
			p->param.feedback = TmpData16;
			break;
		case 5:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 90;
			}
			p->param.dry = TmpData16;
			break;
		case 6:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 60;
			}
			p->param.wet = TmpData16;
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(ChorusParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(ChorusParam));
			AudioEffectChorusInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			break;
	}
}
void Communication_Effect_Chorus(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	ChorusUnit *p = (ChorusUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(ChorusParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_Chorus(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(ChorusParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_AUTOWAH_EN
void Comm_Effect_AutoWah(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
	int16_t TmpData16;
	AutoWahUnit *p = (AutoWahUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(p->param.modulation_rate != TmpData16)
			{
				p->param.modulation_rate = TmpData16;
			}
			AudioEffectAutoWahInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		case 2:
			memcpy(&TmpData16, &buf[1], 2);
			if(p->param.min_frequency != TmpData16)
			{
				p->param.min_frequency = TmpData16;
			}
			AudioEffectAutoWahInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		case 3:
			memcpy(&TmpData16, &buf[1], 2);
			if(p->param.max_frequency != TmpData16)
			{
				p->param.max_frequency = TmpData16;
			}
			AudioEffectAutoWahInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		case 4:
			memcpy(&TmpData16, &buf[1], 2);
			if(p->param.depth != TmpData16)
			{
				p->param.depth = TmpData16;
			}
			AudioEffectAutoWahInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		case 5:
			memcpy(&TmpData16, &buf[1], 2);
			p->param.dry = TmpData16;
			break;
		case 6:
			memcpy(&TmpData16, &buf[1], 2);
			p->param.wet = TmpData16;
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(AutoWahParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(AutoWahParam));
			AudioEffectAutoWahInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			break;
	}
}
void Communication_Effect_AutoWah(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	AutoWahUnit *p = (AutoWahUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(AutoWahParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_AutoWah(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(AutoWahParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_STEREO_WIDEN_EN
void Comm_Effect_StereoWindener(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
	int16_t TmpData16;
	StereoWidenerUnit *p = (StereoWidenerUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 1)
			{
				TmpData16 = 1;
			}
			p->param.shaping  = TmpData16;
			AudioEffectStereoWidenerInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(StereoWidenerParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(StereoWidenerParam));
			AudioEffectStereoWidenerInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			break;
	}
}
void Communication_Effect_StereoWindener(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	StereoWidenerUnit *p = (StereoWidenerUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(StereoWidenerParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_StereoWindener(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(StereoWidenerParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_PINGPONG_EN
void Comm_Effect_PingPong(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
	int16_t TmpData16;
	PingPongUnit *p = (PingPongUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			TmpData16 = TmpData16 == 0? 1 : TmpData16;
			if(p->param.attenuation != TmpData16)
			{
				p->param.attenuation = TmpData16;
			}
			break;
		case 2:
			memcpy(&TmpData16, &buf[1], 2);
			if(p->param.delay != TmpData16)
			{
				p->param.delay = TmpData16;
				if(p->param.delay > p->param.max_delay)
				{
					p->param.delay = p->param.max_delay;
				}
			}
			break;
		case 3:
			memcpy(&TmpData16, &buf[1], 2);
			if(p->param.high_quality_enable != TmpData16)
			{
				if(p->enable)
				{
					IsEffectChange = 1;//
				}
				p->param.high_quality_enable = TmpData16;
				if(p->param.high_quality_enable)
				{
					if(p->param.max_delay > 1000)
					{
						p->param.max_delay = 1000;
					}
				}
				else
				{
					if(p->param.max_delay > 3000)
					{
						p->param.max_delay = 3000;
					}
				}
				p->param.delay = p->param.delay > p->param.max_delay? p->param.max_delay : p->param.delay;
			}
			break;
		case 4:
			memcpy(&TmpData16, &buf[1], 2);
			p->param.wetdrymix = TmpData16;
			break;
		case 5:
			memcpy(&TmpData16, &buf[1], 2);
			if(p->param.max_delay != TmpData16)
			{
				p->param.max_delay = TmpData16;
				if(p->param.high_quality_enable)
				{
					if(p->param.max_delay > 1000)
					{
						p->param.max_delay = 1000;
					}
				}
				else
				{
					if(p->param.max_delay > 3000)
					{
						p->param.max_delay = 3000;
					}
				}
				p->param.delay = p->param.delay > p->param.max_delay? p->param.max_delay : p->param.delay;
				if(p->enable)
				{
					IsEffectChange = 1;
				}
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(PingPongParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(PingPongParam));
			AudioEffectPingPongInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			break;
	}
}
void Communication_Effect_PingPong(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	PingPongUnit *p = (PingPongUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(PingPongParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];;
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_PingPong(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(PingPongParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_3D_PLUS_EN
void Comm_Effect_ThreeDPlus(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
	int16_t TmpData16;
	ThreeDUnit *p = (ThreeDUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 100)
			{
				TmpData16 = 100;
			}
			if(p->param.intensity != TmpData16)
			{
				p->param.intensity = TmpData16;
				AudioEffectThreeDPlusInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(ThreeDPlusParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(ThreeDPlusParam));
			AudioEffectThreeDPlusInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			break;
	}
}
void Communication_Effect_ThreeDPlus(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	ThreeDUnit *p = (ThreeDUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(ThreeDPlusParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];;
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_ThreeDPlus(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(ThreeDPlusParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_NS_BLUE_EN
void Comm_Effect_NSBlue(EffectNode *pNode, uint8_t * buf)
{
	int16_t TmpData16;
	NSBlueUnit *p = (NSBlueUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16>9)
			{
				TmpData16 = 9;
			}
			p->param.level = TmpData16;
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(NSBlueParam));
			AudioEffectNSBlueInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		default:
			break;
	}
}
void Communication_Effect_NSBlue(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	NSBlueUnit *p = (NSBlueUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(NSBlueParam);
		uint8_t Enable = 0;
		void* pParam = NULL;
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_NSBlue(pNode, buf);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(NSBlueParam));
		}
	}
}
#endif

//-----------------
#if CFG_AUDIO_EFFECT_FLANGER_EN
void Comm_Effect_Flanger(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
    uint16_t TmpData16;
    //int16_t TmpDataS16;
    FlangerUnit *p = (FlangerUnit *)pNode->EffectUnit;
	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(p->param.delay_length != TmpData16)
			{
				if((TmpData16 > 15) || (TmpData16 < 1))
				{
					TmpData16 = 1;
				}
				p->param.delay_length = TmpData16;
				if(p->enable)
				{
					AudioEffectFlangerInit(p,  gCtrlVars.sample_rate);
				}
			}
			break;
		case 2:
			memcpy(&TmpData16, &buf[1], 2);
			if(p->param.mod_depth != TmpData16)
			{
				if(TmpData16 > p->param.delay_length)
				{
					TmpData16 = p->param.delay_length;
				}
				p->param.mod_depth = TmpData16;
				if(p->enable)
				{
					AudioEffectFlangerInit(p,  gCtrlVars.sample_rate);
				}
			}
			break;
		case 3:
			memcpy(&TmpData16, &buf[1], 2);
			if(p->param.mod_rate != TmpData16)
			{
				if(TmpData16 > 1000)
				{
					TmpData16 = 1000;
				}
				p->param.mod_rate = TmpData16;
				if(p->enable)
				{
					AudioEffectFlangerInit(p,  gCtrlVars.sample_rate);
				}
			}
			break;
		case 4:
			memcpy(&TmpData16, &buf[1], 2);
			if(p->param.feedback != TmpData16)
			{
				if(TmpData16 > 100)
				{
					TmpData16 = 100;
				}
				p->param.feedback = TmpData16;
			}
			break;
		case 5:
			memcpy(&TmpData16, &buf[1], 2);
			if(p->param.dry != TmpData16)
			{
				if(TmpData16 > 100)
				{
					TmpData16 = 100;
				}
				p->param.dry = TmpData16;
			}
		case 6:
			memcpy(&TmpData16, &buf[1], 2);
			if(p->param.wet != TmpData16)
			{
				if(TmpData16 > 100)
				{
					TmpData16 = 100;
				}
				p->param.wet = TmpData16;
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(FlangerParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(FlangerParam));
			AudioEffectFlangerInit(p,  gCtrlVars.sample_rate);
			break;
		 default:
		 	break;
	}
}

void Communication_Effect_Flanger(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	FlangerUnit *p = (FlangerUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(FlangerParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_Flanger(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(FlangerParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_FREQ_SHIFTER_FINE_EN
void Comm_Effect_FreqShifterFine(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
    uint16_t TmpData16;
    int16_t TmpDataS16;
	FreqShifterFineUnit *p = (FreqShifterFineUnit *)pNode->EffectUnit;;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 >  1000) TmpDataS16 = 1000;
			if(TmpDataS16 < -1000) TmpDataS16 = -1000;
			if(TmpData16 != p->param.deltaf)
			{
				p->param.deltaf = TmpDataS16;
				AudioEffectFreqShifterFineInit(p, gCtrlVars.sample_rate);
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(FreqShifterFineParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(FreqShifterFineParam));
			AudioEffectFreqShifterFineInit(p, gCtrlVars.sample_rate);
			break;
		 default:
		 	break;
	}
}

void Communication_Effect_FreqShifterFine(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	FreqShifterFineUnit *p = (FreqShifterFineUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(FreqShifterFineParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_FreqShifterFine(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(FreqShifterFineParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_OVERDRIVE_EN
void Comm_Effect_Overdrive(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
    uint16_t TmpData16;
    OverdriveUnit *p = (OverdriveUnit *)pNode->EffectUnit;;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 >  10923) TmpData16 = 10923;
			if(TmpData16 < 4096) TmpData16 = 4096;
			p->param.threshold_compression = TmpData16;
			AudioEffectOverdriveInit(p, gCtrlVars.sample_rate);
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(OverdriveParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(OverdriveParam));
			AudioEffectOverdriveInit(p, gCtrlVars.sample_rate);
			break;
		 default:
		 	break;
	}
}

void Communication_Effect_Overdrive(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	OverdriveUnit *p = (OverdriveUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(OverdriveParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_Overdrive(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(OverdriveParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_DISTORTION_EN
void Comm_Effect_Distortion(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
    uint16_t TmpData16;
    DistortionUnit *p = (DistortionUnit *)pNode->EffectUnit;;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 >  48) TmpData16 = 48;
			p->param.gain = TmpData16;
			break;
		case 2:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 >  100) TmpData16 = 100;
			p->param.wet = TmpData16;
			break;
		case 3:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 >  100) TmpData16 = 100;
			p->param.dry = TmpData16;
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(DistortionParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(DistortionParam));
			AudioEffectDistortionInit(p, gCtrlVars.sample_rate);
			break;
		 default:
		 	break;
	}
}

void Communication_Effect_Distortion(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	DistortionUnit *p = (DistortionUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(DistortionParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_Distortion(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(DistortionParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_EQDRC_EN
void Comm_Effect_EQ_DRC(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
    uint16_t TmpData16,TmpData16_1;
	int16_t TmpDataS16;
	int16_t TmpData1S16;
	int16_t TmpData2S16;
	int16_t TmpData3S16;
	uint8_t filter_num;
	//int16_t TmpData, TmpDataS16;

    EQDRCUnit *p = (EQDRCUnit *)pNode->EffectUnit;;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			break;
		case 2:
			break;
		case 53:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 > 4)
			{
				TmpData16 = 4;
			}

			if(p->param.param_drc.mode != TmpData16)
			{
				p->param.param_drc.mode = TmpData16;
				AudioEffectEQDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 54:
			memcpy(&TmpData16, &buf[1], 2);
			if(TmpData16 ==0)
			{
				TmpData16 = 1;
			}
			if(TmpData16 > 4)
			{
				TmpData16 = 4;
			}

			if(p->param.param_drc.cf_type != TmpData16)
			{
				p->param.param_drc.cf_type = TmpData16;
				AudioEffectEQDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 55:
			memcpy(&TmpDataS16, &buf[1], 2);

			if(TmpDataS16 != p->param.param_drc.q_l)
			{
				p->param.param_drc.q_l = TmpDataS16;
				AudioEffectEQDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 56:
			memcpy(&TmpDataS16, &buf[1], 2);

			if(TmpDataS16 != p->param.param_drc.q_h)
			{
				p->param.param_drc.q_h = TmpDataS16;
				AudioEffectEQDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 57:
			memcpy(&TmpData16, &buf[1], 2);
			memcpy(&TmpData16_1, &buf[3], 2);
			if(TmpData16 > 20000)
			{
				TmpData16 = 20000;
			}
			if(TmpData16 < 20)
			{
				TmpData16 = 20;
			}
			if(TmpData16_1 > 20000)
			{
				TmpData16_1 = 20000;
			}
			if(TmpData16_1 < 20)
			{
				TmpData16_1 = 20;
			}
			if((TmpData16 != p->param.param_drc.fc[0]) || (TmpData16_1 != p->param.param_drc.fc[1]))
			{
				p->param.param_drc.fc[0] = TmpData16;
				p->param.param_drc.fc[1] = TmpData16_1;
				AudioEffectEQDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 58:
			memcpy(&TmpDataS16, &buf[1], 2);
			memcpy(&TmpData1S16, &buf[3], 2);
			memcpy(&TmpData2S16, &buf[5], 2);
			memcpy(&TmpData3S16, &buf[7], 2);

			TmpDataS16 = TmpDataS16 < -9000? -9000 : TmpDataS16;
			TmpDataS16 = TmpDataS16 > 0? 0 : TmpDataS16;
			TmpData1S16 = TmpData1S16 < -9000? -9000 : TmpData1S16;
			TmpData1S16 = TmpData1S16 > 0? 0 : TmpData1S16;
			TmpData2S16 = TmpData2S16 < -9000? -9000 : TmpData2S16;
			TmpData2S16 = TmpData2S16 > 0? 0 : TmpData2S16;
			TmpData3S16 = TmpData3S16 < -9000? -9000 : TmpData3S16;
			TmpData3S16 = TmpData3S16 > 0? 0 : TmpData3S16;

			if((TmpDataS16 != p->param.param_drc.threshold[0]) || (TmpData1S16 != p->param.param_drc.threshold[1]) || (TmpData2S16 != p->param.param_drc.threshold[2])|| (TmpData3S16 != p->param.param_drc.threshold[3]))
			{
				p->param.param_drc.threshold[0] = TmpDataS16;
				p->param.param_drc.threshold[1] = TmpData1S16;
				p->param.param_drc.threshold[2] = TmpData2S16;
				p->param.param_drc.threshold[3] = TmpData3S16;
				AudioEffectEQDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 59:
			memcpy(&TmpDataS16, &buf[1], 2);
			memcpy(&TmpData1S16, &buf[3], 2);
			memcpy(&TmpData2S16, &buf[5], 2);
			memcpy(&TmpData3S16, &buf[7], 2);

			TmpDataS16  = TmpDataS16 < 1? 	   1 : TmpDataS16;
			TmpDataS16  = TmpDataS16 > 1000?	1000 : TmpDataS16;
			TmpData1S16 = TmpData1S16 < 1?	   1 : TmpData1S16;
			TmpData1S16 = TmpData1S16 > 1000? 1000 : TmpData1S16;
			TmpData2S16 = TmpData2S16 < 1?	   1 : TmpData2S16;
			TmpData2S16 = TmpData2S16 > 1000? 1000 : TmpData2S16;
			TmpData3S16 = TmpData3S16 < 1?	   1 : TmpData3S16;
			TmpData3S16 = TmpData3S16 > 1000? 1000 : TmpData3S16;

			if((TmpDataS16 != p->param.param_drc.ratio[0]) || (TmpData1S16 != p->param.param_drc.ratio[1]) || (TmpData2S16 != p->param.param_drc.ratio[2])|| (TmpData3S16 != p->param.param_drc.ratio[3]))
			{
				p->param.param_drc.ratio[0] = TmpDataS16;
				p->param.param_drc.ratio[1] = TmpData1S16;
				p->param.param_drc.ratio[2] = TmpData2S16;
				p->param.param_drc.ratio[3] = TmpData3S16;
				AudioEffectEQDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 60:
			memcpy(&TmpDataS16, &buf[1], 2);
			memcpy(&TmpData1S16, &buf[3], 2);
			memcpy(&TmpData2S16, &buf[5], 2);
			memcpy(&TmpData3S16, &buf[7], 2);

			TmpDataS16  = TmpDataS16 < 0? 	   0 : TmpDataS16;
			TmpDataS16  = TmpDataS16 > 7500?	7500 : TmpDataS16;
			TmpData1S16 = TmpData1S16 < 0?	   0 : TmpData1S16;
			TmpData1S16 = TmpData1S16 > 7500? 7500 : TmpData1S16;
			TmpData2S16 = TmpData2S16 < 0?	   0 : TmpData2S16;
			TmpData2S16 = TmpData2S16 > 7500? 7500 : TmpData2S16;
			TmpData3S16 = TmpData3S16 < 0?	   0 : TmpData3S16;
			TmpData3S16 = TmpData3S16 > 7500? 7500 : TmpData3S16;

			if((TmpDataS16 != p->param.param_drc.attack_tc[0]) || (TmpData1S16 != p->param.param_drc.attack_tc[1]) || (TmpData2S16 != p->param.param_drc.attack_tc[2])|| (TmpData3S16 != p->param.param_drc.attack_tc[3]))
			{
				p->param.param_drc.attack_tc[0] = TmpDataS16;
				p->param.param_drc.attack_tc[1] = TmpData1S16;
				p->param.param_drc.attack_tc[2] = TmpData2S16;
				p->param.param_drc.attack_tc[3] = TmpData3S16;
				AudioEffectEQDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 61:
			memcpy(&TmpDataS16, &buf[1], 2);
			memcpy(&TmpData1S16, &buf[3], 2);
			memcpy(&TmpData2S16, &buf[5], 2);
			memcpy(&TmpData3S16, &buf[7], 2);

			TmpDataS16  = TmpDataS16 < 0? 	   0 : TmpDataS16;
			TmpDataS16  = TmpDataS16 > 7500?	7500 : TmpDataS16;
			TmpData1S16 = TmpData1S16 < 0?	   0 : TmpData1S16;
			TmpData1S16 = TmpData1S16 > 7500? 7500 : TmpData1S16;
			TmpData2S16 = TmpData2S16 < 0?	   0 : TmpData2S16;
			TmpData2S16 = TmpData2S16 > 7500? 7500 : TmpData2S16;
			TmpData3S16 = TmpData3S16 < 0?	   0 : TmpData3S16;
			TmpData3S16 = TmpData3S16 > 7500? 7500 : TmpData3S16;

			if((TmpDataS16 != p->param.param_drc.release_tc[0]) || (TmpData1S16 != p->param.param_drc.release_tc[1]) || (TmpData2S16 != p->param.param_drc.release_tc[2])|| (TmpData3S16 != p->param.param_drc.release_tc[3]))
			{
				p->param.param_drc.release_tc[0] = TmpDataS16;
				p->param.param_drc.release_tc[1] = TmpData1S16;
				p->param.param_drc.release_tc[2] = TmpData2S16;
				p->param.param_drc.release_tc[3] = TmpData3S16;
				AudioEffectEQDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 62:
			memcpy(&TmpDataS16, &buf[1], 2);
			memcpy(&TmpData1S16, &buf[3], 2);
			memcpy(&TmpData2S16, &buf[5], 2);
			memcpy(&TmpData3S16, &buf[7], 2);

			TmpDataS16  = TmpDataS16 > 32536?	32536 : TmpDataS16;
			TmpData1S16 = TmpData1S16 > 32536?  32536 : TmpData1S16;
			TmpData2S16 = TmpData2S16 > 32536?  32536 : TmpData2S16;
			TmpData3S16 = TmpData3S16 > 32536?  32536 : TmpData3S16;

			p->param.param_drc.pregain[0] = TmpDataS16;
			p->param.param_drc.pregain[1] = TmpData1S16;
			p->param.param_drc.pregain[2] = TmpData2S16;
			p->param.param_drc.pregain[3] = TmpData3S16;
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(EQDRCParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(EQDRCParam));
			AudioEffectEQDRCInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		 default:
			if(buf[0] < 53)
			{
				filter_num = (buf[0]- 3) / 5;
				memcpy(&TmpDataS16, &buf[1], 2);
				if(((buf[0]- 3) % 5) == 0)
				{
					p->param.param_eq.eq_params[filter_num].enable = TmpDataS16;
				}
				else if(((buf[0]- 3) % 5) == 1)
				{
					p->param.param_eq.eq_params[filter_num].type = TmpDataS16;
				}
				else if(((buf[0]- 3) % 5) == 2)
				{
					p->param.param_eq.eq_params[filter_num].f0 = TmpDataS16;
				}
				else if(((buf[0]- 3) % 5) == 3)
				{
					p->param.param_eq.eq_params[filter_num].Q = TmpDataS16;
				}
				else if(((buf[0]- 3) % 5) == 4)
				{
					p->param.param_eq.eq_params[filter_num].gain = TmpDataS16;
				}
				AudioEffectEQDRCInit(p, p->channel, gCtrlVars.sample_rate);
			}
		 	break;
	}
}

void Communication_Effect_EQ_DRC(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	EQDRCUnit *p = (EQDRCUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(EQDRCParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_EQ_DRC(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(EQDRCParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_AEC_EN
void Comm_Effect_AEC(EffectNode * pNode, uint8_t * buf, uint8_t index)
{
	AECUnit *p = (AECUnit *)pNode->EffectUnit;
	int16_t TmpData;
	switch(buf[0])
	{
		case 0:
			memcpy(&TmpData, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData)
				{
					p->enable = TmpData;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData, &buf[1], 2);
			if(TmpData > 5)
			{
				TmpData = 5;
			}
			if(p->param.es_level != TmpData)
			{
				p->param.es_level = TmpData;
				AudioEffectAECInit(p, 1, 16000);//固定为16K采样率
			}
			break;
		case 2:
			memcpy(&TmpData, &buf[1], 2);
			if(TmpData > 5)
			{
				TmpData = 5;
			}
			if(p->param.ns_level != TmpData)
			{
				p->param.ns_level = TmpData;
				AudioEffectAECInit(p, 1, 16000);//固定为16K采样率
			}
			break;
		case 0xf0://自定义音效，全部参数为0xF0
			memcpy(&TmpData, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(AECParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData)
			{
				p->enable = TmpData;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(AECParam));
			if(p->enable)
			{
				AudioEffectAECInit(p, 1, 16000);//固定为16K采样率
			}
			break;
		default:
			break;
	}
}

void Communication_Effect_AEC(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	AECUnit *p = (AECUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		APP_DBG("Communication_Effect_AEC ask\n");
		memset(tx_buf, 0, sizeof(tx_buf));
		uint16_t LenParam = sizeof(AECParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_AEC(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(AECParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_DISTORTION_DS1_EN
void Comm_Effect_DistortionDS1(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
    uint16_t TmpData16;
	int16_t TmpDataS16;
	DistortionDS1Unit *p = (DistortionDS1Unit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 < 0)
			{
				TmpDataS16 = 0;
			}
			else if(TmpDataS16 > 100)
			{
				TmpDataS16 = 100;
			}
			if(p->param.distortion_level != TmpDataS16)
			{
				p->param.gain = TmpDataS16;
				AudioEffectDistortionDS1Init(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 2:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 > 100)
			{
				TmpDataS16 = 100;
			}
			else if(TmpDataS16 < 0)
			{
				TmpDataS16 = 0;
			}
			if(p->param.out_level != TmpDataS16)
			{
				p->param.out_level = TmpDataS16;
				AudioEffectDistortionDS1Init(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(DistortionDS1Param));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(DistortionDS1Param));
			AudioEffectDistortionDS1Init(p, p->channel, gCtrlVars.sample_rate);
			break;
		 default:
		 	break;
	}
}

void Communication_Effect_DistortionDS1(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	DistortionDS1Unit *p = (DistortionDS1Unit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(DistortionDS1Param);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_DistortionDS1(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(DistortionDS1Param));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_OVERDRIVE_POLY_EN
void Comm_Effect_OverdrivePoly(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
    uint16_t TmpData16;
	int16_t TmpDataS16;
	OverdrivePolyUnit *p = (OverdrivePolyUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 < 0)
			{
				TmpDataS16 = 0;
			}
			else if(TmpDataS16 > 48)
			{
				TmpDataS16 = 48;
			}
			if(p->param.gain != TmpDataS16)
			{
				p->param.gain = TmpDataS16;
				AudioEffectOverdrivePolyInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 2:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 > 100)
			{
				TmpDataS16 = 100;
			}
			else if(TmpDataS16 < 0)
			{
				TmpDataS16 = 0;
			}
			if(p->param.out_level != TmpDataS16)
			{
				p->param.out_level = TmpDataS16;
				AudioEffectOverdrivePolyInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(OverdrivePolyParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(OverdrivePolyParam));
			AudioEffectOverdrivePolyInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		 default:
		 	break;
	}
}

void Communication_Effect_OverdrivePoly(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	OverdrivePolyUnit *p = (OverdrivePolyUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(OverdrivePolyParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_OverdrivePoly(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(OverdrivePolyParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_COMPANDER_EN
void Comm_Effect_Compander(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
    uint16_t TmpData16;
	int16_t TmpDataS16;
	CompanderUnit *p = (CompanderUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 < -9000)
			{
				TmpDataS16 = -9000;
			}
			else if(TmpDataS16 > 0)
			{
				TmpDataS16 = 0;
			}
			if(p->param.threshold != TmpDataS16)
			{
				p->param.threshold = TmpDataS16;
				AudioEffectCompanderInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 2:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 > 10000)
			{
				TmpDataS16 = 10000;
			}
			else if(TmpDataS16 < 1)
			{
				TmpDataS16 = 1;
			}
			if(p->param.slope_below != TmpDataS16)
			{
				p->param.slope_below = TmpDataS16;
				AudioEffectCompanderInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 3:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 > 10000)
			{
				TmpDataS16 = 10000;
			}
			else if(TmpDataS16 < 1)
			{
				TmpData16 = 1;
			}
			if(p->param.slope_above != TmpDataS16)
			{
				p->param.slope_above = TmpDataS16;
				AudioEffectCompanderInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 4:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 > 32767)
			{
				TmpDataS16 = 32767;
			}
			else if(TmpDataS16 < 0)
			{
				TmpDataS16 = 0;
			}
			if(p->param.alpha_attack != TmpDataS16)
			{
				p->param.alpha_attack = TmpDataS16;
				AudioEffectCompanderInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 5:
				memcpy(&TmpDataS16, &buf[1], 2);
				if(TmpDataS16 > 32767)
				{
					TmpDataS16 = 32767;
				}
				else if(TmpDataS16 < 0)
				{
					TmpDataS16 = 0;
				}
				if(p->param.alpha_release != TmpDataS16)
				{
					p->param.alpha_release = TmpDataS16;
					AudioEffectCompanderInit(p, p->channel, gCtrlVars.sample_rate);
				}
				break;
		case 6:
				memcpy(&TmpDataS16, &buf[1], 2);
				if(TmpDataS16 > 1800)
				{
					TmpDataS16 = 1800;
				}
				else if(TmpDataS16 < -7200)
				{
					TmpDataS16 = -7200;
				}
				if(p->param.pregain != TmpDataS16)
				{
					p->param.pregain = TmpDataS16;
					AudioEffectCompanderInit(p, p->channel, gCtrlVars.sample_rate);
				}
				break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(CompanderParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(CompanderParam));
			AudioEffectCompanderInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		 default:
		 	break;
	}
}

void Communication_Effect_Compander(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	CompanderUnit *p = (CompanderUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(CompanderParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_Compander(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(CompanderParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_LOW_LEVEL_COMPRESSOR_EN
void Comm_Effect_LowLevelCompressor(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
    uint16_t TmpData16;
	int16_t TmpDataS16;
	LowLevelCompressorUnit *p = (LowLevelCompressorUnit *)pNode->EffectUnit;

	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 < -9600)
			{
				TmpDataS16 = -9600;
			}
			else if(TmpDataS16 > 0)
			{
				TmpDataS16 = 0;
			}
			if(p->param.threshold != TmpDataS16)
			{
				p->param.threshold = TmpDataS16;
				AudioEffectLowLevelCompressorInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 2:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 > 4800)
			{
				TmpDataS16 = 4800;
			}
			else if(TmpDataS16 < 0)
			{
				TmpDataS16 = 0;
			}
			if(p->param.gain != TmpDataS16)
			{
				p->param.gain = TmpDataS16;
				AudioEffectLowLevelCompressorInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 3:
			memcpy(&TmpDataS16, &buf[1], 2);
			if(TmpDataS16 > 32767)
			{
				TmpDataS16 = 32767;
			}
			else if(TmpDataS16 < 0)
			{
				TmpDataS16 = 0;
			}
			if(p->param.alpha_attack != TmpDataS16)
			{
				p->param.alpha_attack = TmpDataS16;
				AudioEffectLowLevelCompressorInit(p, p->channel, gCtrlVars.sample_rate);
			}
			break;
		case 4:
				memcpy(&TmpDataS16, &buf[1], 2);
				if(TmpDataS16 > 32767)
				{
					TmpDataS16 = 32767;
				}
				else if(TmpDataS16 < 0)
				{
					TmpDataS16 = 0;
				}
				if(p->param.alpha_release != TmpDataS16)
				{
					p->param.alpha_release = TmpDataS16;
					AudioEffectLowLevelCompressorInit(p, p->channel, gCtrlVars.sample_rate);
				}
				break;
		case 0xFF:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(LLCompressorParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(LLCompressorParam));
			AudioEffectLowLevelCompressorInit(p, p->channel, gCtrlVars.sample_rate);
			break;
		 default:
		 	break;
	}
}

void Communication_Effect_LowLevelCompressor(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = addr;
	LowLevelCompressorUnit *p = (LowLevelCompressorUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(LLCompressorParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_LowLevelCompressor(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(LLCompressorParam));
		}
	}
}
#endif

#if CFG_AUDIO_EFFECT_HOWLING_SUPPRESSOR_FINE_EN
void Comm_Effect_HowlingSuppressorFine(EffectNode *pNode, uint8_t * buf, uint8_t index)
{
    int16_t TmpData16;
    HowlingFineUnit *p = (HowlingFineUnit *)pNode->EffectUnit;
	switch(buf[0])//
	{
		case 0:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
			}
			else
			{
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					pNode->Enable = p->enable;
					IsEffectChange = 1;
				}
			}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
			effect_enable_list[index] = pNode->Enable;
#endif
			break;
		case 1:
			memcpy(&TmpData16, &buf[1], 2);
			p->param.q_min = TmpData16;
			if(p->param.q_min>p->param.q_max)
			{
				p->param.q_min=p->param.q_max;
			}
			break;
		case 2:
			memcpy(&TmpData16, &buf[1], 2);
			p->param.q_max = TmpData16;
			if(p->param.q_min>p->param.q_max)
			{
				p->param.q_min=p->param.q_max;
			}
			break;
		case 0xff:
			memcpy(&TmpData16, &buf[1], 2);
			if(p == NULL)
			{
				if(TmpData16 == 1)
				{
					pNode->Enable = 1;
					IsEffectChange = 1;
				}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
				effect_enable_list[index] = pNode->Enable;
#endif
				memcpy(effect_list_param[index], &buf[3], sizeof(HowlingFineParam));//导入ini文件时，如果音效默认开启和ini文件不同，默认未开启的音效会变回默认参数
				break;
			}
			if(p->enable != TmpData16)
			{
				p->enable = TmpData16;
				pNode->Enable = p->enable;
			}
			memcpy(&p->param, &buf[3], sizeof(HowlingFineParam));
			if(p->param.q_min>p->param.q_max)
			{
				p->param.q_min=p->param.q_max;
			}
			AudioEffectHowlingSuppressorFineInit(p,CFG_PARA_SAMPLE_RATE);
			break;
	}
}

void Communication_Effect_HowlingFine(uint8_t Control, EffectNode* addr, uint8_t *buf, uint32_t len)
{
	EffectNode *pNode = (EffectNode *)addr;
	HowlingFineUnit *p = (HowlingFineUnit *)pNode->EffectUnit;

	if(len == 0)//ask
	{
		uint16_t LenParam = sizeof(HowlingFineParam);
		uint8_t Enable = 0;
		void* pParam = effect_list_param[Control - 0x81];
		if(p != NULL)
		{
			Enable = p->enable;
			pParam = (void *)&p->param;
		}
		Comm_Effect_Send(p, Enable, pParam, LenParam, Control);
	}
	else
	{
		Comm_Effect_HowlingSuppressorFine(pNode, buf, Control - 0x81);
		if(p != NULL)
		{
			memcpy(effect_list_param[Control - 0x81], &p->param, sizeof(HowlingFineParam));
		}
	}
}
#endif //end of CFG_AUDIO_EFFECT_HOWLING_SUPPRESSOR_FINE_EN

#include "audio_effect.h"
EffectNode* FindEffectNode(uint8_t index);
void Communication_Effect_After_0x80(uint8_t Control, uint8_t *buf, uint32_t len)
{
	EffectNode *node;

	node = FindEffectNode(Control - 0x81);
	//DBG("Control = %d\n", Control);
	switch(effect_list[Control - 0x81])
	{
#if CFG_AUDIO_EFFECT_AUTO_TUNE_EN
		case AUTO_TUNE:
			Communication_Effect_AutoTune(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_DRC_EN
		case DRC:
			Communication_Effect_DRC(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_ECHO_EN
		case ECHO:
			Communication_Effect_Echo(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_EQ_EN
		case EQ:
			Communication_Effect_EQ(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_NOISE_SUPPRESSOR_EN
		case EXPANDER:
			Communication_Effect_Expander(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_FREQ_SHIFTER_EN
		case FREQ_SHIFTER:
			Communication_Effect_FreqShifter(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_HOWLING_SUPPRESSOR_EN
        case HOWLING_SUPPRESSOR:
			Communication_Effect_HowlingSuppressor(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_PITCH_SHIFTER_EN
		case PITCH_SHIFTER:
			Communication_Effect_PitchShifter(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_REVERB_EN
		case REVERB:
			Communication_Effect_Reverb(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_SILENCE_DECTOR_EN
		case SILENCE_DETECTOR:
			Communication_Effect_SilenceDetector(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_3D_EN
        case THREE_D:
			Communication_Effect_ThreeD(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_VB_EN
		case VIRTUAL_BASS:
			Communication_Effect_VB(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_VOICE_CHANGER_EN
		case VOICE_CHANGER:
			Communication_Effect_VoiceChanger(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_GAIN_CONTROL_EN
		case GAIN_CONTROL:
			Communication_Effect_GainControl(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_VOCAL_CUT_EN
		case VOCAL_CUT:
			Communication_Effect_VocalCut(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_PLATE_REVERB_EN
        case PLATE_REVERB:
			Communication_Effect_PlateReverb(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_REVERB_PRO_EN
        case REVERB_PRO:
			Communication_Effect_ReverbPro(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_VOICE_CHANGER_PRO_EN
        case VOICE_CHANGER_PRO:
			Communication_Effect_VoiceChangerPro(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_PHASE_CONTROL_EN
		case PHASE_CONTROL:
			Communication_Effect_Phase(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_VOCAL_REMOVE_EN
		case VOCAL_REMOVE:
			Communication_Effect_VocalRemove(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_PITCH_SHIFTER_PRO_EN
		case PITCH_SHIFTER_PRO:
			Communication_Effect_PitchShifterPro(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_VB_CLASS_EN
		case VIRTUAL_BASS_CLASSIC:
			Communication_Effect_VBClass(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_PCM_DELAY_EN
		case PCM_DELAY:
			Communication_Effect_Delay(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_EXCITER_EN
		case EXCITER:
			Communication_Effect_HarmonicExciter(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_CHORUS_EN
		case CHORUS:
			Communication_Effect_Chorus(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_AUTOWAH_EN
		case AUTO_WAH:
			Communication_Effect_AutoWah(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_STEREO_WIDEN_EN
		case STEREO_WIDENER:
			Communication_Effect_StereoWindener(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_PINGPONG_EN
		case PINGPONG:
			Communication_Effect_PingPong(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_3D_PLUS_EN
		case THREE_D_PLUS:
			Communication_Effect_ThreeDPlus(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_NS_BLUE_EN
		case NOISE_Suppressor_Blue:
			Communication_Effect_NSBlue(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_FLANGER_EN
		case FLANGER:
			Communication_Effect_Flanger(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_FREQ_SHIFTER_FINE_EN
		case FREQ_SHIFTER_FINE:
			Communication_Effect_FreqShifterFine(Control, node, buf, len);
			break;
#endif
#if	CFG_AUDIO_EFFECT_OVERDRIVE_EN
		case OVERDRIVE:
			Communication_Effect_Overdrive(Control, node, buf, len);
			break;
#endif
#if	CFG_AUDIO_EFFECT_DISTORTION_EN
		case DISTORTION:
			Communication_Effect_Distortion(Control, node, buf, len);
			break;
#endif
#if	CFG_AUDIO_EFFECT_EQDRC_EN
		case EQ_DRC:
			Communication_Effect_EQ_DRC(Control, node, buf, len);
			break;
#endif
#if CFG_AUDIO_EFFECT_AEC_EN
		case AEC:
			Communication_Effect_AEC(Control, node, buf, len);
			break;
#endif
#if	CFG_AUDIO_EFFECT_DISTORTION_DS1_EN
		case DISTORTION_DS1:
			Communication_Effect_DistortionDS1(Control, node, buf, len);
			break;
#endif
#if	CFG_AUDIO_EFFECT_OVERDRIVE_POLY_EN
		case OVERDRIVE_POLY:
			Communication_Effect_OverdrivePoly(Control, node, buf, len);
			break;
#endif
#if	CFG_AUDIO_EFFECT_COMPANDER_EN
		case COMPANDER:
			Communication_Effect_Compander(Control, node, buf, len);
			break;
#endif
#if	CFG_AUDIO_EFFECT_LOW_LEVEL_COMPRESSOR_EN
		case LOW_LEVEL_COMPRESSOR:
			Communication_Effect_LowLevelCompressor(Control, node, buf, len);
			break;
#endif
#if	CFG_AUDIO_EFFECT_HOWLING_SUPPRESSOR_FINE_EN
		case HOWLING_FINE:
			Communication_Effect_HowlingFine(Control, node, buf, len);
			break;
#endif
		default:
			break;
	}
}

void Communication_Effect_Config(uint8_t Control, uint8_t *buf, uint32_t len)
{
	switch(Control)
	{
		case 0x00:
			Communication_Effect_0x00();
			break;
		case 0x01:
			Communication_Effect_0x01(buf, len);
			break;	
	    case 0x02:
			Communication_Effect_0x02();
			break;
		case 0x03:
			Communication_Effect_0x03(buf, len);
			break;
		case 0x04:
			Communication_Effect_0x04(buf, len);
			break;
		case 0x06:
			Communication_Effect_0x06(buf, len);
			break;
		case 0x07:
			Communication_Effect_0x07(buf, len);
			break;
		case 0x08:
			Communication_Effect_0x08(buf, len);
			break;
		case 0x09:
			Communication_Effect_0x09(buf, len);
			break;
		case 0x0A:
			Communication_Effect_0x0A(buf, len);
			break;
		case 0x0B://I2S0，不支持在线调音
			break;
		case 0x0C://I2S1，不支持在线调音
			break;
		case 0x0D://spdif，不支持在线调音
			break;
		case 0x80:
			Communication_Effect_0x80(buf, len);
			break;
		case 0xfc://user define tag
			 Communication_Effect_0xfc(buf, len);
			break;
		case 0xfd://user define tag
			 Communication_Effect_0xfd(buf, len);
			break;
		case 0xff:
			Communication_Effect_0xff(buf, len);
			break;		
		default:		
			if((Control >= 0x81) && (Control < 0xfe))
			{
			    #ifdef FUNC_OS_EN
				osMutexLock(AudioEffectMutex);
			    #endif
				Communication_Effect_After_0x80(Control, buf, len);
				#ifdef FUNC_OS_EN
				osMutexUnlock(AudioEffectMutex);
				#endif

				gCtrlVars.SamplesPerFrame = CFG_PARA_MIN_SAMPLES_PER_FRAME;

				//音效整体处理流程完善之后才能这样处理
				////AEC 帧长固定为256，后续有调音的情况下再做调整，sam，20190725
				//if(gCtrlVars.mic_aec_unit.enable)
				//{
				//	gCtrlVars.SamplesPerFrame = 256;
				//	APP_DBG("AEC samples = 256\n");
				//}
				
				if(gCtrlVars.SamplesPerFrame != AudioCoreFrameSizeGet(DefaultNet))
				{
					if(len > 0)//上位机正在设置
					{
						IsEffectSampleLenChange = 1;
					}
				}
			}	
			break;
	}
	//-----Send ACK ----------------------//
	if(Control > 0xf0)///有效控制寄存器范围   这句解决加调音参数加密问题
	{
		return;
	}
	if((Control > 2)&&(Control != 0x80))
	{
		if(len > 0)// if(len = 0) {polling all parameter}
		{
			memset(tx_buf, 0, sizeof(tx_buf));
			tx_buf[0] = Control;
			Communication_Effect_Send(tx_buf, 1);
		}
	}
}

void HIDUsb_Rx(uint8_t *buf,uint16_t len)
{
#ifdef CFG_COMMUNICATION_BY_USB
	 UsbLoadAudioMode(len,buf);
#endif
}

//* USB导入调音参数，并解析处理
void UsbLoadAudioMode(uint16_t len,uint8_t *buff)
{
#ifdef CFG_COMMUNICATION_BY_USB
	uint16_t cmd_len,packet_len,k;
	uint8_t end_code;
	uint16_t position;

	//加锁，防止频繁切换模式并连调音工具时出现音效参数解析异常问题出现
	if(LoadAudioParamMutex != NULL)
	{
		osMutexLock(LoadAudioParamMutex);
	}

	for(k = 0; k < sizeof(communic_buf); k++)//
	{
		communic_buf[k]=0;
	}

	communic_buf_w = 0;
	position = 0;
	while(position < len)
	{
		if ((buff[position] ==0xa5) && (buff[position+1] ==0x5a))
		{
			packet_len = buff[position + 3];

			end_code   = buff[position + 4 + packet_len];//0x16

			if(end_code == 0x16)////end code ok
			{
				cmd_len = packet_len + 5;
				communic_buf_w = 0;

				for(k = 0; k < cmd_len; k++)//get signal COMMAND
				{
					communic_buf[k]=buff[position++];
				}

				Communication_Effect_Config(communic_buf[2], &communic_buf[4], communic_buf[3]);
				for(k = 0; k < cmd_len; k++)//
				{
					communic_buf[k]=0;
				}
				communic_buf_w = 0;
			}
			else  // end code error
			{
				position++;
			}
	 	}
		else // serch start code....
		{
			position++;
		}
	}
	//解锁，防止频繁切换模式并连调音工具时出现音效参数解析异常问题出现
	if(LoadAudioParamMutex != NULL)
	{
		osMutexUnlock(LoadAudioParamMutex);
	}
#endif
}


#endif

