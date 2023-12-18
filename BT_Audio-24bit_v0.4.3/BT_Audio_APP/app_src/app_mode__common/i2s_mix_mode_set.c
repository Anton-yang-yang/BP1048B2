#include "type.h"
#include "dma.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "app_config.h"
#include "mode_task_api.h"
#include "main_task.h"
#include "debug.h"
#include "i2s_interface.h"
//service
#include "audio_core_api.h"
#include "audio_core_service.h"
#include "mode_task_api.h"

#ifdef CFG_FUNC_I2S_MIX_MODE

#define	CFG_RES_I2S_MIX_MODE					0 //0:master mode ;1:slave mode 外设未接不要配slave

static void AudioI2s0GPIOSet(void)
{
#if CFG_RES_I2S0_IO_PORT == 0			
	//I2S0_MODULE Port0
	#if (CFG_RES_I2S_MIX_MODE == 0)
	GPIO_PortAModeSet(GPIOA0, 9);// mclk 3:in;9:out
	#else
	GPIO_PortAModeSet(GPIOA0, 3);
	#endif
	GPIO_PortAModeSet(GPIOA1, 6);// lrclk
	GPIO_PortAModeSet(GPIOA2, 5);// bclk
	
	#ifdef CFG_RES_AUDIO_I2S0OUT_EN
	GPIO_PortAModeSet(GPIOA3, 7);// out
	#endif
	#ifdef CFG_RES_AUDIO_I2S0IN_EN
	GPIO_PortAModeSet(GPIOA4, 1);// din
	#endif

#else									
	//I2S0_MODULE Port1
	#if (CFG_RES_I2S_MIX_MODE == 0)
	GPIO_PortAModeSet(GPIOA24, 9);//mclk 3:in;9:out
	#else
	GPIO_PortAModeSet(GPIOA24, 3);
	#endif
	GPIO_PortAModeSet(GPIOA20, 6);//lrclk
	GPIO_PortAModeSet(GPIOA21, 5);//bclk
	
	#ifdef CFG_RES_AUDIO_I2S0OUT_EN
	GPIO_PortAModeSet(GPIOA22, 10);//do
	#endif
	#ifdef CFG_RES_AUDIO_I2S0IN_EN
	GPIO_PortAModeSet(GPIOA23, 3);//di
	#endif
#endif
}

static void AudioI2s1GPIOSet(void)
{
#if CFG_RES_I2S1_IO_PORT == 0		
    //I2S1_MODULE Port0
	#if (CFG_RES_I2S_MIX_MODE == 0)
	GPIO_PortAModeSet(GPIOA27, 6);	//mclk 1:in;6:out
	#else
	GPIO_PortAModeSet(GPIOA27, 1);
	#endif
	
	GPIO_PortAModeSet(GPIOA28, 1);	//lrclk
	GPIO_PortAModeSet(GPIOA29, 1);	//bclk

	#ifdef CFG_RES_AUDIO_I2S1OUT_EN
	GPIO_PortAModeSet(GPIOA30, 6);	//do
	#endif
	#ifdef CFG_RES_AUDIO_I2S1IN_EN
	GPIO_PortAModeSet(GPIOA31, 2);	//di
	#endif

#elif CFG_RES_I2S1_IO_PORT == 1		
    //I2S1_MODULE Port1
	#if (CFG_RES_I2S_MIX_MODE == 0)
	GPIO_PortAModeSet(GPIOA7, 5);	//mclk 2:in;5:out
	#else
	GPIO_PortAModeSet(GPIOA7, 3);
	#endif
	GPIO_PortAModeSet(GPIOA8, 1);	//lrclk
	GPIO_PortAModeSet(GPIOA9, 2);	//bclk
	#ifdef CFG_RES_AUDIO_I2S1OUT_EN
	GPIO_PortAModeSet(GPIOA10, 4);	//do
	#endif
	#ifdef CFG_RES_AUDIO_I2S1IN_EN
	GPIO_PortAModeSet(GPIOA11, 2);	//di
	#endif

#elif CFG_RES_I2S1_IO_PORT == 2		
    //I2S1_MODULE Port2
	GPIO_PortAModeSet(GPIOA1, 7);//lrclk
	GPIO_PortAModeSet(GPIOA2, 6);//bclk
	#ifdef CFG_RES_AUDIO_I2S1OUT_EN
	GPIO_PortAModeSet(GPIOA31, 5);//do
	#endif
	#ifdef CFG_RES_AUDIO_I2S1IN_EN
	GPIO_PortAModeSet(GPIOA30, 2);//di
	#endif

#else								 
    //I2S1_MODULE Port3
	GPIO_PortAModeSet(GPIOA20, 7);//lrclk
	GPIO_PortAModeSet(GPIOA21, 6);//bclk
	#ifdef CFG_RES_AUDIO_I2S1OUT_EN
	GPIO_PortAModeSet(GPIOA11, 4);//do
	#endif
	#ifdef CFG_RES_AUDIO_I2S1IN_EN
	GPIO_PortAModeSet(GPIOA10, 1);//di
	#endif
#endif
}

