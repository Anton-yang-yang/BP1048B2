/**
 **************************************************************************************
 * @file    hdmi_in_mode.c
 * @brief
 *
 * @author  Cecilia Wang
 * @version V1.0.0
 *
 * $Created: 2018-03-23 13:06:47$
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
#include "hdmi_in_api.h"
#include "timeout.h"
#include "main_task.h"
#include "audio_effect.h"
#include "mcu_circular_buf.h"
#include "resampler_polyphase.h"
//#include "decoder_service.h"
//#include "remind_sound_service.h"
#include "hdmi_in_Mode.h"
#include "powercontroller.h"
#include "deepsleep.h"
#include "backup_interface.h"
#include "breakpoint.h"
#include "otg_device_hcd.h"
#include "otg_device_standard_request.h"
#include "ctrlvars.h"
#include "recorder_service.h"
#include "audio_vol.h"
#include "clk.h"
#include "ctrlvars.h"
#include "reset.h"
#include "watchdog.h"

#include "cec.h"
#include "remind_sound_item.h"
#include "mode_task_api.h"
#include "remind_sound.h"
//#include "audio_adjust.h"
//#include "audio_common.h"

extern CECInitTypeDef 	*gCecInitDef;
extern HDMIInfo         *gHdmiCt;
#ifdef  CFG_APP_HDMIIN_MODE_EN

#define HDMI_IN_PLAY_TASK_STACK_SIZE		512//1024
#define HDMI_IN_PLAY_TASK_PRIO				4
#define HDMI_IN_NUM_MESSAGE_QUEUE			10

#define PCM_REMAIN_SMAPLES					3//spdif数据丢声道时,返回数据会超过一帧128，用于缓存数据，试听感更加好

//由于采样值从32bit转换为16bit，可以使用同一个buf，否则要独立申请
#define HDMI_ARC_CARRY_LEN					(MAX_FRAME_SAMPLES * 2 * 2 * 2 + PCM_REMAIN_SMAPLES * 2 * 4)// buf len for get data form dma fifo, deal
 //转采样输出buf,如果spdif转采样提升大于四倍需要加大此SPDIF_CARRY_LEN，比如输入8000以下转48000,需要缩小单次carry帧大小或调大HDMI_SRC_RESAMPLER_OUT_LEN、HDMI_ARC_PCM_FIFO_LEN
#define HDMI_SRC_RESAMPLER_OUT_LEN			(MAX_FRAME_SAMPLES * 2 * 2 * 4)


typedef struct _HdmiInPlayContext
{
	TaskState			state;

	uint32_t			*hdmiARCFIFO;	    //ARC的DMA循环fifo
	uint32_t            *sourceBuf_ARC;		//取ARC数据
	uint32_t            *hdmiARCCarry;
	uint32_t			*hdmiARCPcmFifo;
	MCU_CIRCULAR_CONTEXT hdmiPcmCircularBuf;

	AudioCoreContext 	*AudioCoreHdmiIn;
#if	defined(CFG_FUNC_REMIND_SOUND_EN)
	uint16_t*			Source2Decoder;
	TaskState			DecoderSync;
	bool				IsSoundRemindDone;
#endif

#ifdef CFG_FUNC_RECORDER_EN
	TaskState			RecorderSync;
#endif

	//play
	uint32_t 			SampleRate;

	ResamplerPolyphaseContext* ResamplerCt;
	uint32_t*			resampleOutBuf;

	uint32_t			hdmiSampleRate;

	uint32_t 			hdmiDmaWritePtr;
	uint32_t 			hdmiDmaReadPtr;
	uint32_t 			hdmiPreSample;
	uint8_t  			hdmiSampleRateCheckFlg;
	uint32_t 			hdmiSampleRateFromSW;

	uint8_t    			hdmiArcDone;
	uint8_t     		hdmiRetransCnt;
	TIMER       		hdmiMaxRespondTime;
	SPDIF_TYPE_STR  	AudioInfo;

#ifdef	CFG_FUNC_SOFT_ADJUST_IN
	int16_t 			pcmRemain[PCM_REMAIN_SMAPLES * 4];//用于缓存底层给上来超过128samples的数据
	int16_t 			pcmRemainLen;
#endif
}HdmiInPlayContext;


/**根据appconfig缺省配置:DMA 8个通道配置**/
/*1、cec需PERIPHERAL_ID_TIMER3*/
/*2、SD卡录音需PERIPHERAL_ID_SDIO RX/TX*/
/*3、SPDIF需PERIPHERAL_ID_SDPIF_RX*/
/*4、在线串口调音需PERIPHERAL_ID_UART1 RX/TX,建议使用USB HID，节省DMA资源*/
/*5、Mic开启需PERIPHERAL_ID_AUDIO_ADC1_RX，mode之间通道必须一致*/
/*6、Dac0开启需PERIPHERAL_ID_AUDIO_DAC0_TX mode之间通道必须一致*/
/*7、DacX需开启PERIPHERAL_ID_AUDIO_DAC1_TX mode之间通道必须一致*/
/*注意DMA 8个通道配置冲突:*/
/*a、UART在线调音和DAC-X有冲突,默认在线调音使用USB HID*/
/*b、UART在线调音与HDMI/SPDIF模式冲突*/
static const uint8_t DmaChannelMap[29] = {
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
	6,//PERIPHERAL_ID_SDPIF_RX,			//8 SPDIF_RX /TX same chanell
	6,//PERIPHERAL_ID_SDPIF_TX,			//8 SPDIF_RX /TX same chanell
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
	6,//PERIPHERAL_ID_I2S0_RX,		//21
#else
	255,//PERIPHERAL_ID_I2S0_RX,		//21
#endif

#if	((defined(CFG_RES_AUDIO_I2SOUT_EN )&&(CFG_RES_I2S == 0)) || defined(CFG_RES_AUDIO_I2S0OUT_EN))
	7,//PERIPHERAL_ID_I2S0_TX,		//22
#else	
	255,//PERIPHERAL_ID_I2S0_TX,		//22
#endif	

#if defined (CFG_FUNC_I2S_MIX_MODE) && defined (CFG_RES_AUDIO_I2S1IN_EN)	
	4,//PERIPHERAL_ID_I2S1_RX,		//23
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

typedef enum __HDMI_FAST_SWITCH_ARC_STATUS
{
	FAST_SWITCH_INIT_STATUS = 0,
	FAST_SWITCH_DONE_STATUS = 1,
	FAST_SWITCH_WORK_STATUS = 2,

} HDMI_FAST_SWITCH_ARC_STATUS;

static HdmiInPlayContext*		hdmiInPlayCt;

//osMutexId			hdmiPcmFifoMutex;
bool 				HdmiSpdifLockFlag = FALSE;
uint32_t 			hdmiCurSampleRate = 0;
bool 				unmuteLockFlag = 0;
TIMER 				unmuteLockTime;

static void HdmiARCScan(void);
static void HdmiInPlayRunning(uint16_t msgId);

bool HdmiInPlayResMalloc(uint16_t SampleLen)
{
	hdmiInPlayCt->hdmiARCFIFO = (uint32_t*)osPortMallocFromEnd(SampleLen * 2 * 2 * 2 * 2);
	if(hdmiInPlayCt->hdmiARCFIFO == NULL)
	{
		return FALSE;
	}
	memset(hdmiInPlayCt->hdmiARCFIFO, 0, SampleLen * 2 * 2 * 2 * 2);

	hdmiInPlayCt->sourceBuf_ARC = (uint32_t *)osPortMallocFromEnd(SampleLen * 2 * 2);
	if(hdmiInPlayCt->sourceBuf_ARC == NULL)
	{
		return FALSE;
	}
	memset(hdmiInPlayCt->sourceBuf_ARC, 0, SampleLen * 2 * 2);

	hdmiInPlayCt->hdmiARCPcmFifo = (uint32_t *)osPortMallocFromEnd(SampleLen * 2 * 2 * 2 * 2);
	if(hdmiInPlayCt->hdmiARCPcmFifo == NULL)
	{
		return FALSE;
	}
	MCUCircular_Config(&hdmiInPlayCt->hdmiPcmCircularBuf, hdmiInPlayCt->hdmiARCPcmFifo, SampleLen * 2 * 2 * 2 * 2);
	memset(hdmiInPlayCt->hdmiARCPcmFifo, 0, SampleLen * 2 * 2 * 2 * 2);

#if	defined(CFG_FUNC_REMIND_SOUND_EN)
	hdmiInPlayCt->Source2Decoder = (uint16_t*)osPortMalloc(SampleLen * 2 * 2);//One Frame
	if(hdmiInPlayCt->Source2Decoder == NULL)
	{
		return FALSE;
	}
	memset(hdmiInPlayCt->Source2Decoder, 0, SampleLen * 2 * 2);//2K
#endif
	return TRUE;
}


/**
 * @func        HdmiInPaly_Init
 * @brief       HdmiIn模式参数配置，资源初始化
 * @param       MessageHandle parentMsgHandle
 * @Output      None
 * @return      bool
 * @Others      任务块、Adc、Dac、AudioCore配置
 * @Others      数据流从Adc到audiocore配有函数指针，audioCore到Dac同理，由audiocoreService任务按需驱动
 * Record
 */
 bool HdmiInPlayInit(void)
{
	AudioCoreIO AudioIOSet;
	bool ret;
	//将SPDIF时钟切换到AUPLL
	Clock_SpdifClkSelect(APLL_CLK_MODE);

	DMA_ChannelAllocTableSet((uint8_t *)DmaChannelMap);//HdmiIn

	hdmiInPlayCt = (HdmiInPlayContext*)osPortMalloc(sizeof(HdmiInPlayContext));
	if(hdmiInPlayCt == NULL)
	{
		return FALSE;
	}
	memset(hdmiInPlayCt, 0, sizeof(HdmiInPlayContext));
	// Audio core config
	hdmiInPlayCt->SampleRate = CFG_PARA_SAMPLE_RATE;

	if(!HdmiInPlayResMalloc(AudioCoreFrameSizeGet(DefaultNet)))
	{
		APP_DBG("HdmiInPlayResMalloc error\n");
		return FALSE;
	}

	hdmiInPlayCt->hdmiARCCarry = (uint32_t *)osPortMalloc(HDMI_ARC_CARRY_LEN);
	if(hdmiInPlayCt->hdmiARCCarry == NULL)
	{
		return FALSE;
	}
	memset(hdmiInPlayCt->hdmiARCCarry, 0, HDMI_ARC_CARRY_LEN);

#if 0
	hdmiInPlayCt->ResamplerCt = (ResamplerPolyphaseContext*)osPortMalloc(sizeof(ResamplerPolyphaseContext));
	if(hdmiInPlayCt->ResamplerCt == NULL)
	{
		return FALSE;
	}
	hdmiInPlayCt->resampleOutBuf = (uint32_t *)osPortMalloc(HDMI_SRC_RESAMPLER_OUT_LEN);
	if(hdmiInPlayCt->resampleOutBuf == NULL)
	{
		return FALSE;
	}
#endif
	hdmiInPlayCt->hdmiDmaWritePtr 		 = 0;
	hdmiInPlayCt->hdmiDmaReadPtr  		 = 0;
	hdmiInPlayCt->hdmiSampleRateCheckFlg = 0;
	hdmiInPlayCt->hdmiSampleRateFromSW 	 = 0;
	hdmiInPlayCt->hdmiPreSample			 = 0;


	memset(&hdmiInPlayCt->AudioInfo, 0, sizeof(SPDIF_TYPE_STR));
	hdmiInPlayCt->AudioInfo.audio_type = SPDIF_AUDIO_PCM_DATA_TYPE;

	hdmiInPlayCt->AudioCoreHdmiIn = (AudioCoreContext *)&AudioCore;

	HDMI_ARC_Init((uint16_t *)hdmiInPlayCt->hdmiARCFIFO, AudioCoreFrameSizeGet(DefaultNet) * 2 * 2 * 2 * 2, &hdmiInPlayCt->hdmiPcmCircularBuf);
	HDMI_CEC_DDC_Init();

	//Audio init
//	//note Soure0.和sink0已经在main app中配置，不要随意配置

	if(!ModeCommonInit())
	{
		return FALSE;
	}
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	#ifdef CFG_FUNC_MIC_KARAOKE_EN
	hdmiInPlayCt->AudioCoreHdmiIn->AudioEffectProcess = (AudioCoreProcessFunc)AudioEffectProcess;
	#else
	hdmiInPlayCt->AudioCoreHdmiIn->AudioEffectProcess = (AudioCoreProcessFunc)AudioMusicProcess;
	#endif
#else
	hdmiInPlayCt->AudioCoreHdmiIn->AudioEffectProcess = (AudioCoreProcessFunc)AudioBypassProcess;
#endif
	//HDMI_CEC_InitiateARC();

	//Audio init

	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));

	AudioIOSet.Adapt = SRC_SRA;
	AudioIOSet.Sync = FALSE;
	AudioIOSet.Channels = 2;
	AudioIOSet.Net = DefaultNet;

	AudioIOSet.DataIOFunc = HDMI_ARC_DataGet;
	AudioIOSet.LenGetFunc = HDMI_ARC_DataLenGet;

	AudioIOSet.Depth = AudioCoreFrameSizeGet(DefaultNet) * 2;
	AudioIOSet.LowLevelCent = 40;
	AudioIOSet.HighLevelCent = 60;
	AudioIOSet.SampleRate = CFG_PARA_SAMPLE_RATE;
