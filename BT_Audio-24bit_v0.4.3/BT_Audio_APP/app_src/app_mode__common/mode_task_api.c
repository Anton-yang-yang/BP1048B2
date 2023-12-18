/*
 * mode_task_api.c
 *
 *  Created on: Mar 30, 2021
 *      Author: piwang
 */
#include "type.h"
#include "dma.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "app_config.h"
#include "mode_task_api.h"
#include "main_task.h"
#include "debug.h"
#include "ctrlvars.h"
#include "breakpoint.h"
#include "remind_sound.h"
#include "i2s_interface.h"
//service
#include "bt_manager.h"
#include "audio_core_api.h"
#include "bt_app_tws.h"
#include "bt_tws_api.h"
#include "tws_mode.h"
#include "audio_vol.h"
#include "audio_effect.h"
#include "remind_sound.h"
#include "watchdog.h"
#include "audio_core_service.h"
#include "mode_task_api.h"
#include "bt_stack_service.h"
#include "hdmi_in_api.h"
#include "display_task.h"
//app
#if (BT_AVRCP_VOLUME_SYNC == ENABLE)
#include "bt_app_avrcp_deal.h"
#endif


void AudioI2sOutParamsSet(void);

#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
extern int32_t RemindMp3DecoderInit(void);
extern int32_t RemindMp3DecoderDeinit(void);
#endif

#ifdef CFG_APP_USB_PLAY_MODE_EN
void WaitUdiskUnlock(void)
{
	if(GetSysModeState(ModeUDiskAudioPlay) == ModeStateDeinit)
	{
		while(osMutexLock_1000ms(UDiskMutex) != TRUE)WDG_Feed();
	}
}
#endif

#ifdef CFG_APP_CARD_PLAY_MODE_EN
void SDCardForceExitFuc(void)
{
	if(GetSysModeState(ModeCardAudioPlay) == ModeStateDeinit)
	{
		SDCard_Force_Exit();
	}
}
#endif

void PauseAuidoCore(void)
{
	while(GetAudioCoreServiceState() != TaskStatePaused)
	{
		AudioCoreServicePause();
		osTaskDelay(2);
	}
}

#ifdef BT_TWS_SUPPORT
uint16_t AudioDAC0DataSetNull(void* Buf, uint16_t Len)
{
     Buf = Buf;
     Len = Len;
     return 0;
}

uint16_t AudioDAC0DataSpaceLenGetNull(void)
{
	return 512*8;
}

void tws_slave_fifo_clear(void)
{
// 待完善
//	memset(tws_audio_core_buf,0,sizeof(tws_audio_core_buf));
}
#endif

//配置系统标准通路
bool ModeCommonInit(void)
{
	AudioCoreIO AudioIOSet;
	uint16_t SampleLen = AudioCoreFrameSizeGet(DefaultNet);
	uint16_t FifoLenStereo = SampleLen * sizeof(PCM_DATA_TYPE) * 2 * 2;//立体声8倍大小于帧长，单位byte
//	uint16_t FifoLenMono = SampleLen * sizeof(PCM_DATA_TYPE) * 2;//单声到4倍大小于帧长，单位byte

#if defined(TWS_DAC0_OUT) && defined(TWS_DACX_OUT)
	extern void tws_device_open_isr(void);
	extern void tws_device_close(void);
	bool		tws_device_init_flag = FALSE;
#endif

#ifdef TWS_CODE_BACKUP//BT_TWS_SUPPORT
	SoftFlagRegister(SoftFlagAudioCoreSourceIsDeInit);
#endif

#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	EffectPcmBufMalloc(SampleLen);
	EffectPcmBufClear(SampleLen);
#endif

	//////////申请DMA fifo
#ifdef CFG_RES_AUDIO_DAC0_EN
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	#ifdef TWS_DAC0_OUT
		AudioIOSet.Resident = TRUE;
		AudioIOSet.Depth = TWS_SINK_DEV_FIFO_SAMPLES;
	#else
		AudioIOSet.Depth = AudioCore.FrameSize[0] * 2 ;
	#endif
	if(!AudioCoreSinkIsInit(AUDIO_DAC0_SINK_NUM))
	{
	#ifndef TWS_DAC0_OUT
		mainAppCt.DACFIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
		mainAppCt.DACFIFO = (uint32_t*)osPortMallocFromEnd(mainAppCt.DACFIFO_LEN);//DAC fifo
	#endif
		if(mainAppCt.DACFIFO != NULL)
		{
			memset(mainAppCt.DACFIFO, 0, mainAppCt.DACFIFO_LEN);
		}
		else
		{
			APP_DBG("malloc DACFIFO error\n");
			return FALSE;
		}

		//sink0
		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = TRUE;
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = DefaultNet;
	#ifdef TWS_DAC0_OUT
		AudioIOSet.DataIOFunc = AudioDAC0DataSet_tws;
	#else
		AudioIOSet.DataIOFunc = AudioDAC0DataSet;
	#endif
		AudioIOSet.LenGetFunc = AudioDAC0DataSpaceLenGet;
	#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 0;//不需要做位宽转换处理
	#endif
		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_DAC0_SINK_NUM))
		{
			DBG("Dac init error");
			return FALSE;
		}
		AudioDAC_Init(DAC0, mainAppCt.SampleRate, (void*)mainAppCt.DACFIFO, mainAppCt.DACFIFO_LEN, NULL, 0);
#if defined(TWS_DAC0_OUT) && defined(TWS_DACX_OUT)
		tws_device_init_flag = TRUE;
#endif
	}
	else//sam add,20230221
	{
		AudioDAC_SampleRateChange(DAC0, CFG_PARA_SAMPLE_RATE);
	#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 0;//DAC 24bit ,sink最后一级输出时需要转变为24bi
		AudioCore.AudioSink[AUDIO_DAC0_SINK_NUM].BitWidth = AudioIOSet.IOBitWidth;
		AudioCore.AudioSink[AUDIO_DAC0_SINK_NUM].BitWidthConvFlag = AudioIOSet.IOBitWidthConvFlag;
	#endif
	}
	AudioCoreSinkEnable(AUDIO_DAC0_SINK_NUM);
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	#ifdef TWS_DACX_OUT
		AudioIOSet.Resident = TRUE;
		AudioIOSet.Depth = TWS_SINK_DEV_FIFO_SAMPLES;
	#else
		AudioIOSet.Depth = AudioCore.FrameSize[0] * 2 ;
	#endif
	if(!AudioCoreSinkIsInit(AUDIO_DACX_SINK_NUM))
	{
	#ifndef TWS_DACX_OUT
		mainAppCt.DACXFIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE);
		mainAppCt.DACXFIFO = (uint32_t*)osPortMallocFromEnd(mainAppCt.DACXFIFO_LEN);//DACX fifo
	#endif
		if(mainAppCt.DACXFIFO != NULL)
		{
			memset(mainAppCt.DACXFIFO, 0, mainAppCt.DACXFIFO_LEN);
		}
		else
		{
			APP_DBG("malloc DACXFIFO error\n");
			return FALSE;
		}

		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = TRUE;
		AudioIOSet.Channels = 1;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.DataIOFunc = AudioDAC1DataSet;
		AudioIOSet.LenGetFunc = AudioDAC1DataSpaceLenGet;
	#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 0;//不需要做位宽转换处理
	#endif
		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_DACX_SINK_NUM))
		{
			DBG("Dacx init error");
			return FALSE;
		}
		AudioDAC_Init(DAC1, mainAppCt.SampleRate, NULL, 0, (void*)mainAppCt.DACXFIFO, mainAppCt.DACXFIFO_LEN);
