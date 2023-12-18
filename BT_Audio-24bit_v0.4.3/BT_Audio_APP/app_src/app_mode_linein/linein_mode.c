/**
 **************************************************************************************
 * @file    linein_mode.c
 * @brief   
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2017-12-26 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include "type.h"
#include "irqn.h"
#include "gpio.h"
#include "dma.h"
#include "rtos_api.h"
#include "app_message.h"
#include "app_config.h"
#include "debug.h"
#include "delay.h"
#include "audio_adc.h"
#include "dac.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "audio_core_api.h"
#include "audio_core_service.h"
#include "decoder.h"
#include "remind_sound.h"
#include "main_task.h"
#include "audio_effect.h"
#include "powercontroller.h"
#include "deepsleep.h"
#include "backup_interface.h"
#include "breakpoint.h"
#include "otg_device_hcd.h"
#include "otg_device_standard_request.h"
#include "audio_vol.h"
#include "ctrlvars.h"
#include "main_task.h"
#include "reset.h"
#include "mode_task_api.h"
#include "audio_effect_flash_param.h"

#ifdef CFG_APP_LINEIN_MODE_EN

#define LINEIN_PLAY_TASK_STACK_SIZE		512//1024
#define LINEIN_PLAY_TASK_PRIO			3
#define LINEIN_NUM_MESSAGE_QUEUE		10

#define LINEIN_SOURCE_NUM				APP_SOURCE_NUM

typedef struct _LineInPlayContext
{
	//xTaskHandle 		taskHandle;
	//MessageHandle		msgHandle;

	uint32_t			*ADCFIFO;			//ADC的DMA循环fifo
	AudioCoreContext 	*AudioCoreLineIn;

	//play
	uint32_t 			SampleRate; //带提示音时，如果不重采样，要避免采样率配置冲突
}LineInPlayContext;

/**根据appconfig缺省配置:DMA 8个通道配置**/
/*1、cec需PERIPHERAL_ID_TIMER3*/
/*2、SD卡录音需PERIPHERAL_ID_SDIO RX/TX*/
/*3、在线串口调音需PERIPHERAL_ID_UART1 RX/TX,建议使用USB HID，节省DMA资源*/
/*4、线路输入需PERIPHERAL_ID_AUDIO_ADC0_RX*/
/*5、Mic开启需PERIPHERAL_ID_AUDIO_ADC1_RX，mode之间通道必须一致*/
/*6、Dac0开启需PERIPHERAL_ID_AUDIO_DAC0_TX mode之间通道必须一致*/
/*7、DacX需开启PERIPHERAL_ID_AUDIO_DAC1_TX mode之间通道必须一致*/
/*注意DMA 8个通道配置冲突:*/
/*a、UART在线调音和DAC-X有冲突,默认在线调音使用USB HID*/
/*b、UART在线调音与HDMI/SPDIF模式冲突*/

static const uint8_t sDmaChannelMap[29] = {
	255,//PERIPHERAL_ID_SPIS_RX = 0,	//0
	255,//PERIPHERAL_ID_SPIS_TX,		//1
	
#ifdef CFG_APP_HDMIIN_MODE_EN
	5,//PERIPHERAL_ID_TIMER3,			//2
#else
	255,//PERIPHERAL_ID_TIMER3,			//2
#endif

#if defined (CFG_FUNC_I2S_MIX_MODE) && defined (CFG_RES_AUDIO_I2S1IN_EN)
	255,//PERIPHERAL_ID_SDIO_RX,		//3
	255,//PERIPHERAL_ID_SDIO_TX,		//4
#else
	4,//PERIPHERAL_ID_SDIO_RX,			//3
	4,//PERIPHERAL_ID_SDIO_TX,			//4
#endif
	
	255,//PERIPHERAL_ID_UART0_RX,		//5
	255,//PERIPHERAL_ID_TIMER1,			//6
	255,//PERIPHERAL_ID_TIMER2,			//7
	255,//PERIPHERAL_ID_SDPIF_RX,		//8 SPDIF_RX /TX same chanell
	255,//PERIPHERAL_ID_SDPIF_TX,		//8 SPDIF_RX /TX same chanell
	255,//PERIPHERAL_ID_SPIM_RX,		//9
	255,//PERIPHERAL_ID_SPIM_TX,		//10
	255,//PERIPHERAL_ID_UART0_TX,		//11
	
#ifdef CFG_COMMUNICATION_BY_UART	
	7,//PERIPHERAL_ID_UART1_RX,			//12
	6,//PERIPHERAL_ID_UART1_TX,			//13
#else
	255,//PERIPHERAL_ID_UART1_RX,		//12
	255,//PERIPHERAL_ID_UART1_TX,		//13
#endif

	255,//PERIPHERAL_ID_TIMER4,			//14
	255,//PERIPHERAL_ID_TIMER5,			//15
	255,//PERIPHERAL_ID_TIMER6,			//16
	0,//PERIPHERAL_ID_AUDIO_ADC0_RX,	//17
	1,//PERIPHERAL_ID_AUDIO_ADC1_RX,	//18
	2,//PERIPHERAL_ID_AUDIO_DAC0_TX,	//19
	3,//PERIPHERAL_ID_AUDIO_DAC1_TX,	//20

#if defined (CFG_FUNC_I2S_MIX_MODE) && defined (CFG_RES_AUDIO_I2S0IN_EN)	
	6,//PERIPHERAL_ID_I2S0_RX,			//21
#else
	255,//PERIPHERAL_ID_I2S0_RX,		//21
#endif

#if	((defined(CFG_RES_AUDIO_I2SOUT_EN )&&(CFG_RES_I2S == 0)) || defined(CFG_RES_AUDIO_I2S0OUT_EN))
	7,//PERIPHERAL_ID_I2S0_TX,			//22
#else	
	255,//PERIPHERAL_ID_I2S0_TX,		//22
#endif	

#if defined (CFG_FUNC_I2S_MIX_MODE) && defined (CFG_RES_AUDIO_I2S1IN_EN)	
	4,//PERIPHERAL_ID_I2S1_RX,			//23
#else
	255,//PERIPHERAL_ID_I2S1_RX,		//23
#endif

#if	((defined(CFG_RES_AUDIO_I2SOUT_EN )&&(CFG_RES_I2S == 1))|| defined(CFG_RES_AUDIO_I2S1OUT_EN))
	5,	//PERIPHERAL_ID_I2S1_TX,		//24
#else
	255,//PERIPHERAL_ID_I2S1_TX,		//24
#endif

	255,//PERIPHERAL_ID_PPWM,			//25
	255,//PERIPHERAL_ID_ADC,     		//26
	255,//PERIPHERAL_ID_SOFTWARE,		//27
};