#ifdef	CFG_AUDIO_WIDTH_24BIT
	AudioIOSet.IOBitWidth = 0;//0,16bit,1:24bit
#ifdef BT_TWS_SUPPORT
	AudioIOSet.IOBitWidthConvFlag = 0;//tws不需要数据进行位宽扩展，会在TWS_SOURCE_NUM以后统一转成24bit
#else
	AudioIOSet.IOBitWidthConvFlag = 1;//需要数据进行位宽扩展
#endif
#endif

	if(!AudioCoreSourceInit(&AudioIOSet, HDMI_IN_SOURCE_NUM))
	{
		DBG("hdmi source init error!\n");
		return FALSE;
	}

	AudioCoreSourceAdjust(HDMI_IN_SOURCE_NUM, TRUE);

	DMA_ChannelDisable(PERIPHERAL_ID_SPDIF_RX);
	DMA_CircularConfig(PERIPHERAL_ID_SPDIF_RX, AudioCoreFrameSizeGet(DefaultNet) * 2 * 2 * 2 * 2, (uint16_t *)hdmiInPlayCt->hdmiARCFIFO, AudioCoreFrameSizeGet(DefaultNet) * 2 * 2 * 2 * 2);
    DMA_ChannelEnable(PERIPHERAL_ID_SPDIF_RX);
	SPDIF_ModuleEnable();