#if defined(TWS_DAC0_OUT) && defined(TWS_DACX_OUT)
		tws_device_init_flag = TRUE;
#endif
	}
	else//sam add,20230221
	{
		AudioDAC_SampleRateChange(DAC1, CFG_PARA_SAMPLE_RATE);
	#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 0;//DAC 24bit ,sink最后一级输出时需要转变为24bi
		AudioCore.AudioSink[AUDIO_DACX_SINK_NUM].BitWidth = AudioIOSet.IOBitWidth;
		AudioCore.AudioSink[AUDIO_DACX_SINK_NUM].BitWidthConvFlag = AudioIOSet.IOBitWidthConvFlag;
	#endif
	}
	AudioCoreSinkEnable(AUDIO_DACX_SINK_NUM);
#endif

#if defined(TWS_DAC0_OUT) && defined(TWS_DACX_OUT)
	if(tws_device_init_flag)
	{
		tws_device_close();
		tws_device_open_isr();
	}
	tws_device_init_flag = FALSE;
#endif
	//Mic1 analog  = Soure0.
	AudioADC_AnaInit();
	//AudioADC_VcomConfig(1);//MicBias en
	AudioADC_MicBias1Enable(1);
#if CFG_RES_MIC_SELECT
	if(!AudioCoreSourceIsInit(MIC_SOURCE_NUM))
	{
		mainAppCt.ADCFIFO = (uint32_t*)osPortMallocFromEnd(FifoLenStereo);//ADC fifo
		if(mainAppCt.ADCFIFO != NULL)
		{
			memset(mainAppCt.ADCFIFO, 0, FifoLenStereo);
		}
		else
		{
			APP_DBG("malloc ADCFIFO error\n");
			return FALSE;
		}

		AudioADC_DynamicElementMatch(ADC1_MODULE, TRUE, TRUE);
		AudioADC_PGASel(ADC1_MODULE, CHANNEL_LEFT, LINEIN3_LEFT_OR_MIC1);
		AudioADC_PGASel(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2);
		AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_LEFT, LINEIN3_LEFT_OR_MIC1, 15, 4);//0db bypass
		AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2, 15, 4);

		//Mic1   digital
	#ifdef CFG_AUDIO_WIDTH_24BIT
		AudioADC_DigitalInit(ADC1_MODULE, mainAppCt.SampleRate, (void*)mainAppCt.ADCFIFO, AudioCoreFrameSizeGet(DefaultNet) * 2 * 2 * 2 * 2);
	#else
		AudioADC_DigitalInit(ADC1_MODULE, mainAppCt.SampleRate, (void*)mainAppCt.ADCFIFO, AudioCoreFrameSizeGet(DefaultNet) * 2 * 2 * 2);
	#endif

		//Soure0.
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = TRUE;
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.DataIOFunc = AudioADC1DataGet;
		AudioIOSet.LenGetFunc = AudioADC1DataLenGet;
	#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 0;//需要数据进行位宽扩展
	#endif
		if(!AudioCoreSourceInit(&AudioIOSet, MIC_SOURCE_NUM))
		{
			DBG("mic Source error");
			return FALSE;
		}
	}

	AudioCoreSourceEnable(MIC_SOURCE_NUM);
#endif

#ifdef BT_TWS_SUPPORT
	if(!AudioCoreSourceIsInit(TWS_SOURCE_NUM))
	{
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = FALSE;//TRUE
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.DataIOFunc = TWSDataGet;
		AudioIOSet.LenGetFunc = TWSDataLenGet;
	#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 1;//需要数据进行位宽扩展
	#endif
		if(!AudioCoreSourceInit(&AudioIOSet, TWS_SOURCE_NUM))
		{
			DBG("Tws Source error");
			return FALSE;
		}
	}
	AudioCoreSourceEnable(TWS_SOURCE_NUM);

	if(GetSystemMode() != ModeTwsSlavePlay && !AudioCoreSinkIsInit(TWS_SINK_NUM))
	{
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = FALSE;//TRUE
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.Depth = FifoLenStereo;
		AudioIOSet.DataIOFunc = TwsSinkDataSet;
		AudioIOSet.LenGetFunc = TwsSinkSpaceLenGet;
		AudioIOSet.Resident = TRUE;
	#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 0;//不需要做位宽转换处理
	#endif
		if(!AudioCoreSinkInit(&AudioIOSet, TWS_SINK_NUM))
		{
			DBG("Tws Sink error");
			return FALSE;
		}
		else
			DBG("TWS_SINK_Init!\n");
		AudioCoreSinkEnable(TWS_SINK_NUM);
	}
#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(!AudioCoreSourceIsInit(REMIND_SOURCE_NUM))
	{
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.Adapt = STD;//SRC_ONLY;
		AudioIOSet.SampleRate = CFG_PARA_SAMPLE_RATE;//初始值
		AudioIOSet.Sync = FALSE;
		AudioIOSet.Channels = 1;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.DataIOFunc = RemindDataGet;
		AudioIOSet.LenGetFunc = RemindDataLenGet;

	#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 1;//需要数据进行位宽扩展
	#endif
		if(!AudioCoreSourceInit(&AudioIOSet, REMIND_SOURCE_NUM))
		{
			DBG("remind source error!\n");
			SoftFlagRegister(SoftFlagNoRemind);
			return FALSE;
		}
	#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
		RemindMp3DecoderInit();
	#endif
	}
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	#if defined(TWS_IIS0_OUT) || defined(TWS_IIS1_OUT)
		AudioIOSet.Resident = TRUE;
		AudioIOSet.Depth = TWS_SINK_DEV_FIFO_SAMPLES;
	#else
		AudioIOSet.Depth = AudioCore.FrameSize[0] * 2 ;
	#endif
	if(!AudioCoreSinkIsInit(AUDIO_I2SOUT_SINK_NUM))
	{
#if !defined(TWS_IIS0_OUT) && !defined(TWS_IIS1_OUT)
		mainAppCt.I2SFIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
		mainAppCt.I2SFIFO = (uint32_t*)osPortMallocFromEnd(mainAppCt.I2SFIFO_LEN);//I2S fifo
#endif
		if(mainAppCt.I2SFIFO != NULL)
		{
			memset(mainAppCt.I2SFIFO, 0, mainAppCt.I2SFIFO_LEN);
		}
		else
		{
			APP_DBG("malloc I2SFIFO error\n");
			return FALSE;
		}

#if CFG_RES_I2S_MODE == 0 || !defined(CFG_FUNC_I2S_OUT_SYNC_EN)// Master 或不开微调
#if CFG_PARA_I2S_SAMPLERATE == CFG_PARA_SAMPLE_RATE
		AudioIOSet.Adapt = STD;//SRC_ONLY
#else
		AudioIOSet.Adapt = SRC_ONLY;
#endif
#else//slave
#if CFG_PARA_I2S_SAMPLERATE == CFG_PARA_SAMPLE_RATE
		AudioIOSet.Adapt = STD;//SRA_ONLY;//CLK_ADJUST_ONLY;
#else
		AudioIOSet.Adapt = SRC_ONLY;//SRC_SRA;//SRC_ADJUST;
#endif
#endif
		AudioIOSet.Sync = TRUE;//I2S slave 时候如果master没有接，有可能会导致DAC也不出声音。
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.HighLevelCent = 60;
		AudioIOSet.LowLevelCent = 40;
		AudioIOSet.SampleRate = CFG_PARA_I2S_SAMPLERATE;//根据实际外设选择
#if (CFG_RES_I2S == 0)
		AudioIOSet.DataIOFunc = AudioI2S0_DataSet;
		AudioIOSet.LenGetFunc = AudioI2S0_DataSpaceLenGet;
#else
		AudioIOSet.DataIOFunc = AudioI2S1_DataSet;
		AudioIOSet.LenGetFunc = AudioI2S1_DataSpaceLenGet;
#endif
#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 0;//不需要做位宽转换处理
#endif
		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_I2SOUT_SINK_NUM))
		{
			DBG("I2S out init error");
			return FALSE;
		}

		AudioI2sOutParamsSet();
		AudioCoreSinkEnable(AUDIO_I2SOUT_SINK_NUM);
		AudioCoreSinkAdjust(AUDIO_I2SOUT_SINK_NUM, TRUE);
	}
	else//sam add,20230221
	{
		I2S_SampleRateSet(CFG_RES_I2S_MODULE, CFG_PARA_SAMPLE_RATE);
	#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 0;//DAC 24bit ,sink最后一级输出时需要转变为24bi
		AudioCore.AudioSink[AUDIO_I2SOUT_SINK_NUM].BitWidth = AudioIOSet.IOBitWidth;
		AudioCore.AudioSink[AUDIO_I2SOUT_SINK_NUM].BitWidthConvFlag = AudioIOSet.IOBitWidthConvFlag;
		AudioCore.AudioSink[AUDIO_I2SOUT_SINK_NUM].Sync = TRUE;		
	#endif
	}