void AudioI2s0ParamsSet(void)
{
#ifdef CFG_RES_I2S0_EN
	I2SParamCt i2s0_set;
	i2s0_set.IsMasterMode = CFG_RES_I2S_MIX_MODE;// 0:master 1:slave
	i2s0_set.SampleRate = CFG_PARA_I2S_SAMPLERATE; //外设采样率
	i2s0_set.I2sFormat = I2S_FORMAT_I2S;
	
#ifdef	CFG_AUDIO_WIDTH_24BIT
	i2s0_set.I2sBits = I2S_LENGTH_24BITS;
#else
	i2s0_set.I2sBits = I2S_LENGTH_16BITS;
#endif

	#if defined (CFG_RES_AUDIO_I2S0IN_EN) && defined (CFG_RES_AUDIO_I2S0OUT_EN)
	i2s0_set.I2sTxRxEnable = 3;
	#elif defined (CFG_RES_AUDIO_I2S0IN_EN)
	i2s0_set.I2sTxRxEnable = 2;
	#elif defined (CFG_RES_AUDIO_I2S0OUT_EN)
	i2s0_set.I2sTxRxEnable = 1;
	#else
	i2s0_set.I2sTxRxEnable = 0;
	#endif

	i2s0_set.TxPeripheralID = PERIPHERAL_ID_I2S0_TX;
	i2s0_set.RxPeripheralID = PERIPHERAL_ID_I2S0_RX;
#ifdef CFG_RES_AUDIO_I2S0OUT_EN
	i2s0_set.TxBuf = (void*)mainAppCt.I2S0_TX_FIFO;
	i2s0_set.TxLen = AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2;
#endif
#ifdef CFG_RES_AUDIO_I2S0IN_EN
	i2s0_set.RxBuf = (void*)mainAppCt.I2S0_RX_FIFO;
	i2s0_set.RxLen = AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2 ;
#endif
	AudioI2s0GPIOSet();

	//I2S_ModuleDisable(I2S0_MODULE);
	I2S_AlignModeSet(I2S0_MODULE, I2S_LOW_BITS_ACTIVE);
	AudioI2S_Init(I2S0_MODULE, &i2s0_set);
#endif
}


void AudioI2s1ParamsSet(void)
{
#ifdef CFG_RES_I2S1_EN
	I2SParamCt i2s1_set;
	i2s1_set.IsMasterMode = CFG_RES_I2S_MIX_MODE;// 0:master 1:slave
	i2s1_set.SampleRate = CFG_PARA_I2S_SAMPLERATE; //外设采样率
	i2s1_set.I2sFormat = I2S_FORMAT_I2S;
	
#ifdef	CFG_AUDIO_WIDTH_24BIT
	i2s1_set.I2sBits = I2S_LENGTH_24BITS;
#else
	i2s1_set.I2sBits = I2S_LENGTH_16BITS;
#endif

	#if defined (CFG_RES_AUDIO_I2S1IN_EN) && defined (CFG_RES_AUDIO_I2S1OUT_EN)
	i2s1_set.I2sTxRxEnable = 3;
	#elif defined (CFG_RES_AUDIO_I2S1IN_EN)
	i2s1_set.I2sTxRxEnable = 2;
	#elif defined (CFG_RES_AUDIO_I2S1OUT_EN)
	i2s1_set.I2sTxRxEnable = 1;
	#else
	i2s1_set.I2sTxRxEnable = 0;
	#endif

	i2s1_set.TxPeripheralID = PERIPHERAL_ID_I2S1_TX;
	i2s1_set.RxPeripheralID = PERIPHERAL_ID_I2S1_RX;

#ifdef CFG_RES_AUDIO_I2S1OUT_EN
	i2s1_set.TxBuf = (void*)mainAppCt.I2S1_TX_FIFO;
	i2s1_set.TxLen = AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2;
#endif
#ifdef CFG_RES_AUDIO_I2S1IN_EN
	i2s1_set.RxBuf = (void*)mainAppCt.I2S1_RX_FIFO;
	i2s1_set.RxLen = AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2 ;
#endif
	AudioI2s1GPIOSet();

	I2S_ModuleDisable(I2S1_MODULE);
	I2S_AlignModeSet(I2S1_MODULE, I2S_LOW_BITS_ACTIVE);
	AudioI2S_Init(I2S1_MODULE, &i2s1_set);
#endif
}