static  LineInPlayContext*		sLineInPlayCt;

void LineInPlayResFree(void)
{
	//注意：AudioCore父任务调整到mainApp下，此处只关闭AudioCore通道，不关闭任务
	AudioCoreProcessConfig((void*)AudioNoAppProcess);
	AudioCoreSourceDisable(LINEIN_SOURCE_NUM);

	AudioAnaChannelSet(ANA_INPUT_CH_NONE);

#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	AudioEffectsDeInit();
#endif

#if (LINEIN_INPUT_CHANNEL == ANA_INPUT_CH_LINEIN3)
	AudioADC_Disable(ADC1_MODULE);
	AudioADC_DeInit(ADC1_MODULE);
#else
	AudioADC_Disable(ADC0_MODULE);
	AudioADC_DeInit(ADC0_MODULE);
#endif

	if(sLineInPlayCt->ADCFIFO != NULL)
	{
		APP_DBG("ADCFIFO\n");
		osPortFree(sLineInPlayCt->ADCFIFO);
		sLineInPlayCt->ADCFIFO = NULL;
	}
	AudioCoreSourceDeinit(LINEIN_SOURCE_NUM);
	
	APP_DBG("Line:Kill Ct\n");
}

bool LineInPlayResMalloc(uint16_t SampleLen)
{
	AudioCoreIO	AudioIOSet;
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));

	AudioIOSet.Adapt = STD;
	AudioIOSet.Sync = TRUE;
	AudioIOSet.Channels = 2;
	AudioIOSet.Net = DefaultNet;
#if (LINEIN_INPUT_CHANNEL == ANA_INPUT_CH_LINEIN3)
	AudioIOSet.DataIOFunc = AudioADC1DataGet;
	AudioIOSet.LenGetFunc = AudioADC1DataLenGet;
#else
	AudioIOSet.DataIOFunc = AudioADC0DataGet;
	AudioIOSet.LenGetFunc = AudioADC0DataLenGet;
#endif
#ifdef	CFG_AUDIO_WIDTH_24BIT
	AudioIOSet.IOBitWidth = 0;//0,16bit,1:24bit
#ifdef BT_TWS_SUPPORT
	AudioIOSet.IOBitWidthConvFlag = 0;//tws不需要数据进行位宽扩展，会在TWS_SOURCE_NUM以后统一转成24bit
#else
	AudioIOSet.IOBitWidthConvFlag = 1;//需要数据进行位宽扩展
#endif
#endif
	//AudioIOSet.
	if(!AudioCoreSourceInit(&AudioIOSet, LINEIN_SOURCE_NUM))
	{
		DBG("Line source error!\n");
		return FALSE;
	}

	sLineInPlayCt = (LineInPlayContext*)osPortMalloc(sizeof(LineInPlayContext));
	if(sLineInPlayCt == NULL)
	{
		return FALSE;
	}
	memset(sLineInPlayCt, 0, sizeof(LineInPlayContext));

	//LineIn5  digital (DMA)
	sLineInPlayCt->ADCFIFO = (uint32_t*)osPortMallocFromEnd(SampleLen * 2 * 2 * 2);
	if(sLineInPlayCt->ADCFIFO == NULL)
	{
		return FALSE;
	}
	memset(sLineInPlayCt->ADCFIFO, 0, SampleLen * 2 * 2 * 2);

	return TRUE;
}