#ifdef CFG_FUNC_BREAKPOINT_EN
		BackupInfoUpdata(BACKUP_SYS_INFO);
#endif


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

	//AudioCoreSourceEnable(HDMI_IN_SOURCE_NUM);
#ifdef CFG_FUNC_REMIND_SOUND_EN

	DBG("HdmiIn Play run\n");
	ret = RemindSoundServiceItemRequest(SOUND_REMIND_HDMIMODE, REMIND_PRIO_NORMAL);
	if(ret == FALSE)
	{
		if(IsAudioPlayerMute() == TRUE)
		{
			HardWareMuteOrUnMute();
		}
	}
#endif

	HdmiSpdifLockFlag = FALSE;
	hdmiCurSampleRate = 0;
	unmuteLockFlag = 0;

#ifndef CFG_FUNC_REMIND_SOUND_EN
	 if(IsAudioPlayerMute() == TRUE)
	 {
		 HardWareMuteOrUnMute();
	 }
#endif

	return TRUE;
}

static void HdmiInSampleRateChange(void)
{
#if 1//def CFG_FUNC_MIXER_SRC_EN
	AudioCoreSourceChange(HDMI_IN_SOURCE_NUM, 2, hdmiInPlayCt->hdmiSampleRate);
#else
	if(hdmiInPlayCt->hdmiSampleRate != hdmiInPlayCt->SampleRate)
	{
		hdmiInPlayCt->SampleRate = hdmiInPlayCt->hdmiSampleRate;//注意此处调整会造成提示音和mic数据dac不正常。
		APP_DBG("Dac Sample:%d\n",(int)hdmiInPlayCt->SampleRate);
#ifdef CFG_RES_AUDIO_DACX_EN
		AudioDAC_SampleRateChange(DAC1, hdmiInPlayCt->SampleRate);
#endif
#ifdef CFG_RES_AUDIO_DAC0_EN
		AudioDAC_SampleRateChange(DAC0, hdmiInPlayCt->SampleRate);
#endif
	}
#endif
}