#endif

#ifdef CFG_FUNC_BREAKPOINT_EN
	//注意 是否需要模式过滤，部分模式无需记忆和恢复
	if(GetSystemMode() != ModeIdle)
	{
		BackupInfoUpdata(BACKUP_SYS_INFO);
	}
#endif
#ifdef TWS_CODE_BACKUP//BT_TWS_SUPPORT
	if(!IsIdleModeReady() && GetBtManager()->twsState == BT_TWS_STATE_CONNECTED && GetBtManager()->twsRole == BT_TWS_MASTER)
	{
		g_tws_need_init = 1;
	}
#endif	

#ifdef CFG_FUNC_I2S_MIX_MODE
	I2sMixModeInit();
#endif

	return TRUE;
}

//释放公共通路，
void ModeCommonDeinit(void)
{
#ifdef TWS_CODE_BACKUP//BT_TWS_SUPPORT
	//模式切换时,清除TWS同步的标志位
	g_tws_need_init = 0;
#endif

	SoftFlagRegister(SoftFlagAudioCoreSourceIsDeInit);
#ifdef	CFG_FUNC_AUDIO_EFFECT_EN
	EffectPcmBufRelease();
#endif
#if defined(CFG_RES_AUDIO_DAC0_EN) && !defined(TWS_DAC0_OUT)
//	AudioCoreSinkDisable(AUDIO_DAC0_SINK_NUM);
	AudioDAC_Disable(DAC0);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_DAC0_TX);
//	AudioDAC_Reset(DAC0);
	if(mainAppCt.DACFIFO != NULL)
	{
		osPortFree(mainAppCt.DACFIFO);
		mainAppCt.DACFIFO = NULL;
	}
	AudioCoreSinkDeinit(AUDIO_DAC0_SINK_NUM);
#endif

#if defined(CFG_RES_AUDIO_DACX_EN) && !defined(TWS_DACX_OUT)
	AudioCoreSinkDisable(AUDIO_DACX_SINK_NUM);
	AudioDAC_Disable(DAC1);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC1_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC1_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC1_TX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_DAC1_TX);
//	AudioDAC_Reset(DAC1);
	if(mainAppCt.DACXFIFO != NULL)
	{
		osPortFree(mainAppCt.DACXFIFO);
		mainAppCt.DACXFIFO = NULL;
	}
	AudioCoreSinkDeinit(AUDIO_DACX_SINK_NUM);
#endif

#if CFG_RES_MIC_SELECT
	AudioCoreSourceDisable(MIC_SOURCE_NUM);
	vTaskDelay(5);
	AudioADC_Disable(ADC1_MODULE);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_ADC1_RX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_ADC1_RX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_ADC1_RX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_ADC1_RX);
	if(mainAppCt.ADCFIFO != NULL)
	{
		APP_DBG("ADC1FIFO\n");
		osPortFree(mainAppCt.ADCFIFO);
		mainAppCt.ADCFIFO = NULL;
	}
	AudioCoreSourceDeinit(MIC_SOURCE_NUM);
#endif
#if defined(CFG_RES_AUDIO_I2SOUT_EN) && !defined(TWS_IIS0_OUT)&& !defined(TWS_IIS1_OUT)
	I2S_ModuleDisable(CFG_RES_I2S_MODULE);
#if CFG_RES_I2S == 0
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_I2S0_TX);
#else
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S1_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S1_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S1_TX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_I2S1_TX);
#endif
	if(mainAppCt.I2SFIFO != NULL)
	{
		APP_DBG("I2SFIFO\n");
		osPortFree(mainAppCt.I2SFIFO);
		mainAppCt.I2SFIFO = NULL;
	}
	AudioCoreSinkDeinit(AUDIO_I2SOUT_SINK_NUM);
#endif
#ifdef BT_TWS_SUPPORT
	if(AudioCoreSourceIsInit(TWS_SOURCE_NUM))
	{
		AudioCoreSourceDisable(TWS_SOURCE_NUM);
		AudioCoreSourceDeinit(TWS_SOURCE_NUM);
	}

	DBG("Deinit TWS Source\n");
#endif

//#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
	AudioCoreSourceDisable(REMIND_SOURCE_NUM);
	AudioCoreSourceDeinit(REMIND_SOURCE_NUM);
	RemindSoundAudioPlayEnd();
#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
	RemindMp3DecoderDeinit();
#endif
#endif

#ifdef CFG_FUNC_I2S_MIX_MODE
	I2sMixModeDeInit();
#endif

//#ifdef CFG_FUNC_AUDIO_EFFECT_EN
//	EffectPcmBufRelease();//audiocore 没暂停，不能释放
//#endif
}