void I2s0MixModeAudioCoreInit(void)
{
	AudioCoreIO	AudioIOSet;
	
#ifdef CFG_RES_AUDIO_I2S0IN_EN
	if(!AudioCoreSourceIsInit(I2S0_SOURCE_NUM))
	{
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));

#if (CFG_RES_I2S_MIX_MODE == 0) || !defined(CFG_FUNC_I2S_IN_SYNC_EN)//master 或者关微调
		#if CFG_PARA_I2S_SAMPLERATE == CFG_PARA_SAMPLE_RATE
		AudioIOSet.Adapt = STD;
		#else
		AudioIOSet.Adapt = SRC_ONLY;
		#endif
#else //slave
		#if CFG_PARA_I2S_SAMPLERATE == CFG_PARA_SAMPLE_RATE
		AudioIOSet.Adapt = SRA_ONLY;//CLK_ADJUST_ONLY;//
		#else
		AudioIOSet.Adapt = SRC_SRA;//SRC_ADJUST;//
		#endif
#endif
		AudioIOSet.Sync = TRUE;//FALSE;//
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.Depth = AudioCoreFrameSizeGet(DefaultNet) * 2;//sI2SInPlayCt->I2SFIFO1 采样点深度
//		DBG("Depth:%d", AudioIOSet.Depth);
		AudioIOSet.HighLevelCent = 60;
		AudioIOSet.LowLevelCent = 40;
		AudioIOSet.SampleRate = CFG_PARA_I2S_SAMPLERATE;//根据实际外设选择

		AudioIOSet.DataIOFunc = AudioI2S0_DataGet ;
		AudioIOSet.LenGetFunc = AudioI2S0_DataLenGet;

		#if (defined(CFG_AUDIO_WIDTH_24BIT) && (CFG_FUNC_I2S1_MIX_EN == 0)) || !defined(BT_TWS_SUPPORT)
		AudioIOSet.IOBitWidth = 1;//0,16bit,1:24bit
		#else
		AudioIOSet.IOBitWidth = 0;//0,16bit,1:24bit
		#endif
		AudioIOSet.IOBitWidthConvFlag = 0;//tws不需要数据进行位宽扩展，会在TWS_SOURCE_NUM以后统一转成24bit

		if(!AudioCoreSourceInit(&AudioIOSet, I2S0_SOURCE_NUM))
		{
			DBG("I2S0_SOURCE_NUM source error!\n");
			return FALSE;
		}

		AudioI2s0ParamsSet();
	}
	AudioCoreSourceEnable(I2S0_SOURCE_NUM);
#endif

#ifdef CFG_RES_AUDIO_I2S0OUT_EN
	if(!AudioCoreSinkIsInit(AUDIO_I2S0_OUT_SINK_NUM))
	{
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));

		if(mainAppCt.I2S0_TX_FIFO != NULL){
			memset(mainAppCt.I2S0_TX_FIFO, 0, AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2);
		}else{
			APP_DBG("malloc I2S0_TX_FIFO error\n");
			return FALSE;
		}
		
		//I2SIn  digital (DMA)
		if(mainAppCt.I2S0_TX_FIFO  == NULL){
			return FALSE;
		}
		memset(mainAppCt.I2S0_TX_FIFO, 0, AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2);

#if CFG_PARA_I2S_SAMPLERATE == CFG_PARA_SAMPLE_RATE
		AudioIOSet.Adapt = STD;//SRC_ONLY