void HdmiInPlayRun(uint16_t msgId)
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
				
			default:
				//if(hdmiInPlayCt->state == TaskStateRunning)
				{
					HdmiInPlayRunning(msgId);
					if(HdmiSpdifLockFlag && !SPDIF_FlagStatusGet(LOCK_FLAG_STATUS))
					{
						APP_DBG("SPDIF RX UNLOCK!\n");
						HdmiSpdifLockFlag = FALSE;
						//HDMI_CEC_DDC_Init();
						AudioCoreSourceDisable(HDMI_IN_SOURCE_NUM);
//				#ifdef CFG_FUNC_FREQ_ADJUST
//						AudioCoreSourceFreqAdjustDisable();
//				#endif
					}

					if(!HdmiSpdifLockFlag && SPDIF_FlagStatusGet(LOCK_FLAG_STATUS)/* && HDMI_ARC_IsReady()*/
						// #if	defined(CFG_FUNC_REMIND_SOUND_EN)
						// && hdmiInPlayCt->IsSoundRemindDone
						// #endif
					)
					{
						if((TV_XIAOMI_XMD == gHdmiCt->hdmi_tv_inf.tv_type && HDMI_ARC_IsReady())
						|| (TV_XIAOMI_XMD != gHdmiCt->hdmi_tv_inf.tv_type))
						{
							APP_DBG("SPDIF RX LOCK!\n");
							HdmiSpdifLockFlag = TRUE;

						//AudioCoreSourceMute(APP_SOURCE_NUM, TRUE, TRUE);
						HDMISourceMute();
						vTaskDelay(20);
						AudioCoreSourceEnable(HDMI_IN_SOURCE_NUM);
						//AudioCoreSourceUnmute(APP_SOURCE_NUM, TRUE, TRUE);
						TimeOutSet(&unmuteLockTime, 500);
						unmuteLockFlag = 1;

						}
					}
					if(HdmiSpdifLockFlag == TRUE)
					{
						//硬件采样率获取，设定。
						if(hdmiCurSampleRate != SPDIF_SampleRateGet())
						{
							hdmiCurSampleRate = SPDIF_SampleRateGet();

							APP_DBG("Get samplerate: %d\n", (int)hdmiCurSampleRate);
							hdmiInPlayCt->hdmiSampleRate = hdmiCurSampleRate;
							HdmiInSampleRateChange();
						}

						if(unmuteLockFlag == 1)
						{
							if(IsTimeOut(&unmuteLockTime))
							{
								unmuteLockFlag = 0;
								if(gHdmiCt->hdmi_audiomute_flag == 0)
								{
									HDMISourceUnmute();
									if(IsAudioPlayerMute() == TRUE)
									{
										HardWareMuteOrUnMute();
									}
									if(gHdmiCt->hdmiActiveReportMuteAfterInit == 1)
									{
										gHdmiCt->hdmiActiveReportMuteAfterInit = 0;
										gHdmiCt->hdmiActiveReportMuteflag = 2;
										gHdmiCt->hdmiActiveReportMuteStatus = 0;
									}
								}

							}
						}

						HdmiARCScan();
					}
					HDMI_CEC_Scan(1);

					//任务优先级设置为4,通过发送该命令，可以提高AudioCore service有效利用率
					{
						MessageContext		msgSend;
						msgSend.msgId		= MSG_NONE;
						MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);
					}
				}
				break;
		}
}