void LineInPlayResInit(void)
{
	sLineInPlayCt->SampleRate = CFG_PARA_SAMPLE_RATE;

	//Core Source1 para
	sLineInPlayCt->AudioCoreLineIn = (AudioCoreContext*)&AudioCore;
	//Audio init

	//Core Soure1.Para
	sLineInPlayCt->AudioCoreLineIn->AudioSource[LINEIN_SOURCE_NUM].Enable = 1;

	//Core Process
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	sLineInPlayCt->AudioCoreLineIn->AudioEffectProcess = (AudioCoreProcessFunc)AudioMusicProcess;
#else
	sLineInPlayCt->AudioCoreLineIn->AudioEffectProcess = (AudioCoreProcessFunc)AudioBypassProcess;
#endif

	AudioAnaChannelSet(LINEIN_INPUT_CHANNEL);

#if (LINEIN_INPUT_CHANNEL == ANA_INPUT_CH_LINEIN3)
	AudioADC_DigitalInit(ADC1_MODULE, sLineInPlayCt->SampleRate, (void*)sLineInPlayCt->ADCFIFO, AudioCoreFrameSizeGet(DefaultNet) * 2 * 2 * 2);
#else
	AudioADC_DigitalInit(ADC0_MODULE, sLineInPlayCt->SampleRate, (void*)sLineInPlayCt->ADCFIFO, AudioCoreFrameSizeGet(DefaultNet) * 2 * 2 * 2);
#endif
}

void LineInPlayRun(uint16_t msgId)
{
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	if(IsEffectChange == 1)
	{
		IsEffectChange = 0;
		AudioAPPDigitalGianProcess(mainAppCt.SysCurrentMode);
		AudioEffectsLoadInit(0, mainAppCt.EffectMode);
	}
	if(IsEffectChangeReload == 1)
	{
		IsEffectChangeReload = 0;
		AudioEffectsLoadInit(1, mainAppCt.EffectMode);
		AudioAPPDigitalGianProcess(mainAppCt.SysCurrentMode);
		AudioEffectsLoadInit(0, mainAppCt.EffectMode);
	}
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
	if(EffectParamFlashUpdataFlag == 1)
	{
		EffectParamFlashUpdataFlag = 0;
		EffectParamFlashUpdata();
	}
#endif
#endif

	switch(msgId)
	{
		case MSG_PLAY_PAUSE:
			HardWareMuteOrUnMute();
			break;
			
		default:
			CommonMsgProccess(msgId);
			break;
	}
}

bool LineInPlayInit(void)
{
	APP_DBG("LineIn Play Init\n");
	DMA_ChannelAllocTableSet((uint8_t *)sDmaChannelMap);//lineIn

#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	//音效参数遍历，确定系统帧长，待修改，sam, mark
#endif
	if(!ModeCommonInit())
	{
		return FALSE;
	}
	if(!LineInPlayResMalloc(AudioCoreFrameSizeGet(DefaultNet)))
	{
		APP_DBG("LineInPlay Res Error!\n");
		return FALSE;
	}
	LineInPlayResInit();

#ifdef CFG_FUNC_AUDIO_EFFECT_EN
#ifdef CFG_EFFECT_PARAM_IN_FLASH_EN
	//mainAppCt.EffectMode = EFFECT_MODE_FLASH_Music;
#else
	mainAppCt.EffectMode = EFFECT_MODE_NORMAL;
#endif
	AudioEffectsLoadInit(1, mainAppCt.EffectMode);
	AudioAPPDigitalGianProcess(mainAppCt.SysCurrentMode);
	AudioEffectsLoadInit(0, mainAppCt.EffectMode);
#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(RemindSoundServiceItemRequest(SOUND_REMIND_XIANLUMO, REMIND_ATTR_NEED_MUTE_APP_SOURCE) == FALSE)
	{
		if(IsAudioPlayerMute() == TRUE)
		{
			HardWareMuteOrUnMute();
		}
	}
#endif

#ifndef CFG_FUNC_REMIND_SOUND_EN
	if(IsAudioPlayerMute() == TRUE)
	{
		HardWareMuteOrUnMute();
	}
#endif

	return TRUE;
}


bool LineInPlayDeinit(void)
{
	APP_DBG("LineIn Play Deinit\n");
	if(sLineInPlayCt == NULL)
	{
		return TRUE;
	}

	if(IsAudioPlayerMute() == FALSE)
	{
		HardWareMuteOrUnMute();
	}	
	
	PauseAuidoCore();
	LineInPlayResFree();
	ModeCommonDeinit();//通路全部释放

	osPortFree(sLineInPlayCt);
	sLineInPlayCt = NULL;

	return TRUE;
}
#endif//#ifdef CFG_APP_LINEIN_MODE_EN