#else
		AudioIOSet.Adapt = SRC_ONLY;
#endif

		AudioIOSet.Sync = TRUE;//I2S slave 时候如果master没有接，有可能会导致DAC也不出声音。
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.HighLevelCent = 60;
		AudioIOSet.LowLevelCent = 40;
		AudioIOSet.SampleRate = CFG_PARA_I2S_SAMPLERATE;//根据实际外设选择
//		AudioIOSet.Depth = AudioCoreFrameSizeGet(DefaultNet) * 2 ;
#if defined(TWS_IIS0_OUT) || defined(TWS_IIS1_OUT)
		AudioIOSet.Resident = TRUE;
		AudioIOSet.Depth = TWS_SINK_DEV_FIFO_SAMPLES;
#else
		AudioIOSet.Depth = AudioCoreFrameSizeGet(DefaultNet) * 2 ;
#endif
		AudioIOSet.DataIOFunc = AudioI2S0_DataSet;
		AudioIOSet.LenGetFunc = AudioI2S0_DataSpaceLenGet;

#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 0;//不需要做位宽转换处理
#endif
		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_I2S0_OUT_SINK_NUM))
		{
			DBG("I2S0 out init error");
			return FALSE;
		}

		AudioI2s0ParamsSet();
		AudioCoreSinkEnable(AUDIO_I2S0_OUT_SINK_NUM);
		AudioCoreSinkAdjust(AUDIO_I2S0_OUT_SINK_NUM, TRUE);
	}
	else//sam add,20230221
	{
		I2S_SampleRateSet(I2S0_MODULE, CFG_PARA_SAMPLE_RATE);
		#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 0;//DAC 24bit ,sink最后一级输出时需要转变为24bi
		AudioCore.AudioSink[AUDIO_I2S0_OUT_SINK_NUM].BitWidth = AudioIOSet.IOBitWidth;
		AudioCore.AudioSink[AUDIO_I2S0_OUT_SINK_NUM].BitWidthConvFlag = AudioIOSet.IOBitWidthConvFlag;
		AudioCore.AudioSink[AUDIO_I2S0_OUT_SINK_NUM].Sync = TRUE;
		#endif
	}	
#endif
}

void I2s1MixModeAudioCoreInit(void)
{
	AudioCoreIO	AudioIOSet;
	
#ifdef CFG_RES_AUDIO_I2S1IN_EN
	if(!AudioCoreSourceIsInit(I2S1_SOURCE_NUM))
	{
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));

#if CFG_RES_I2S_MIX_MODE == 0 || !defined(CFG_FUNC_I2S_IN_SYNC_EN)//master 或者关微调
		#if CFG_PARA_I2S_SAMPLERATE == CFG_PARA_SAMPLE_RATE
		AudioIOSet.Adapt = STD;
		#else
		AudioIOSet.Adapt = SRC_ONLY;
		#endif
#else //slave
		#if CFG_PARA_I2S_SAMPLERATE == CFG_PARA_SAMPLE_RATE
		AudioIOSet.Adapt = SRA_ONLY;//CLK_ADJUST_ONLY;//
		#else
		AudioIOSet.Adapt = SRC_SRA;//SRC_ADJUST;//
		#endif
#endif

		AudioIOSet.Sync = TRUE;//FALSE;//
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.Depth = AudioCoreFrameSizeGet(DefaultNet) * 2;//sI2SInPlayCt->I2SFIFO1 采样点深度
//		DBG("Depth:%d", AudioIOSet.Depth);
		AudioIOSet.HighLevelCent = 60;
		AudioIOSet.LowLevelCent = 40;
		AudioIOSet.SampleRate = CFG_PARA_I2S_SAMPLERATE;//根据实际外设选择

		AudioIOSet.DataIOFunc = AudioI2S1_DataGet ;
		AudioIOSet.LenGetFunc = AudioI2S1_DataLenGet;

		#if (defined(CFG_AUDIO_WIDTH_24BIT) && (CFG_FUNC_I2S1_MIX_EN == 0)) || !defined(BT_TWS_SUPPORT)
		AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
		#else
		AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
		#endif
		AudioIOSet.IOBitWidthConvFlag = 0;//tws不需要数据进行位宽扩展，会在TWS_SOURCE_NUM以后统一转成24bit

		if(!AudioCoreSourceInit(&AudioIOSet, I2S1_SOURCE_NUM))
		{
			DBG("I2S1_SOURCE_NUM source error!\n");
			return FALSE;
		}

		AudioI2s1ParamsSet();
	}
	AudioCoreSourceEnable(I2S1_SOURCE_NUM);