static void HdmiInPlayRunning(uint16_t msgId)
{
	if(hdmiInPlayCt->hdmiArcDone == FAST_SWITCH_WORK_STATUS)
	{
		if(IsTimeOut(&hdmiInPlayCt->hdmiMaxRespondTime))
		{
			TimeOutSet(&hdmiInPlayCt->hdmiMaxRespondTime, 200);
			msgId = MSG_HDMI_AUDIO_ARC_ONOFF;
		}
	}
	switch(msgId)
	{
		case MSG_HDMI_AUDIO_ARC_ONOFF:
			if(hdmiInPlayCt->hdmiArcDone != FAST_SWITCH_WORK_STATUS)
			{
				gHdmiCt->hdmiGiveReportStatus = 0;
				hdmiInPlayCt->hdmiRetransCnt = 0;
			}

			hdmiInPlayCt->hdmiArcDone = FAST_SWITCH_WORK_STATUS;
			TimeOutSet(&hdmiInPlayCt->hdmiMaxRespondTime, 200);
			if(gHdmiCt->hdmiGiveReportStatus == 1)
			{
				mainAppCt.hdmiArcOnFlg = !mainAppCt.hdmiArcOnFlg;
				hdmiInPlayCt->hdmiArcDone = FAST_SWITCH_DONE_STATUS;
			}
			else
			{
				if(HDMI_CEC_IsWorking() == CEC_IS_IDLE)
				{
					HDMI_ARC_OnOff(!mainAppCt.hdmiArcOnFlg);
					hdmiInPlayCt->hdmiRetransCnt ++;
				}
				else if(HDMI_CEC_IsWorking() == CEC_IS_INACTIVE)
				{
					hdmiInPlayCt->hdmiRetransCnt = 0;
					mainAppCt.hdmiArcOnFlg = !mainAppCt.hdmiArcOnFlg;
					hdmiInPlayCt->hdmiArcDone = FAST_SWITCH_DONE_STATUS;
				}
			}

			if(hdmiInPlayCt->hdmiRetransCnt >= 2)
			{
				hdmiInPlayCt->hdmiRetransCnt = 0;
				mainAppCt.hdmiArcOnFlg = !mainAppCt.hdmiArcOnFlg;
				hdmiInPlayCt->hdmiArcDone = FAST_SWITCH_DONE_STATUS;
			}
			break;

		default:
			CommonMsgProccess(msgId);
			break;
	}
}