#ifdef CFG_RES_AUDIO_I2SOUT_EN
void AudioI2sOutParamsSet(void)
{
	I2SParamCt i2s_set;
	i2s_set.IsMasterMode = CFG_RES_I2S_MODE;// 0:master 1:slave
	i2s_set.SampleRate = CFG_PARA_I2S_SAMPLERATE; //外设采样率
	i2s_set.I2sFormat = I2S_FORMAT_I2S;
#ifdef	CFG_AUDIO_WIDTH_24BIT
	i2s_set.I2sBits = I2S_LENGTH_24BITS;
#else
	i2s_set.I2sBits = I2S_LENGTH_16BITS;
#endif
	i2s_set.I2sTxRxEnable = 1;
#if (CFG_RES_I2S == 0)
	i2s_set.TxPeripheralID = PERIPHERAL_ID_I2S0_TX;
#else
	i2s_set.TxPeripheralID = PERIPHERAL_ID_I2S1_TX;
#endif
	i2s_set.TxBuf = (void*)mainAppCt.I2SFIFO;

	i2s_set.TxLen = mainAppCt.I2SFIFO_LEN;

#if CFG_RES_I2S == 0
#if CFG_RES_I2S_IO_PORT == 0			//I2S0_MODULE Port0
#if CFG_RES_I2S_MODE == 0
	GPIO_PortAModeSet(GPIOA0, 9);// mclk 3:in;9:out
#else
	GPIO_PortAModeSet(GPIOA0, 3);
#endif
	GPIO_PortAModeSet(GPIOA1, 6);// lrclk
	GPIO_PortAModeSet(GPIOA2, 5);// bclk
	GPIO_PortAModeSet(GPIOA3, 7);// out
//	GPIO_PortAModeSet(GPIOA4, 1);// din

#else									//I2S0_MODULE Port1
#if CFG_RES_I2S_MODE == 0
	GPIO_PortAModeSet(GPIOA24, 9);//mclk 3:in;9:out
#else
	GPIO_PortAModeSet(GPIOA24, 3);
#endif
	GPIO_PortAModeSet(GPIOA20, 6);//lrclk
	GPIO_PortAModeSet(GPIOA21, 5);//bclk
	GPIO_PortAModeSet(GPIOA22, 10);//do
//	GPIO_PortAModeSet(GPIOA23, 3);//di
#endif

#else//CFG_RES_I2S == 1
#if CFG_RES_I2S_IO_PORT == 0			//I2S0_MODULE Port0
#if CFG_RES_I2S_MODE == 0
	GPIO_PortAModeSet(GPIOA27, 6);//mclk 1:in;6:out
#else
	GPIO_PortAModeSet(GPIOA27, 1);//mclk 1:in;6:out
#endif
	GPIO_PortAModeSet(GPIOA28, 1);//lrclk
	GPIO_PortAModeSet(GPIOA29, 1);//bclk
	GPIO_PortAModeSet(GPIOA30, 6);//do
//	GPIO_PortAModeSet(GPIOA31, 2);//di

#elif CFG_RES_I2S_IO_PORT == 1			//I2S0_MODULE Port1
#if CFG_RES_I2S_MODE == 0
	GPIO_PortAModeSet(GPIOA7, 5);//mclk 3:in;5:out
#else
	GPIO_PortAModeSet(GPIOA7, 3);//mclk 3:in;5:out
#endif
	GPIO_PortAModeSet(GPIOA8, 1);//lrclk
	GPIO_PortAModeSet(GPIOA9, 2);//bclk
	GPIO_PortAModeSet(GPIOA10, 4);//do
//	GPIO_PortAModeSet(GPIOA11, 2);//di

#elif CFG_RES_I2S_IO_PORT == 2			//I2S1_MODULE Port2
	GPIO_PortAModeSet(GPIOA1, 7);//lrclk
	GPIO_PortAModeSet(GPIOA2, 6);//bclk
	GPIO_PortAModeSet(GPIOA31, 5);//do
//	GPIO_PortAModeSet(GPIOA30, 2);//di

#else									//I2S1_MODULE Port3
	GPIO_PortAModeSet(GPIOA20, 7);//lrclk
	GPIO_PortAModeSet(GPIOA21, 6);//bclk
	GPIO_PortAModeSet(GPIOA11, 4);//do
//	GPIO_PortAModeSet(GPIOA10, 1);//di
#endif
#endif//CFG_RES_I2S == 0
	I2S_AlignModeSet(CFG_RES_I2S_MODULE, I2S_LOW_BITS_ACTIVE);
	AudioI2S_Init(CFG_RES_I2S_MODULE, &i2s_set);//
}
#endif



bool AudioIoCommonForHfp(uint32_t sampleRate, uint16_t gain, uint8_t gainBoostSel)
{
	AudioCoreIO AudioIOSet;
	uint16_t SampleLen = AudioCoreFrameSizeGet(DefaultNet);
	uint16_t FifoLenStereo = SampleLen * 2 * 2 * 2;//立体声8倍大小于帧长，单位byte
//	uint16_t FifoLenMono = SampleLen * 2 * 2;//单声到4倍大小于帧长，单位byte

#ifdef BT_TWS_SUPPORT
	SoftFlagRegister(SoftFlagAudioCoreSourceIsDeInit);
	AudioCoreSourceDisable(TWS_SOURCE_NUM);
	AudioCoreSourceDeinit(TWS_SOURCE_NUM);
#endif

#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	EffectPcmBufMalloc(SampleLen);
	EffectPcmBufClear(SampleLen);
#endif

#if CFG_RES_MIC_SELECT
	AudioCoreSourceDisable(MIC_SOURCE_NUM);

	if(!AudioCoreSourceIsInit(MIC_SOURCE_NUM))
	{
		//Mic1 analog  = Soure0.
		AudioADC_AnaInit();
		//AudioADC_VcomConfig(1);//MicBias en
		AudioADC_MicBias1Enable(1);
		mainAppCt.ADCFIFO = (uint32_t*)osPortMallocFromEnd(FifoLenStereo);//ADC fifo
		if(mainAppCt.ADCFIFO != NULL)
		{
			memset(mainAppCt.ADCFIFO, 0, FifoLenStereo);
		}
		else
		{
			APP_DBG("malloc ADCFIFO error\n");
			return FALSE;
		}

		AudioADC_DynamicElementMatch(ADC1_MODULE, TRUE, TRUE);
		AudioADC_PGASel(ADC1_MODULE, CHANNEL_LEFT, LINEIN3_LEFT_OR_MIC1);
		AudioADC_PGASel(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2);
		AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_LEFT, LINEIN3_LEFT_OR_MIC1, 15, 4);//0db bypass
		AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2, 15, 4);

		//Mic1   digital
		AudioADC_DigitalInit(ADC1_MODULE, sampleRate, (void*)mainAppCt.ADCFIFO, AudioCoreFrameSizeGet(DefaultNet) * 2 * 2 * 2);

		//Soure0.
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = TRUE;
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.DataIOFunc = AudioADC1DataGet;
		AudioIOSet.LenGetFunc = AudioADC1DataLenGet;
#ifdef CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;			//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 0;	//需要数据位宽不扩展
#endif
		if(!AudioCoreSourceInit(&AudioIOSet, MIC_SOURCE_NUM))
		{
			DBG("mic Source error");
			return FALSE;
		}
		AudioCoreSourceEnable(MIC_SOURCE_NUM);
	}
	else //采样率等 重配
	{
		AudioCoreSourceDisable(MIC_SOURCE_NUM);
//		AudioADC_AnaInit();
		//AudioADC_VcomConfig(1);//MicBias en
//		AudioADC_MicBias1Enable(1);

		AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_LEFT, LINEIN3_LEFT_OR_MIC1, gain, gainBoostSel);
		AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2, gain, gainBoostSel);

		//Mic1	 digital
		memset(mainAppCt.ADCFIFO, 0, AudioCoreFrameSizeGet(DefaultNet) * 2 * 2 * 2);
		AudioADC_DigitalInit(ADC1_MODULE, sampleRate, (void*)mainAppCt.ADCFIFO, AudioCoreFrameSizeGet(DefaultNet) * 2 * 2 * 2);
	}
#endif