#endif

#ifdef CFG_RES_AUDIO_I2S1OUT_EN
	if(!AudioCoreSinkIsInit(AUDIO_I2S1_OUT_SINK_NUM))
	{
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));

		if(mainAppCt.I2S1_TX_FIFO != NULL){
			memset(mainAppCt.I2S1_TX_FIFO, 0, AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2);
		}else{
			APP_DBG("malloc I2S1_TX_FIFO error\n");
			return FALSE;
		}
		
		//I2SIn  digital (DMA)
		if(mainAppCt.I2S1_TX_FIFO  == NULL){
			return FALSE;
		}
		memset(mainAppCt.I2S1_TX_FIFO, 0, AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2);

#if CFG_PARA_I2S_SAMPLERATE == CFG_PARA_SAMPLE_RATE
		AudioIOSet.Adapt = STD;//SRC_ONLY
#else
		AudioIOSet.Adapt = SRC_ONLY;
#endif

		AudioIOSet.Sync = TRUE;//I2S slave 时候如果master没有接，有可能会导致DAC也不出声音。
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.HighLevelCent = 60;
		AudioIOSet.LowLevelCent = 40;
		AudioIOSet.SampleRate = CFG_PARA_I2S_SAMPLERATE;//根据实际外设选择
		//AudioIOSet.Depth = AudioCoreFrameSizeGet(DefaultNet) * 2 ;

		#if defined(TWS_IIS0_OUT) || defined(TWS_IIS1_OUT)
		AudioIOSet.Depth = TWS_SINK_DEV_FIFO_SAMPLES;
		AudioIOSet.Resident = TRUE;
		#else
		AudioIOSet.Depth = AudioCoreFrameSizeGet(DefaultNet) * 2 ;
		#endif
		AudioIOSet.DataIOFunc = AudioI2S1_DataSet;
		AudioIOSet.LenGetFunc = AudioI2S1_DataSpaceLenGet;

#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 0;//不需要做位宽转换处理
#endif
		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_I2S1_OUT_SINK_NUM))
		{
			DBG("I2S1 out init error");
			return FALSE;
		}

		AudioI2s1ParamsSet();
		AudioCoreSinkEnable(AUDIO_I2S1_OUT_SINK_NUM);
		AudioCoreSinkAdjust(AUDIO_I2S1_OUT_SINK_NUM, TRUE);
	}
	else//sam add,20230221
	{
		I2S_SampleRateSet(I2S1_MODULE, CFG_PARA_SAMPLE_RATE);
		#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 0;//DAC 24bit ,sink最后一级输出时需要转变为24bi
		AudioCore.AudioSink[AUDIO_I2S1_OUT_SINK_NUM].BitWidth = AudioIOSet.IOBitWidth;
		AudioCore.AudioSink[AUDIO_I2S1_OUT_SINK_NUM].BitWidthConvFlag = AudioIOSet.IOBitWidthConvFlag;
		AudioCore.AudioSink[AUDIO_I2S1_OUT_SINK_NUM].Sync = TRUE;
		#endif
	}	
#endif
}

bool I2sMixModeInit(void)
{
#ifdef CFG_RES_I2S0_EN
	I2s0MixModeAudioCoreInit();
#endif

#ifdef CFG_RES_I2S1_EN
	I2s1MixModeAudioCoreInit();
#endif
}

void I2sMixModeDeInit(void)
{
#ifdef CFG_RES_AUDIO_I2S0IN_EN
	if(AudioCoreSourceIsInit(I2S0_SOURCE_NUM))
	{
		AudioCoreSourceDisable(I2S0_SOURCE_NUM);
		AudioCoreSourceDeinit(I2S0_SOURCE_NUM);
	}
#endif

#ifdef CFG_RES_AUDIO_I2S1IN_EN
	if(AudioCoreSourceIsInit(I2S1_SOURCE_NUM))
	{
		AudioCoreSourceDisable(I2S1_SOURCE_NUM);
		AudioCoreSourceDeinit(I2S1_SOURCE_NUM);
	}
#endif	
}

#endif