static void HdmiARCScan(void)
{
	int32_t pcm_space;
	uint16_t spdif_len;
	int16_t pcm_len;
	int16_t *pcmBuf  = (int16_t *)hdmiInPlayCt->hdmiARCCarry;
	uint16_t cnt;

	spdif_len = DMA_CircularDataLenGet(PERIPHERAL_ID_SPDIF_RX);
	pcm_space = MCUCircular_GetSpaceLen(&hdmiInPlayCt->hdmiPcmCircularBuf) - 16;

#if 1//def CFG_FUNC_MIXER_SRC_EN
	pcm_space = (pcm_space * hdmiInPlayCt->hdmiSampleRate) / CFG_PARA_SAMPLE_RATE - 16;
#endif

	if(pcm_space < 16)
	{
		DBG("hdmi pcm_space err\n");
		return;
	}
	if((spdif_len >> 1) > pcm_space)
	{
		spdif_len = pcm_space * 2;
	}

	spdif_len = spdif_len /(MAX_FRAME_SAMPLES * 2 * 4) * (MAX_FRAME_SAMPLES * 2 * 4);
	spdif_len = spdif_len & 0xFFF8;
	if(!spdif_len)
	{
		return ;
	}

	cnt = (spdif_len / 8) / MAX_FRAME_SAMPLES;
	while(cnt--)
	{
		DMA_CircularDataGet(PERIPHERAL_ID_SPDIF_RX, pcmBuf, MAX_FRAME_SAMPLES * 8);
		//由于从32bit转换为16bit，buf可以使用同一个，否则要独立申请。
		SPDIF_SPDIFDatatoAudioData((int32_t *)pcmBuf, MAX_FRAME_SAMPLES * 8, (int32_t *)pcmBuf, SPDIF_WORDLTH_16BIT, &hdmiInPlayCt->AudioInfo);
        pcm_len = hdmiInPlayCt->AudioInfo.output_length;

		if(hdmiInPlayCt->AudioInfo.audio_type != SPDIF_AUDIO_PCM_DATA_TYPE)
		{
			return;
		}
		if(!mainAppCt.hdmiArcOnFlg)
		{
			memset(pcmBuf, 0, pcm_len);
		}
		MCUCircular_PutData(&hdmiInPlayCt->hdmiPcmCircularBuf, pcmBuf, pcm_len);//注意格式转换返回值是byte
	 }
}