#ifdef BT_TWS_SUPPORT
	if(!AudioCoreSourceIsInit(TWS_SOURCE_NUM))
	{
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = FALSE;//TRUE
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.DataIOFunc = TWSDataGet;
		AudioIOSet.LenGetFunc = TWSDataLenGet;
#ifdef CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;			//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 0;	//需要数据位宽不扩展
#endif
		if(!AudioCoreSourceInit(&AudioIOSet, TWS_SOURCE_NUM))
		{
			DBG("Tws Source error");
			return FALSE;
		}

	}
	AudioCoreSourceEnable(TWS_SOURCE_NUM);
#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(!AudioCoreSourceIsInit(REMIND_SOURCE_NUM))
	{
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));

		AudioIOSet.Adapt = SRC_ONLY;
		AudioIOSet.SampleRate = CFG_PARA_SAMPLE_RATE;//初始值
		AudioIOSet.Sync = FALSE;
		AudioIOSet.Channels = 1;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.DataIOFunc = RemindDataGet;
		AudioIOSet.LenGetFunc = RemindDataLenGet;

#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 0;//需要数据进行位宽扩展
#endif
		if(!AudioCoreSourceInit(&AudioIOSet, REMIND_SOURCE_NUM))
		{
			DBG("remind source error!\n");
			SoftFlagRegister(SoftFlagNoRemind);
			return FALSE;
		}
#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY		
		RemindMp3DecoderInit();
#endif
	}
#endif


#ifdef CFG_RES_AUDIO_DAC0_EN
	AudioCoreSinkDisable(AUDIO_DAC0_SINK_NUM);
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	#ifdef TWS_DAC0_OUT
	AudioIOSet.Resident = TRUE;
	AudioIOSet.Depth = TWS_SINK_DEV_FIFO_SAMPLES;
	#else
	AudioIOSet.Depth = AudioCore.FrameSize[DefaultNet] * 2 ;
	#endif
	if(!AudioCoreSinkIsInit(AUDIO_DAC0_SINK_NUM))
	{
	#ifndef TWS_DAC0_OUT
		mainAppCt.DACFIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
		mainAppCt.DACFIFO = (uint32_t*)osPortMallocFromEnd(mainAppCt.DACFIFO_LEN);//DAC fifo
	#endif
		if(mainAppCt.DACFIFO != NULL)
		{
			memset(mainAppCt.DACFIFO, 0, mainAppCt.DACFIFO_LEN);
		}
		else
		{
			APP_DBG("malloc DACFIFO error\n");
			return FALSE;
		}
		//sink0

		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = TRUE;
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.DataIOFunc = AudioDAC0DataSet;
		AudioIOSet.LenGetFunc = AudioDAC0DataSpaceLenGet;

#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 1;//DAC 24bit ,sink最后一级输出时需要转变为24bi
#endif
		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_DAC0_SINK_NUM))
		{
			DBG("Dac init error");
			return FALSE;
		}
		AudioDAC_Init(DAC0, sampleRate, (void*)mainAppCt.DACFIFO, mainAppCt.DACFIFO_LEN, NULL, 0);
	}
	else
	{
		AudioDAC_SampleRateChange(DAC0, sampleRate);
#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 1;//DAC 24bit ,sink最后一级输出时需要转变为24bi
		AudioCore.AudioSink[AUDIO_DAC0_SINK_NUM].BitWidth = AudioIOSet.IOBitWidth;
		AudioCore.AudioSink[AUDIO_DAC0_SINK_NUM].BitWidthConvFlag = AudioIOSet.IOBitWidthConvFlag;
#endif
	}
	AudioCoreSinkEnable(AUDIO_DAC0_SINK_NUM);
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	#ifdef TWS_DAC1_OUT
	AudioIOSet.Resident = TRUE;
	AudioIOSet.Depth = TWS_SINK_DEV_FIFO_SAMPLES;
	#else
	AudioIOSet.Depth = AudioCore.FrameSize[DefaultNet] * 2 ;
	#endif
	if(!AudioCoreSinkIsInit(AUDIO_DACX_SINK_NUM))
	{
	#ifndef TWS_DACX_OUT
		mainAppCt.DACXFIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE);
		mainAppCt.DACXFIFO = (uint32_t*)osPortMallocFromEnd(mainAppCt.DACXFIFO_LEN);//DACX fifo
	#endif
		if(mainAppCt.DACXFIFO != NULL)
		{
			memset(mainAppCt.DACXFIFO, 0, mainAppCt.DACXFIFO_LEN);
		}
		else
		{
			APP_DBG("malloc DACXFIFO error\n");
			return FALSE;
		}
		//不改变原来结构，sink2 用于DAC-X，可以根据实际情况调整

		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = TRUE;
		AudioIOSet.Channels = 1;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.DataIOFunc = AudioDAC1DataSet;
		AudioIOSet.LenGetFunc = AudioDAC1DataSpaceLenGet;
#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 1;//不需要做位宽转换处理
#endif
		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_DACX_SINK_NUM))
		{
			DBG("Dacx init error");
			return FALSE;
		}
		AudioDAC_Init(DAC1, sampleRate, NULL, 0, (void*)mainAppCt.DACXFIFO, mainAppCt.DACXFIFO_LEN);
	}
	else
	{
		AudioDAC_SampleRateChange(DAC1, sampleRate);
	#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 1;//DAC 24bit ,sink最后一级输出时需要转变为24bi
		AudioCore.AudioSink[AUDIO_DACX_SINK_NUM].BitWidth = AudioIOSet.IOBitWidth;
		AudioCore.AudioSink[AUDIO_DACX_SINK_NUM].BitWidthConvFlag = AudioIOSet.IOBitWidthConvFlag;
	#endif
	}
	AudioCoreSinkEnable(AUDIO_DACX_SINK_NUM);
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN
	#if defined(TWS_IIS0_OUT) || defined(TWS_IIS1_OUT)
	AudioIOSet.Resident = TRUE;
	AudioIOSet.Depth = TWS_SINK_DEV_FIFO_SAMPLES;
	#else
	AudioIOSet.Depth = AudioCore.FrameSize[DefaultNet] * 2 ;
	#endif
	AudioCoreSinkDeinit(AUDIO_I2SOUT_SINK_NUM);
	if(!AudioCoreSinkIsInit(AUDIO_I2SOUT_SINK_NUM))
	{
	#if !defined(TWS_IIS0_OUT) && !defined(TWS_IIS1_OUT)
		mainAppCt.I2SFIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
		mainAppCt.I2SFIFO = (uint32_t*)osPortMallocFromEnd(mainAppCt.I2SFIFO_LEN);//I2S fifo
	#endif

		if(mainAppCt.I2SFIFO != NULL)
		{
			memset(mainAppCt.I2SFIFO, 0, mainAppCt.I2SFIFO_LEN);
		}
		else
		{
			APP_DBG("malloc I2SFIFO error\n");
			return FALSE;
		}


#if CFG_RES_I2S_MODE == 0 || !defined(CFG_FUNC_I2S_OUT_SYNC_EN)// Master 或不开微调
#if CFG_PARA_I2S_SAMPLERATE == CFG_BTHF_PARA_SAMPLE_RATE
		AudioIOSet.Adapt = STD;//SRC_ONLY
#else
		AudioIOSet.Adapt = SRC_ONLY;
#endif
#else//slave
#if CFG_PARA_I2S_SAMPLERATE == CFG_BTHF_PARA_SAMPLE_RATE
		AudioIOSet.Adapt = SRA_ONLY;//CLK_ADJUST_ONLY;
#else
		AudioIOSet.Adapt = SRC_SRA;//SRC_ADJUST;
#endif
#endif
		AudioIOSet.Sync = TRUE;//I2S slave 时候如果master没有接，有可能会导致DAC也不出声音。
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.HighLevelCent = 60;
		AudioIOSet.LowLevelCent = 40;
		AudioIOSet.SampleRate = CFG_PARA_I2S_SAMPLERATE;//根据实际外设选择
//		AudioIOSet.CoreSampleRate = CFG_BTHF_PARA_SAMPLE_RATE;
#if (CFG_RES_I2S == 0)
		AudioIOSet.DataIOFunc = AudioI2S0_DataSet;
		AudioIOSet.LenGetFunc = AudioI2S0_DataSpaceLenGet;
#else
		AudioIOSet.DataIOFunc = AudioI2S1_DataSet;
		AudioIOSet.LenGetFunc = AudioI2S1_DataSpaceLenGet;
#endif
#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 1;//不需要做位宽转换处理
#endif
		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_I2SOUT_SINK_NUM))
		{
			DBG("I2S out init error");
			return FALSE;
		}

		AudioI2sOutParamsSet();
		AudioCoreSinkEnable(AUDIO_I2SOUT_SINK_NUM);
		AudioCoreSinkAdjust(AUDIO_I2SOUT_SINK_NUM, TRUE);
	}
	else
	{
		I2S_SampleRateSet(CFG_RES_I2S_MODULE, CFG_PARA_I2S_SAMPLERATE);
	#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 1;//DAC 24bit ,sink最后一级输出时需要转变为24bi
		AudioCore.AudioSink[AUDIO_I2SOUT_SINK_NUM].BitWidth = AudioIOSet.IOBitWidth;
		AudioCore.AudioSink[AUDIO_I2SOUT_SINK_NUM].BitWidthConvFlag = AudioIOSet.IOBitWidthConvFlag;
		AudioCore.AudioSink[AUDIO_I2SOUT_SINK_NUM].Sync = FALSE;
	#endif
	}
#endif
	return TRUE;
}


#ifdef BT_TWS_SUPPORT
void tws_device_close(void)
{
	btManager.TwsDacStatus = FALSE;
#ifdef  TWS_IIS0_OUT
	*(volatile unsigned long *)0x40029000 &= ~0x80;//enable iis0
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_I2S0_TX);
	DMA_CircularConfig(PERIPHERAL_ID_I2S0_TX, mainAppCt.I2SFIFO_LEN/2, mainAppCt.I2SFIFO,mainAppCt.I2SFIFO_LEN);
	DMA_CircularWritePtrSet(PERIPHERAL_ID_I2S0_TX, mainAppCt.I2SFIFO_LEN - sizeof(PCM_DATA_TYPE) * 2);
	DMA_ChannelEnable(PERIPHERAL_ID_I2S0_TX);
#endif
#ifdef TWS_IIS1_OUT
	*(volatile unsigned long *)0x4002A000 &= ~0x80;//enable iis1
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S1_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S1_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S1_TX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_I2S1_TX);
	DMA_CircularConfig(PERIPHERAL_ID_I2S1_TX, mainAppCt.I2SFIFO_LEN/2, mainAppCt.I2SFIFO, mainAppCt.I2SFIFO_LEN);
	DMA_CircularWritePtrSet(PERIPHERAL_ID_I2S1_TX, mainAppCt.I2SFIFO_LEN - sizeof(PCM_DATA_TYPE) * 2);
	DMA_ChannelEnable(PERIPHERAL_ID_I2S1_TX);
#endif
#ifdef TWS_DAC0_OUT
	AudioDAC_Disable(DAC0);
	AudioDAC_Reset(DAC0);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_DAC0_TX);
	DMA_CircularConfig(PERIPHERAL_ID_AUDIO_DAC0_TX, mainAppCt.DACFIFO_LEN/2, mainAppCt.DACFIFO, mainAppCt.DACFIFO_LEN);
	DMA_CircularWritePtrSet(PERIPHERAL_ID_AUDIO_DAC0_TX, mainAppCt.DACFIFO_LEN - sizeof(PCM_DATA_TYPE) * 2);
	DMA_ChannelEnable(PERIPHERAL_ID_AUDIO_DAC0_TX);

#endif
#ifdef TWS_DACX_OUT
	AudioDAC_Disable(DAC1);
	AudioDAC_Reset(DAC1);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC1_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC1_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC1_TX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_DAC1_TX);
	DMA_CircularConfig(PERIPHERAL_ID_AUDIO_DAC1_TX, mainAppCt.DACXFIFO_LEN / 2, mainAppCt.DACXFIFO, mainAppCt.DACXFIFO_LEN);
#ifdef CFG_AUDIO_WIDTH_24BIT
	DMA_ConfigModify(PERIPHERAL_ID_AUDIO_DAC1_TX, DMA_SRC_DWIDTH_WORD, 	DMA_SRC_DWIDTH_WORD, DMA_SRC_AINCR_SRC_WIDTH, DMA_DST_AINCR_NO);
#endif
	DMA_CircularWritePtrSet(PERIPHERAL_ID_AUDIO_DAC1_TX, mainAppCt.DACXFIFO_LEN - sizeof(PCM_DATA_TYPE));
	DMA_ChannelEnable(PERIPHERAL_ID_AUDIO_DAC1_TX);
#endif
}

__attribute__((section(".tws_sync_code")))
void tws_device_open_isr(void)
{
	btManager.TwsDacStatus = TRUE;
#ifdef TWS_DAC0_OUT
	AudioDAC_Enable(DAC0);
#endif
#ifdef  TWS_IIS0_OUT
	*(volatile unsigned long *)0x40029000 |= 0x80;//enable iis0
#endif
#ifdef TWS_IIS1_OUT
	*(volatile unsigned long *)0x4002A000 |= 0x80;//enable iis1
#endif
#ifdef TWS_DACX_OUT
	AudioDAC_Enable(DAC1);
#endif
}

void tws_device_resume(void)
{
	btManager.TwsDacStatus = TRUE;
#ifdef TWS_IIS0_OUT
	if ((*(volatile unsigned long *)0x40029000 & 0x80) == 0)
	{
		*(volatile unsigned long *)0x40029000 |= 0x80;
	}
#endif
#ifdef TWS_IIS1_OUT
	if ((*(volatile unsigned long *)0x4002A000 & 0x80) == 0)
	{
		*(volatile unsigned long *)0x4002A000 |= 0x80;
	}
#endif
#ifdef TWS_DAC0_OUT
	if(((*(volatile unsigned long *)0x4002E000)&0x01) == 0x0)
	{
		AudioDAC_Enable(DAC0);
	}
#endif
#ifdef TWS_DACX_OUT
	if(((*(volatile unsigned long *)0x4002E024)&0x01) == 0x0)
	{
		AudioDAC_Enable(DAC1);
	}
#endif
}
#else
void tws_device_open_isr(void)
{

}
#endif