bool HdmiInPlayDeinit(void)
{
	APP_DBG("HdmiIn Play Deinit\n");
	if(hdmiInPlayCt == NULL)
	{
		return FALSE;
	}

	if(IsAudioPlayerMute() == FALSE)
	{
		HardWareMuteOrUnMute();
	}

	PauseAuidoCore();
	AudioCoreProcessConfig((void*)AudioNoAppProcess);
	AudioCoreSourceDisable(HDMI_IN_SOURCE_NUM);
	AudioCoreSourceDeinit(HDMI_IN_SOURCE_NUM);

#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	AudioEffectsDeInit();
#endif

	HDMI_CEC_DDC_DeInit();
	HDMI_ARC_DeInit();

	hdmiInPlayCt->AudioCoreHdmiIn = NULL;
	if(hdmiInPlayCt->hdmiARCFIFO != NULL)
	{
		osPortFree(hdmiInPlayCt->hdmiARCFIFO);
		hdmiInPlayCt->hdmiARCFIFO = NULL;
	}

	if(hdmiInPlayCt->sourceBuf_ARC != NULL)
	{
		osPortFree(hdmiInPlayCt->sourceBuf_ARC);
		hdmiInPlayCt->sourceBuf_ARC = NULL;
	}

	if(hdmiInPlayCt->hdmiARCCarry != NULL)
	{
		osPortFree(hdmiInPlayCt->hdmiARCCarry);
		hdmiInPlayCt->hdmiARCCarry = NULL;
	}

	if(hdmiInPlayCt->hdmiARCPcmFifo != NULL)
	{
		osPortFree(hdmiInPlayCt->hdmiARCPcmFifo);
		hdmiInPlayCt->hdmiARCPcmFifo = NULL;
	}


#if	defined(CFG_FUNC_REMIND_SOUND_EN)
	if(hdmiInPlayCt->Source2Decoder != NULL)
	{
		osPortFree(hdmiInPlayCt->Source2Decoder);
		hdmiInPlayCt->Source2Decoder = NULL;
	}
#endif

#if 0
	if(hdmiInPlayCt->ResamplerCt != NULL)
	{
		osPortFree(hdmiInPlayCt->ResamplerCt);
		hdmiInPlayCt->ResamplerCt = NULL;
	}
	if(hdmiInPlayCt->resampleOutBuf != NULL)
	{
		osPortFree(hdmiInPlayCt->resampleOutBuf);
		hdmiInPlayCt->resampleOutBuf = NULL;
	}
#endif

	ModeCommonDeinit();//通路全部释放

	osPortFree(hdmiInPlayCt);
	hdmiInPlayCt = NULL;

	return TRUE;
}

#endif  //CFG_APP_HDMIIN_MODE_EN