//各模式下的通用消息处理, 共有的提示音在此处理，因此要求调用次API前，确保APP running状态。避免解码器没准备好。
void CommonMsgProccess(uint16_t Msg)
{
#if defined(CFG_FUNC_DISPLAY_EN)
	MessageContext	msgSend;
#endif
	if(SoftFlagGet(SoftFlagDiscDelayMask) && Msg == MSG_NONE)
	{
		Msg = MSG_BT_STATE_DISCONNECT;
	}

	switch(Msg)
	{
#ifdef TWS_CODE_BACKUP//BT_TWS_SUPPORT
		case MSG_TWS_UNMUTE:
			{
				if(g_tws_need_init == 0 && IsAudioPlayerMute() == TRUE)
				{
					HardWareMuteOrUnMute();
				}
			}
			break;
#endif	
		case MSG_MENU://菜单键
			APP_DBG("menu key\n");
			AudioPlayerMenu();
			break;

		case MSG_MUTE:
			APP_DBG("MSG_MUTE\n");
			#ifdef  CFG_APP_HDMIIN_MODE_EN
			extern HDMIInfo         *gHdmiCt;
			if(GetSystemMode() == ModeHdmiAudioPlay)
			{
				if(IsHDMISourceMute() == TRUE)
					HDMISourceUnmute();
				else
					HDMISourceMute();
				gHdmiCt->hdmiActiveReportMuteStatus = IsHDMISourceMute();
				gHdmiCt->hdmiActiveReportMuteflag = 2;
			}
			else
			#endif
			{
				HardWareMuteOrUnMute();
			}
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_MUTE;
            MessageSend(GetSysModeMsgHandle(), &msgSend);
			#endif
			break;

		case MSG_MAIN_VOL_UP:
			SystemVolUp();
			APP_DBG("MSG_MAIN_VOL_UP\n");
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_VOL;
            MessageSend(GetSysModeMsgHandle(), &msgSend);
			#endif
			break;

		case MSG_MAIN_VOL_DW:
			SystemVolDown();
			APP_DBG("MSG_MAIN_VOL_DW\n");
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_VOL;
            MessageSend(GetSysModeMsgHandle(), &msgSend);
			#endif
			break;

		case MSG_MUSIC_VOLUP:
			AudioMusicVolUp();
#ifdef BT_TWS_SUPPORT
			tws_vol_send(mainAppCt.MusicVolume, IsAudioPlayerMute());
#endif
			APP_DBG("MSG_MUSIC_VOLUP\n");
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_MUSIC_VOL;
            MessageSend(GetSysModeMsgHandle(), &msgSend);
			#endif
			break;

		case MSG_MUSIC_VOLDOWN:
			AudioMusicVolDown();
#ifdef BT_TWS_SUPPORT
			tws_vol_send(mainAppCt.MusicVolume, IsAudioPlayerMute());
#endif
			APP_DBG("MSG_MUSIC_VOLDOWN\n");
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_MUSIC_VOL;
            MessageSend(GetSysModeMsgHandle(), &msgSend);
			#endif
			break;

		#if CFG_RES_MIC_SELECT
		case MSG_MIC_VOLUP:
			AudioMicVolUp();
			APP_DBG("MSG_MIC_VOLUP\n");
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_MIC_VOL;
            MessageSend(GetSysModeMsgHandle(), &msgSend);
			#endif
			break;

		case MSG_MIC_VOLDOWN:
			AudioMicVolDown();
			APP_DBG("MSG_MIC_VOLDOWN\n");
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_MIC_VOL;
            MessageSend(GetSysModeMsgHandle(), &msgSend);
			#endif
			break;
		#endif

#ifdef CFG_APP_BT_MODE_EN
		case MSG_BT_PLAY_SYNC_VOLUME_CHANGED:
			APP_DBG("MSG_BT_PLAY_SYNC_VOLUME_CHANGED\n");
#if (BT_AVRCP_VOLUME_SYNC == ENABLE)
			AudioMusicVolSet(GetBtSyncVolume());
#endif
#ifdef BT_TWS_SUPPORT
			tws_vol_send(mainAppCt.MusicVolume, IsAudioPlayerMute());
#endif
			break;
#endif

#ifdef CFG_FUNC_AUDIO_EFFECT_EN
		#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
		case MSG_EQ:
			APP_DBG("MSG_EQ\n");
			if(mainAppCt.EqMode < EQ_MODE_VOCAL_BOOST)
			{
				mainAppCt.EqMode++;
			}
			else
			{
				mainAppCt.EqMode = EQ_MODE_FLAT;
			}
			APP_DBG("EqMode = %d\n", mainAppCt.EqMode);
			#ifdef BT_TWS_SUPPORT
			if (btManager.twsState == BT_TWS_STATE_CONNECTED)
			{
				if(GetBtManager()->twsRole == BT_TWS_MASTER)
				{
					#ifndef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
					EqModeSet(mainAppCt.EqMode);
					#endif
					tws_eq_mode_send(mainAppCt.EqMode);
				}
			}
			else
			#endif
			{
				#ifndef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
				EqModeSet(mainAppCt.EqMode);
				#endif
			}

			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_EQ;
            MessageSend(GetSysModeMsgHandle(), &msgSend);
			#endif
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
			break;
		#endif

		#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
		case MSG_MUSIC_TREB_UP:
			APP_DBG("MSG_MUSIC_TREB_UP\n");
			if(mainAppCt.MusicTrebStep < MAX_BASS_TREB_GAIN)
			{
				mainAppCt.MusicTrebStep++;
			}
			APP_DBG("MusicTrebStep = %d\n", mainAppCt.MusicTrebStep);
			#ifdef BT_TWS_SUPPORT
			if (btManager.twsState == BT_TWS_STATE_CONNECTED)
			{
				if(GetBtManager()->twsRole == BT_TWS_MASTER)
				{
					MusicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);

					tws_music_bass_treb_send(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
				}
			}
			else
			#endif
			{
				MusicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
			}
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
			break;

		case MSG_MUSIC_TREB_DW:
			APP_DBG("MSG_MUSIC_TREB_DW\n");
			if(mainAppCt.MusicTrebStep)
			{
				mainAppCt.MusicTrebStep--;
			}
			APP_DBG("MusicTrebStep = %d\n", mainAppCt.MusicTrebStep);
			#ifdef BT_TWS_SUPPORT
			if (btManager.twsState == BT_TWS_STATE_CONNECTED)
			{
				if(GetBtManager()->twsRole == BT_TWS_MASTER)
				{
					MusicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);

					tws_music_bass_treb_send(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
				}
			}
			else
			#endif
			{
				MusicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
			}
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
			break;

		case MSG_MUSIC_BASS_UP:
			APP_DBG("MSG_MUSIC_BASS_UP\n");
			if(mainAppCt.MusicBassStep < MAX_BASS_TREB_GAIN)
			{
				mainAppCt.MusicBassStep++;
			}
			APP_DBG("MusicBassStep = %d\n", mainAppCt.MusicBassStep);
			
			#ifdef BT_TWS_SUPPORT
			if (btManager.twsState == BT_TWS_STATE_CONNECTED)
			{
				if(GetBtManager()->twsRole == BT_TWS_MASTER)
				{
					MusicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);

					tws_music_bass_treb_send(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
				}
			}
			else
			#endif
			{
				MusicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
			}

			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
			break;

		case MSG_MUSIC_BASS_DW:
			APP_DBG("MSG_MUSIC_BASS_DW\n");
			if(mainAppCt.MusicBassStep)
			{
				mainAppCt.MusicBassStep--;
			}
			APP_DBG("MusicBassStep = %d\n", mainAppCt.MusicBassStep);
			#ifdef BT_TWS_SUPPORT
			if (btManager.twsState == BT_TWS_STATE_CONNECTED)
			{
				if(GetBtManager()->twsRole == BT_TWS_MASTER)
				{
					MusicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);

					tws_music_bass_treb_send(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
				}
			}
			else
			#endif
			{
				MusicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
			}
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
			break;
		#endif

		case MSG_3D:
			APP_DBG("MSG_3D\n");
			#if CFG_AUDIO_EFFECT_MUSIC_3D_EN
			gCtrlVars.music_threed_unit.three_d_en = !gCtrlVars.music_threed_unit.three_d_en;
			#endif

			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_3D;
            MessageSend(GetSysModeMsgHandle(), &msgSend);
			#endif
			break;

		case MSG_VOCAL_CUT:
//			APP_DBG("MSG_VOCAL_CUT\n");
//			gCtrlVars.vocal_cut_unit.vocal_cut_en = !gCtrlVars.vocal_cut_unit.vocal_cut_en;
//			#ifdef CFG_FUNC_DISPLAY_EN
//            msgSend.msgId = MSG_DISPLAY_SERVICE_VOCAL_CUT;
//            MessageSend(GetSysModeMsgHandle(), &msgSend);
//			#endif
			break;

		case MSG_VB:
			APP_DBG("MSG_VB\n");
			#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN
			gCtrlVars.music_vb_unit.vb_en = !gCtrlVars.music_vb_unit.vb_en;
			#endif
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_VB;
            MessageSend(GetSysModeMsgHandle(), &msgSend);
			#endif
			break;

		case MSG_EFFECTMODE:
			//Sam: 音效切换测试用，用户可以根据修改
			#ifdef CFG_EFFECT_PARAM_IN_FLASH_EN
			if(mainAppCt.SysCurrentMode != ModeBtHfPlay)//AEC 不参与 音效模式切换，后续程序更新需要同步更新
			{
				IsEffectChangeReload = 1;
				mainAppCt.EffectMode ++;
				if(mainAppCt.EffectMode >= CFG_EFFECT_PARAM_IN_FLASH_ACTIVCE_NUM)
				{
					mainAppCt.EffectMode = EFFECT_MODE_FLASH_Music;
				}
				if(mainAppCt.EffectMode >= EFFECT_MODE_NORMAL)
				{
					mainAppCt.EffectMode = EFFECT_MODE_FLASH_Music;
				}

				APP_DBG("mainAppCt.EffectMode: %d\n", mainAppCt.EffectMode);
			}
			#endif
			break;
#endif

		case MSG_REC_MUSIC:
			SetRecMusic(0);
			break;

		#ifdef CFG_FUNC_RTC_EN
		case MSG_RTC_SET_TIME:
			RTC_ServiceModeSelect(0,0);
			break;

		case MSG_RTC_SET_ALARM:
			RTC_ServiceModeSelect(0,1);
			break;

		case MSG_RTC_DISP_TIME:
			RTC_ServiceModeSelect(0,2);
			break;

		case MSG_RTC_UP:
			RTC_RtcUp();
			break;

		case MSG_RTC_DOWN:
			RTC_RtcDown();
			break;

		#ifdef CFG_FUNC_SNOOZE_EN
		case MSG_RTC_SNOOZE:
			if(mainAppCt.AlarmRemindStart)
			{
				mainAppCt.AlarmRemindCnt = 0;
				mainAppCt.AlarmRemindStart = 0;
				mainAppCt.SnoozeOn = 1;
				mainAppCt.SnoozeCnt = 0;
			}
			break;
		#endif

		#endif //end of  CFG_FUNC_RTC_EN

        #ifdef BT_TWS_SUPPORT
		case MSG_BT_TWS_OUT_MODE:
            APP_DBG("MSG_BT_TWS_OUT_MODE\n");
			break;
		#endif

		case MSG_REMIND1:
			#ifdef CFG_FUNC_REMIND_SOUND_EN
			RemindSoundServiceItemRequest(SOUND_REMIND_SHANGYIS,FALSE);
			#endif
			break;

		case MSG_DEVICE_SERVICE_BATTERY_LOW:
			//RemindSound request
			APP_DBG("MSG_DEVICE_SERVICE_BATTERY_LOW\n");
			#ifdef CFG_FUNC_REMIND_SOUND_EN
			RemindSoundServiceItemRequest(SOUND_REMIND_DLGUODI, FALSE);
			#endif
			break;

#ifdef CFG_APP_BT_MODE_EN
		#ifdef POWER_ON_BT_ACCESS_MODE_SET
		case MSG_BT_OPEN_ACCESS:
			if (GetBtManager()->btAccessModeEnable == 0)
			{
				GetBtManager()->btAccessModeEnable = 1;
				BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);
				DBG("open bt access\n");
			}
			break;
		#endif
			
		//蓝牙连接断开消息,用于提示音
		case MSG_BT_STATE_CONNECTED:
			APP_DBG("[BT_STATE]:BT Connected...\n");
			//异常回连过程中，不提示连接断开提示音
			#ifdef BT_USER_VISIBILITY_STATE
			SetBtUserState(BT_USER_STATE_CONNECTED);
			#endif
			BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);

			if(btManager.btDutModeEnable)
				break;

			//if(!(btCheckEventList&BT_EVENT_L2CAP_LINK_DISCONNECT))
			{
				#ifdef CFG_FUNC_REMIND_SOUND_EN
				if(RemindSoundServiceItemRequest(SOUND_REMIND_CONNECT, REMIND_PRIO_SYS|REMIND_ATTR_NEED_HOLD_PLAY))
				{
					if(!SoftFlagGet(SoftFlagWaitBtRemindEnd)&&SoftFlagGet(SoftFlagDelayEnterBtHf))
					{
						SoftFlagRegister(SoftFlagWaitBtRemindEnd);
					}
				}
				else
				#endif
				{
					if(SoftFlagGet(SoftFlagDelayEnterBtHf))
					{
						MessageContext		msgSend;
						SoftFlagDeregister(SoftFlagDelayEnterBtHf);

						msgSend.msgId = MSG_DEVICE_SERVICE_ENTER_BTHF_MODE;
						MessageSend(GetMainMessageHandle(), &msgSend);
					}
				}
			}
			break;

		case MSG_BT_STATE_DISCONNECT:
			APP_DBG("[BT_STATE]:BT Disconnected...\n");
			SoftFlagDeregister(SoftFlagDiscDelayMask);
			
			#ifdef BT_USER_VISIBILITY_STATE
							SetBtUserState(BT_USER_STATE_DISCONNECTED);
			#endif
			if(btManager.btDutModeEnable)
				break;

			BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);

			//异常回连过程中，不提示连接断开提示音
			//if(!(btCheckEventList&BT_EVENT_L2CAP_LINK_DISCONNECT))
			{
				#ifdef CFG_FUNC_REMIND_SOUND_EN
				if(GetSystemMode() != ModeIdle)
				{
					RemindSoundServiceItemRequest(SOUND_REMIND_DISCONNE, REMIND_PRIO_SYS|REMIND_ATTR_NEED_HOLD_PLAY);
				}
				#endif
			}
			break;
#endif

		default:
	#ifdef CFG_FUNC_DISPLAY_TASK_EN
			MsgSendToDisplayTask(Msg);
	#else
			#ifdef CFG_FUNC_DISPLAY_EN
			//display
			Display(Msg);
			#endif
	#endif
			#ifdef CFG_ADC_LEVEL_KEY_EN
			AdcLevelMsgProcess(Msg);
			#endif
			break;
	}
}
