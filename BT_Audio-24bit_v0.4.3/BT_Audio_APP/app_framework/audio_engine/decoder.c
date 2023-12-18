/**
 **************************************************************************************
 * @file    decoder_service.c
 * @brief   
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2018-1-10 20:21:00$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <stdio.h>
#include <string.h>
#include "bt_manager.h"
#include "type.h"
#include "app_message.h"
#include "rtos_api.h"
#include "mvstdio.h"
#include "audio_decoder_api.h"
#include "audio_core_service.h"
#include "decoder.h"
#include "mcu_circular_buf.h"
#include "app_config.h"
#include "core_d1088.h"
#include "debug.h"
#include "main_task.h"
#include "ff.h"
#include "ffpresearch.h"
static volatile uint32_t  decode_mutex = 0;

static void DecoderProcess(DecoderChannels DecoderChannel);
static int16_t SaveDecodedPcmData(int16_t * PcmData,DecoderChannels DecoderChannel, uint16_t PcmDataLen);
//static void SetDecoderStateDeinit(DecoderChannels DecoderChannel);
static void DecoderStopProcess(DecoderChannels DecoderChannel);

AudioDecoderContext *gAudioDecoders[DECODER_MAX_CHANNELS]={NULL};

typedef struct _DecoderServiceContext
{

	MessageHandle		ParentMsgHandle;

	/*for audio decode init*/
//	int32_t				ioType;
	int32_t				decoderType;
	uint32_t			DecoderSize;

	/* for decoded out fifo */
	MCU_CIRCULAR_CONTEXT DecoderCircularBuf;
	int16_t				*DecoderFifo;
	uint16_t			DecoderFifoSize; //
	osMutexId			DecoderPcmFifoMutex;
	int8_t				*decoder_buf;
	uint8_t				Channel;//Decoderfifo内数据声道参数

	/* for save decoded PCM data, unit: Sample*/
	uint32_t			pcmDataSize;
	uint32_t			savedSize;
	int16_t				*toSavePos;

	DecoderState		decoderState;

	uint32_t			StepTime;				//快进快退

	/*for play timer*/
	uint32_t			DecoderSamples;			//当前尚未计入(指示)时间的采样点
	uint32_t			DecoderCurPlayTime;		//当前播放的时间
	uint32_t			DecoderLastPlayTime;

	uint8_t 			DecoderSourecNum;

	uint16_t			MiniInputLimit;		//流解码类型开启输入长度最小限制，memhandle条件下使用
	MemHandle * 		Handler;

	int32_t				StreamEmptyErr;		//解码类型对应的可持续型Empty消息值
	uint16_t			StreamEmptyCnt;  	//解码出错计数器；
//	uint16_t			ErrCntForMsg;		//停止解码前的计数值设定 //当前使用统一静态值。
}DecoderServiceContext;

static 	DecoderServiceContext 		DecoderServiceCt[DECODER_MAX_CHANNELS];


/**
 * @func        DecoderService_Init
 * @brief       DecoderService初始化
 * @param       MessageHandle   
 * @param		BufSize	Decoder buffer Size:4K @wav? 19K @mp3,40K @flac
 * @Output      None
 * @return      bool
 * @Others      解码输出流buf，开启锁机制。
 * Record
 */
 bool DecoderServiceInit(MessageHandle ParentMsgHandle, DecoderChannels DecoderChannel,uint32_t BufSize, uint16_t FifoSize)
{
	//DecoderServiceCt = (DecoderServiceContext)osPortMalloc(sizeof(DecoderServiceContext));
//	memset(&DecoderServiceCt[DecoderChannel], 0, sizeof(DecoderServiceContext));


	DecoderServiceCt[DecoderChannel].ParentMsgHandle = ParentMsgHandle;
	//DecoderServiceCt[DecoderChannel].serviceState = TaskStateCreating;

	DecoderServiceCt[DecoderChannel].DecoderSize = BufSize;
	DecoderServiceCt[DecoderChannel].DecoderFifoSize = FifoSize;

	if(!DecoderServiceCt[DecoderChannel].DecoderSize || DecoderServiceCt[DecoderChannel].DecoderFifoSize < DECODER_FIFO_SIZE_MIN)
	{
		return FALSE;
	}

	DecoderServiceCt[DecoderChannel].decoder_buf = (int8_t*)osPortMalloc(DecoderServiceCt[DecoderChannel].DecoderSize);
	if(DecoderServiceCt[DecoderChannel].decoder_buf == NULL)//DECODER_BUF_SIZE);
	{
		return FALSE;
	}
//	if((DecoderServiceCt[DecoderChannel].decoder_buf = (int8_t*)osPortMalloc(DecoderServiceCt[DecoderChannel].DecoderSize)) == NULL)//DECODER_BUF_SIZE);
//	{
//		return FALSE;
//	}
	memset(DecoderServiceCt[DecoderChannel].decoder_buf, 0, DecoderServiceCt[DecoderChannel].DecoderSize);
	
	if((DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex = xSemaphoreCreateMutex()) == NULL)
	{
		return FALSE;
	}

	DecoderServiceCt[DecoderChannel].DecoderFifo = (int16_t*)osPortMalloc(DecoderServiceCt[DecoderChannel].DecoderFifoSize);
	if(DecoderServiceCt[DecoderChannel].DecoderFifo == NULL)
	{
		return FALSE;
	}

	memset(DecoderServiceCt[DecoderChannel].DecoderFifo, 0, DecoderServiceCt[DecoderChannel].DecoderFifoSize);
	MCUCircular_Config(&DecoderServiceCt[DecoderChannel].DecoderCircularBuf, DecoderServiceCt[DecoderChannel].DecoderFifo, DecoderServiceCt[DecoderChannel].DecoderFifoSize);
//	DecoderServiceCt[DecoderChannel].serviceState = TaskStateReady;
	decode_mutex = 0;
	return TRUE;
}

void ModeDecodeProcess(void)
{
	if(decode_mutex)
	{
		return;
	}
	decode_mutex = 1;
	if(gAudioDecoders[DECODER_MODE_CHANNEL] != NULL)
	{
		DecoderProcess(DECODER_MODE_CHANNEL);
	}
	decode_mutex = 0;
}

#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY	
void RemindDecodeProcess(void)
{
	if(decode_mutex)
	{
		return;
	}
	decode_mutex = 1;
	if(gAudioDecoders[DECODER_REMIND_CHANNEL] != NULL)
	{
		DecoderProcess(DECODER_REMIND_CHANNEL);
	}
	decode_mutex = 0;
}
#endif
static void DecoderProcess(DecoderChannels DecoderChannel)
{
	int32_t DecoderErrCode;
	uint32_t SampleForRetry = CFG_PARA_MAX_SAMPLES_PER_FRAME;
	//static int32_t DecoderCnt = 0;
	do
		{
		switch(DecoderServiceCt[DecoderChannel].decoderState)
		{
			case DecoderStateInitialized:
				break;

			case DecoderStatePlay:
				{
					if(DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex == NULL)
						break;
					DecoderServiceCt[DecoderChannel].decoderState	= DecoderStateDecoding;
					MessageContext		msgSend;
					if(DecoderChannel == DECODER_MODE_CHANNEL)
					{
						msgSend.msgId		= MSG_DECODER_MODE_PLAYING;
					}
					else
					{
						msgSend.msgId		= MSG_DECODER_REMIND_PLAYING;
					}
					MessageSend(DecoderServiceCt[DecoderChannel].ParentMsgHandle, &msgSend);
					osMutexLock(DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex);
					MCUCircular_Config(&DecoderServiceCt[DecoderChannel].DecoderCircularBuf, DecoderServiceCt[DecoderChannel].DecoderFifo, DecoderServiceCt[DecoderChannel].DecoderFifoSize);
					AudioCoreSourcePcmFormatConfig(DecoderServiceCt[DecoderChannel].DecoderSourecNum, (int)gAudioDecoders[DecoderChannel]->song_info.num_channels);//配置audiocore
					DecoderServiceCt[DecoderChannel].Channel = gAudioDecoders[DecoderChannel]->song_info.num_channels;//配置本地fifo 声道数(init后固定不变)
					//AudioCoreSourceUnmute(DecoderSourecNum, TRUE, TRUE);//编码歌曲大多有淡入，sdk淡入时间短且早，效果不显。
					osMutexUnlock(DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex);
				}
				break;

			case DecoderStateDecoding:
				if(DecoderServiceCt[DecoderChannel].Handler != NULL
						&&  DecoderServiceCt[DecoderChannel].MiniInputLimit != 0
						&& mv_msize(DecoderServiceCt[DecoderChannel].Handler) <= DecoderServiceCt[DecoderChannel].MiniInputLimit)
				{
					return;
				}

				if(audio_decoder_decode(gAudioDecoders[DecoderChannel]) != RT_SUCCESS)
				{
					if(RT_SUCCESS != audio_decoder_can_continue(gAudioDecoders[DecoderChannel]))
					{//解码操作结果，非流数据不足，或致命错误；流数据可恢复类型错误此api不处理。
						MessageContext		msgSend;
						DecoderServiceCt[DecoderChannel].decoderState = DecoderStatePause;

						msgSend.msgId		= MSG_DECODER_SERVICE_SONG_PLAY_END;
						MessageSend(DecoderServiceCt[DecoderChannel].ParentMsgHandle, &msgSend);
						return;
					}

					DecoderErrCode = audio_decoder_get_error_code(gAudioDecoders[DecoderChannel]);
					

					if(DecoderErrCode != 0 && DecoderServiceCt[DecoderChannel].StreamEmptyErr == DecoderErrCode)
					{
						MessageContext		msgSend;

						if(++(DecoderServiceCt[DecoderChannel].StreamEmptyCnt) > MUTLI_EMPTY_FOR_MSG)
						{
							DecoderServiceCt[DecoderChannel].StreamEmptyCnt = 0;
							DecoderServiceCt[DecoderChannel].decoderState = DecoderStatePause;
						#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
							if(DecoderChannel == DECODER_REMIND_CHANNEL)
							{
								extern void RemindSoundAudioDecoderStop(void);
								RemindSoundAudioDecoderStop();
							}
							else
						#endif
							{
								msgSend.msgId = MSG_DECODER_SERVICE_MUTLI_EMPTY;
								MessageSend(DecoderServiceCt[DecoderChannel].ParentMsgHandle, &msgSend);	
							}
							APP_DBG("END SONG FOR DECODER TIMEOUT\n");
						}
					}

					return;
				}
				else if(DecoderServiceCt[DecoderChannel].StreamEmptyCnt != 0)
				{
					APP_DBG("FIFO_EMPTY Cnt = %d\n",(int)DecoderServiceCt[DecoderChannel].StreamEmptyCnt);
				}
				DecoderServiceCt[DecoderChannel].StreamEmptyCnt = 0;

				//计算播放时间
				{
					uint32_t Temp;
					DecoderServiceCt[DecoderChannel].DecoderSamples += gAudioDecoders[DecoderChannel]->song_info.pcm_data_length;
					Temp = DecoderServiceCt[DecoderChannel].DecoderSamples / gAudioDecoders[DecoderChannel]->song_info.sampling_rate;
					DecoderServiceCt[DecoderChannel].DecoderSamples -= Temp * gAudioDecoders[DecoderChannel]->song_info.sampling_rate;
					DecoderServiceCt[DecoderChannel].DecoderCurPlayTime += Temp;
					if((DecoderServiceCt[DecoderChannel].DecoderCurPlayTime ) != (DecoderServiceCt[DecoderChannel].DecoderLastPlayTime))
					{
						MessageContext		msgSend;

						msgSend.msgId		= MSG_DECODER_SERVICE_UPDATA_PLAY_TIME;
						MessageSend(DecoderServiceCt[DecoderChannel].ParentMsgHandle, &msgSend);

						//DecoderServiceCt[DecoderChannel].DecoderLastPlayTime = DecoderServiceCt[DecoderChannel].DecoderCurPlayTime;
					}
					DecoderServiceCt[DecoderChannel].DecoderLastPlayTime = DecoderServiceCt[DecoderChannel].DecoderCurPlayTime;
				}
				DecoderServiceCt[DecoderChannel].decoderState = DecoderStateToSavePcmData;
			case DecoderStateToSavePcmData:
				{
					DecoderServiceCt[DecoderChannel].pcmDataSize	= gAudioDecoders[DecoderChannel]->song_info.pcm_data_length * DecoderServiceCt[DecoderChannel].Channel;
					DecoderServiceCt[DecoderChannel].savedSize		= 0;
					DecoderServiceCt[DecoderChannel].toSavePos		= (int16_t *)gAudioDecoders[DecoderChannel]->song_info.pcm_addr;
					if(DecoderServiceCt[DecoderChannel].Channel != gAudioDecoders[DecoderChannel]->song_info.num_channels)//针对特殊歌曲，声道数可变
					{
						int16_t  i;
						if(gAudioDecoders[DecoderChannel]->song_info.num_channels == 2)//DecoderServiceCt[DecoderChannel].Channel == 1 双合并为单
						{
							for(i = 0; i < gAudioDecoders[DecoderChannel]->song_info.pcm_data_length; i++)
							{
								DecoderServiceCt[DecoderChannel].toSavePos[i] = (((int32_t)((DecoderServiceCt[DecoderChannel].toSavePos)[2 * i]) + ((DecoderServiceCt[DecoderChannel].toSavePos)[2 * i + 1])) / 2);
							}
						}
						else//单扩为双
						{
							for(i = gAudioDecoders[DecoderChannel]->song_info.pcm_data_length - 1; i >= 0; i--)
							{
								DecoderServiceCt[DecoderChannel].toSavePos[2 * i + 1] = (DecoderServiceCt[DecoderChannel].toSavePos)[i];
								DecoderServiceCt[DecoderChannel].toSavePos[2 * i] = (DecoderServiceCt[DecoderChannel].toSavePos)[i];
							}
						}
					}
					DecoderServiceCt[DecoderChannel].decoderState = DecoderStateSavePcmData;
					if(DecoderServiceCt[DecoderChannel].pcmDataSize == 0)//add by sam, 20180814，增加一个容错处理，有客户歌曲，解码出来，length为0
					{
						DecoderServiceCt[DecoderChannel].decoderState = DecoderStateDecoding;
						SampleForRetry -= 1;//避免死机,反复解出0个数据时退出。
						break;
					}
				}

			case DecoderStateSavePcmData:
				{
					int32_t		savedSize;  //采样点值

					savedSize = SaveDecodedPcmData(DecoderServiceCt[DecoderChannel].toSavePos, DecoderChannel,
								(DecoderServiceCt[DecoderChannel].pcmDataSize - DecoderServiceCt[DecoderChannel].savedSize) / DecoderServiceCt[DecoderChannel].Channel);

					if(savedSize > 0)
					{

			
						MessageContext		msgSend;
						msgSend.msgId		= MSG_NONE;
						MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);
			
						DecoderServiceCt[DecoderChannel].savedSize += savedSize * DecoderServiceCt[DecoderChannel].Channel;
						DecoderServiceCt[DecoderChannel].toSavePos += savedSize * DecoderServiceCt[DecoderChannel].Channel;
						if(DecoderServiceCt[DecoderChannel].savedSize == DecoderServiceCt[DecoderChannel].pcmDataSize) //上次解码输出已全部 放入fifo
						{
							DecoderServiceCt[DecoderChannel].decoderState = DecoderStateDecoding;
							if(SampleForRetry > savedSize)
							{
								SampleForRetry -= savedSize;
								break;//避免单次解码数据太少，引起OS切换后断音，流和buf可用时，再次解码。
							}
						}
					}
					SampleForRetry = 0;
				}
				break;

			case DecoderStateDeinitializing:
				DecoderServiceCt[DecoderChannel].decoderState = DecoderStateNone;
				break;

			case DecoderStateStop:
				DecoderServiceCt[DecoderChannel].StreamEmptyCnt = 0;
				break;

			case DecoderStatePause:
				//APP_DBG("Decoder play Pause\n");
				//DecoderServiceCt[DecoderChannel].decoderState = DecoderStateNone;
				break;

			default:
				break;
		}
	}while(SampleForRetry && (DecoderServiceCt[DecoderChannel].decoderState == DecoderStateSavePcmData || DecoderServiceCt[DecoderChannel].decoderState == DecoderStateDecoding));
}


static int16_t SaveDecodedPcmData(int16_t * PcmData,DecoderChannels DecoderChannel, uint16_t PcmDataLen)
{
	uint32_t	ProcessSampleLen, SpaceSampleLen;

	if(PcmData == NULL || PcmDataLen == 0 || DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex == NULL)
	{
		return -1;
	}
	osMutexLock(DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex);

	SpaceSampleLen = MCUCircular_GetSpaceLen(&DecoderServiceCt[DecoderChannel].DecoderCircularBuf)/(DecoderServiceCt[DecoderChannel].Channel * 2) - 1;

	ProcessSampleLen = SpaceSampleLen < PcmDataLen ? SpaceSampleLen : PcmDataLen;

	MCUCircular_PutData(&DecoderServiceCt[DecoderChannel].DecoderCircularBuf, PcmData, ProcessSampleLen * DecoderServiceCt[DecoderChannel].Channel * 2);

	osMutexUnlock(DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex);
	return ProcessSampleLen;
}

static void DecoderStopProcess(DecoderChannels DecoderChannel)
{
	APP_DBG("Decoder:play stop\n");
	DecoderServiceCt[DecoderChannel].DecoderLastPlayTime = 0;
	DecoderServiceCt[DecoderChannel].DecoderCurPlayTime = 0;
	DecoderServiceCt[DecoderChannel].DecoderSamples = 0;
	DecoderServiceCt[DecoderChannel].decoderState = DecoderStateNone;
}

/*
static void DecoderServicePause(DecoderChannels DecoderChannel)
{
	DecoderServiceCt[DecoderChannel].decoderState = DecoderStateNone;
}
*/
void DecoderServiceDeinit(DecoderChannels DecoderChannel)
{

	DecoderServiceCt[DecoderChannel].decoderState = DecoderStateNone;
	
	if(DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex != NULL)
	{
		osMutexLock(DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex);
	}
	
	if(DecoderServiceCt[DecoderChannel].DecoderFifo != NULL)
	{
		osPortFree(DecoderServiceCt[DecoderChannel].DecoderFifo);
		DecoderServiceCt[DecoderChannel].DecoderFifo = NULL;
	}

	if(DecoderServiceCt[DecoderChannel].decoder_buf != NULL)
	{
		osPortFree(DecoderServiceCt[DecoderChannel].decoder_buf);
		DecoderServiceCt[DecoderChannel].decoder_buf = NULL;
	}
	
	if(DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex != NULL)
	{
		osMutexUnlock(DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex);
	}

	if(DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex != NULL)
	{
		vSemaphoreDelete(DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex);
		DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex = NULL;
	}

	gAudioDecoders[DecoderChannel] =NULL;


}

void DecoderOslockUnlock(void)
{
	uint32_t decoder_index = 0;
	for(;decoder_index < DECODER_MAX_CHANNELS;decoder_index++)
	{
		if(DecoderServiceCt[decoder_index].DecoderPcmFifoMutex != NULL)
		{
			osMutexUnlock(DecoderServiceCt[decoder_index].DecoderPcmFifoMutex);
			vSemaphoreDelete(DecoderServiceCt[decoder_index].DecoderPcmFifoMutex);
			DecoderServiceCt[decoder_index].DecoderPcmFifoMutex = NULL;
		}
	}
}

int32_t DecoderInit(void *io_handle, DecoderChannels DecoderChannel, int32_t ioType, int32_t decoderType)
{
		int32_t 	ret;//= DECODER_NORMAL;
		if(DecoderServiceCt[DecoderChannel].decoderState != DecoderStateNone && DecoderServiceCt[DecoderChannel].decoderState != DecoderStateInitialized)
		{
			APP_DBG("Decoder State Error :%d \n", DecoderServiceCt[DecoderChannel].decoderState);
			return RT_FAILURE;
		}
		
		gAudioDecoders[DecoderChannel] = (AudioDecoderContext*)DecoderServiceCt[DecoderChannel].decoder_buf;//bkd
		
		ret = audio_decoder_initialize((uint8_t*)DecoderServiceCt[DecoderChannel].decoder_buf, io_handle, ioType, decoderType);//gAudioDecoders[DecoderChannel]
		if(ret != RT_SUCCESS)
		{
			int32_t 	errCode;
			errCode = audio_decoder_get_error_code(gAudioDecoders[DecoderChannel]);
			APP_DBG("AudioDecoder init errCode = %d\n", (int)errCode);
#ifdef USE_DTS_DECODER
			if(decoderType == WAV_DECODER && errCode == -245)//WAV_ERROR_UNSUPPORTED_DTS_FORMAT,       /**< Unsupported DTS stream format  */
			{
				ret = audio_decoder_initialize((uint8_t*)DecoderServiceCt[DecoderChannel].decoder_buf, io_handle, ioType, DTS_DECODER);
			}
			else
#endif
			if(ioType == IO_TYPE_FILE)
			{
				uint8_t FileHead[10];
				UINT Len;
				DecoderType Guess;
				f_lseek(io_handle, 0);
				f_read(io_handle, FileHead, 10, &Len);
				Guess = audio_decoder_guess_format(FileHead);
				if(Guess != UNKNOWN_DECODER && Guess != decoderType)
				{
					ret = audio_decoder_initialize((uint8_t*)DecoderServiceCt[DecoderChannel].decoder_buf, io_handle, ioType, Guess);
				}
			}
		}
		if(audio_decoder_get_memory_size(decoderType) >= DecoderServiceCt[DecoderChannel].DecoderSize)
		{
			APP_DBG("Decodersize set error !!!!!!!");
			return RT_FAILURE;//安全监测
		}
		
		decode_mutex = 0; 
		
		if(ret == RT_SUCCESS)
		{
			DecoderServiceCt[DecoderChannel].decoderState = DecoderStateInitialized;
			APP_DBG("[SONG_INFO]: ChannelCnt : %6d\n",		  (int)gAudioDecoders[DecoderChannel]->song_info.num_channels);
			APP_DBG("[SONG_INFO]: SampleRate : %6d Hz\n",	  (int)gAudioDecoders[DecoderChannel]->song_info.sampling_rate);
#if defined(BT_TWS_SUPPORT)&&(CFG_TWS_ONLY_IN_BT_MODE == DISABLE)
			if(gAudioDecoders[DecoderChannel]->song_info.stream_type == 8 && (*((uint16_t*)(gAudioDecoders[DecoderChannel]->frame)+24)) >= 3000)
			{
				APP_DBG("[SONG_INFO]: song_info.stream_type : %ld  APE TYPE: %d \n",gAudioDecoders[DecoderChannel]->song_info.stream_type, *((uint16_t*)(gAudioDecoders[DecoderChannel]->frame)+24));
				return RT_FAILURE;
			}
			//APP_DBG("[SONG_INFO]: BitRate	 : %6d Kbps\n",   (int)gAudioDecoders[DecoderChannel]->song_info.bitrate / 1000);
			//APP_DBG("[SONG_INFO]: DecoderSize: %6d Bytes \n", (int)gAudioDecoders[DecoderChannel]->decoder_size);
			if(gAudioDecoders[DecoderChannel]->song_info.stream_type != STREAM_TYPE_FLAC && gAudioDecoders[DecoderChannel]->song_info.sampling_rate > 48000)
			{
				return RT_FAILURE;//读取数据速度受限，屏蔽部分高码率歌曲。
			}
#endif			
#ifdef CFG_FUNC_MIXER_SRC_EN
			if(DecoderChannel == DECODER_MODE_CHANNEL)
			{
				//需要先播空
				AudioCoreSourceChange(APP_SOURCE_NUM, gAudioDecoders[DecoderChannel]->song_info.num_channels, gAudioDecoders[DecoderChannel]->song_info.sampling_rate);
			}
#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
			else if(DecoderChannel == DECODER_REMIND_CHANNEL)
			{
				AudioCoreSourceChange(REMIND_SOURCE_NUM, gAudioDecoders[DecoderChannel]->song_info.num_channels, gAudioDecoders[DecoderChannel]->song_info.sampling_rate);
			}	
#endif			
#endif

			DecoderServiceCt[DecoderChannel].MiniInputLimit = 0;
			DecoderServiceCt[DecoderChannel].Handler = NULL;
			DecoderServiceCt[DecoderChannel].StreamEmptyCnt = 0;
			DecoderServiceCt[DecoderChannel].StreamEmptyErr = 0;
			switch(decoderType)
			{
				case MSBC_DECODER:
					if(ioType == IO_TYPE_MEMORY)
					{
						DecoderServiceCt[DecoderChannel].MiniInputLimit = MSBC_DECODER_FIFO_MIN;
						DecoderServiceCt[DecoderChannel].Handler = (MemHandle *)io_handle;
					}
					DecoderServiceCt[DecoderChannel].StreamEmptyErr = SBC_ERROR_STREAM_EMPTY;
					break;
				case SBC_DECODER:
					if(ioType == IO_TYPE_MEMORY)
					{
						DecoderServiceCt[DecoderChannel].MiniInputLimit = SBC_DECODER_FIFO_MIN;
						DecoderServiceCt[DecoderChannel].Handler = (MemHandle *)io_handle;
					}
					DecoderServiceCt[DecoderChannel].StreamEmptyErr = SBC_ERROR_STREAM_EMPTY;
					break;
				case AAC_DECODER:
					if(ioType == IO_TYPE_MEMORY)
					{
						DecoderServiceCt[DecoderChannel].MiniInputLimit = AAC_DECODER_FIFO_MIN;
						DecoderServiceCt[DecoderChannel].Handler = (MemHandle *)io_handle;
					}
					DecoderServiceCt[DecoderChannel].StreamEmptyErr = AAC_ERROR_STREAM_EMPTY;
					break;
				case MP3_DECODER:
					DecoderServiceCt[DecoderChannel].StreamEmptyErr = MPX_ERROR_STREAM_EMPTY;
					break;
				case WAV_DECODER:
					DecoderServiceCt[DecoderChannel].StreamEmptyErr = WAV_ERROR_STREAM_EMPTY;
					break;
				default:
					break;
			}

		}
		else
		{
			int32_t 	errCode;
			errCode = audio_decoder_get_error_code(gAudioDecoders[DecoderChannel]);
			APP_DBG("AudioDecoder init err code = %d\n", (int)errCode);
		}
		return ret;
}
/*
static void SetDecoderStateDeinit(DecoderChannels DecoderChannel)
{
	DecoderServiceCt[DecoderChannel].decoderState = DecoderStateDeinitializing;
}

*/
/**
 * @func        DecoderPcmDataGet
 * @brief       Decoder数据流输出的 拉流API
 * @param       void * pcmData
                uint16_t sampleLen  
 * @Output      None
 * @return      uint16_t 取出的采样点数
 * @Others      数据区需要锁定，指定采样点数取出数据，不足时，按实际长度取数据。
 * Record
 */
static uint16_t DecoderPcmDataGet(void * pcmData,DecoderChannels DecoderChannel,uint16_t sampleLen)
{
	int16_t		dataSize;
	uint32_t	getSize;
	uint32_t 	SpacLen = 0;
	uint32_t 	ValLen = 0;

	if(DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex == NULL)
	{
		return 0;
	}
	if(DecoderServiceCt[DecoderChannel].decoderState <= DecoderStateDeinitializing)
	{
		return 0;
	}
	getSize = sampleLen * 2 * DecoderServiceCt[DecoderChannel].Channel;//采样点数 * 2byte深度* 通道数
	if(getSize == 0)
	{
		return 0;
	}
	osMutexLock(DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex);
	SpacLen = MCUCircular_GetSpaceLen(&DecoderServiceCt[DecoderChannel].DecoderCircularBuf);

	ValLen = DecoderServiceCt[DecoderChannel].DecoderFifoSize - SpacLen;

	if(getSize > ValLen)
	{
		getSize = ValLen;
	}

	dataSize = MCUCircular_GetData(&DecoderServiceCt[DecoderChannel].DecoderCircularBuf, pcmData, getSize);//bkd

	osMutexUnlock(DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex);

	return dataSize / (2 * DecoderServiceCt[DecoderChannel].Channel);
}

static uint16_t DecoderPcmDataLenGet(DecoderChannels DecoderChannel)
{
	if(DecoderServiceCt[DecoderChannel].DecoderPcmFifoMutex == NULL || DecoderServiceCt[DecoderChannel].decoderState <= DecoderStateDeinitializing)//|| DecoderServiceCt[DecoderChannel].serviceState != TaskStateRunning
	{
		return 0;
	}
	return MCUCircular_GetDataLen(&DecoderServiceCt[DecoderChannel].DecoderCircularBuf) / (2 * DecoderServiceCt[DecoderChannel].Channel);
}

uint16_t ModeDecoderPcmDataGet(void * pcmData,uint16_t sampleLen)// call by audio core one by one
{
	return DecoderPcmDataGet(pcmData,DECODER_MODE_CHANNEL,sampleLen);
}
uint16_t ModeDecoderPcmDataLenGet(void)
{
	return DecoderPcmDataLenGet(DECODER_MODE_CHANNEL);
}

#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
uint16_t RemindDecoderPcmDataGet(void * pcmData,uint16_t sampleLen)// call by audio core one by one
{
	return DecoderPcmDataGet(pcmData,DECODER_REMIND_CHANNEL,sampleLen);
}

uint16_t RemindDecoderPcmDataLenGet(void)
{
	return DecoderPcmDataLenGet(DECODER_REMIND_CHANNEL);
}

SongInfo * GetSongInfo(DecoderChannels DecoderChannel)//
{
	return &(gAudioDecoders[DecoderChannel]->song_info);
}
#endif

//解码器工作
void DecoderPlay(DecoderChannels DecoderChannel)
{
	if(DecoderServiceCt[DecoderChannel].decoderState == DecoderStateInitialized)
	{
		DecoderServiceCt[DecoderChannel].decoderState = DecoderStatePlay;
	}
}

//解码器停止
void DecoderStop(DecoderChannels DecoderChannel)
{
	DecoderServiceCt[DecoderChannel].decoderState = DecoderStateStop;
	DecoderStopProcess(DecoderChannel);
}

//解码器停止
void DecoderMuteAndStop(DecoderChannels DecoderChannel)
{
	AudioCoreSourceMute(DecoderServiceCt[DecoderChannel].DecoderSourecNum, TRUE, TRUE);
	DecoderStop(DecoderChannel);
}

void DecoderServiceStop(DecoderChannels DecoderChannel)
{
	APP_DBG("MSG_TASK_STOP\n");
	DecoderServiceCt[DecoderChannel].decoderState = DecoderStateNone;
}

void DecoderPause(DecoderChannels DecoderChannel)
{
	if(DecoderServiceCt[DecoderChannel].decoderState <= DecoderStatePause)
	{
		return;
	}
	DecoderServiceCt[DecoderChannel].decoderState = DecoderStatePause;
}

void DecoderResume(DecoderChannels DecoderChannel)
{
	APP_DBG("Decoder Resume()\n");
	if(DecoderServiceCt[DecoderChannel].decoderState != DecoderStateNone)
	{
		DecoderServiceCt[DecoderChannel].decoderState = DecoderStatePlay;
	}
}

//注意，StepTime 单位为毫秒，实际传递值为整数秒
void DecoderFF(uint32_t StepTime,DecoderChannels DecoderChannel)
{
	uint32_t Timer = NULL;
	MessageContext msgSend;
	
	APP_DBG("Decoder FF()\n");
	APP_DBG("Decoder state = %d\n", DecoderServiceCt[DecoderChannel].decoderState);
	if(DecoderServiceCt[DecoderChannel].decoderState <= DecoderStatePause)//Playing
	{
		return;
	}
	DecoderServiceCt[DecoderChannel].StepTime = StepTime;
	Timer = DecoderServiceCt[DecoderChannel].DecoderCurPlayTime * 1000 + DecoderServiceCt[DecoderChannel].StepTime;
	APP_DBG("PlayTime = %d\n", (int)Timer);
	if(Timer > gAudioDecoders[DecoderChannel]->song_info.duration)
	{
		DecoderServiceCt[DecoderChannel].decoderState = DecoderStatePause;
		APP_DBG("SONG FF END; Play next song.\n");
		msgSend.msgId		= MSG_NEXT;
		MessageSend(GetSysModeMsgHandle(), &msgSend);
		return;
	}
	audio_decoder_seek(gAudioDecoders[DecoderChannel],Timer);
	DecoderServiceCt[DecoderChannel].DecoderSamples = 0;
	DecoderServiceCt[DecoderChannel].DecoderCurPlayTime += DecoderServiceCt[DecoderChannel].StepTime / 1000;

}

//注意，StepTime 单位为毫秒，实际传递值为整数秒
void DecoderFB(uint32_t StepTime,DecoderChannels DecoderChannel)
{
	uint32_t Timer = 0;
	MessageContext msgSend;

	APP_DBG("Decoder FB()\n");
	if(DecoderServiceCt[DecoderChannel].decoderState <= DecoderStatePause)//Playing
	{
		return;
	}
	DecoderServiceCt[DecoderChannel].StepTime = StepTime;
	if(DecoderServiceCt[DecoderChannel].DecoderCurPlayTime * 1000 == 0)
	{
		//DecoderServiceCt[DecoderChannel].decoderState = DecoderStatePause;
		APP_DBG("backward to 0, then pause the player\n");
		msgSend.msgId		= MSG_PLAY_PAUSE;
		MessageSend(GetSysModeMsgHandle(), &msgSend);
		return;
	} 
	else if(DecoderServiceCt[DecoderChannel].DecoderCurPlayTime * 1000 < DecoderServiceCt[DecoderChannel].StepTime)
	{
		DecoderServiceCt[DecoderChannel].DecoderCurPlayTime = 0;
	}
	else
	{
		Timer = DecoderServiceCt[DecoderChannel].DecoderCurPlayTime * 1000 - DecoderServiceCt[DecoderChannel].StepTime;
		audio_decoder_seek(gAudioDecoders[DecoderChannel],Timer);
		DecoderServiceCt[DecoderChannel].DecoderSamples = 0;
		DecoderServiceCt[DecoderChannel].DecoderCurPlayTime -= DecoderServiceCt[DecoderChannel].StepTime / 1000;
	}

}

//解码器seek 文件播放时间 秒，因不同格式情况，精度不定。
bool DecoderSeek(uint32_t Time,DecoderChannels DecoderChannel)
{
	APP_DBG("DecoderTaskSeek()\n");
	audio_decoder_seek(gAudioDecoders[DecoderChannel],Time * 1000);
	DecoderServiceCt[DecoderChannel].DecoderSamples = 0;
	DecoderServiceCt[DecoderChannel].DecoderCurPlayTime = 0;
	if(audio_decoder_seek(gAudioDecoders[DecoderChannel],Time * 1000) != RT_SUCCESS)
	{
		return FALSE;
	}
	DecoderServiceCt[DecoderChannel].DecoderCurPlayTime = Time;
	return TRUE;
}

uint32_t DecoderServicePlayTimeGet(DecoderChannels DecoderChannel)
{
 	return DecoderServiceCt[DecoderChannel].DecoderCurPlayTime;
}

uint32_t GetDecoderState(DecoderChannels DecoderChannel)
{
	return DecoderServiceCt[DecoderChannel].decoderState;
}

//此变量作为全局获取，不要为空。
uint8_t DecoderSourceNumGet(DecoderChannels DecoderChannel)
{
	return DecoderServiceCt[DecoderChannel].DecoderSourecNum;
}

void DecoderSourceNumSet(uint8_t Num,DecoderChannels DecoderChannel)
{
	DecoderServiceCt[DecoderChannel].DecoderSourecNum = Num;
}

#ifdef BT_AUDIO_AAC_ENABLE
AudioDecoderContext *audio_decoder;
static bool 	BtDecoderFlag = 0;
static uint32_t BtDecoderErrCnt = 0;

void BtDecoderDeinit(void)
{
	BtDecoderFlag = 0;
	BtDecoderErrCnt = 0;
	btManager.aacFrameNumber = 0;//AAC
}

uint8_t GetBtDecoderFlag(void)
{
	return BtDecoderFlag;
}

void BtDecodedPcmData(bool init)
{
	static uint8_t state = 0;

	uint16_t SampleForRetry = CFG_PARA_MAX_SAMPLES_PER_FRAME;//单次解码的目标解码采样点数量。

	if(init)
	{
		state = 0;
		BtDecoderErrCnt = 0;
	}

	if(!BtDecoderFlag)
		return;

	do
	{
	switch(state)
	{
		default:
			state = 0;
		case 0:
			if(BtDecoderErrCnt > 100)
			{
				MessageContext		msgSend;
				msgSend.msgId = MSG_DECODER_SERVICE_DISK_ERROR;
				MessageSend(GetSysModeMsgHandle(), &msgSend);
				return;
			}
			if(RT_SUCCESS != audio_decoder_can_continue(audio_decoder))
			{
				//APP_DBG("BtDecoder Unable to continue!!!  %d\n",BtDecoderErrCnt);
				if(BtDecoderErrCnt > 0)
					BtDecoderErrCnt++;
				return;
			}

			uint8_t index = btManager.btLinked_env[btManager.cur_index].a2dp_index;
			if((GetSystemMode() == ModeBtAudioPlay)
				&& (index < BT_LINK_DEV_NUM)
				&& (btManager.a2dpStreamType[index] == BT_A2DP_STREAM_TYPE_AAC))
			{
				//aac
				if(btManager.aacFrameNumber)
					btManager.aacFrameNumber--;
			}

			if(audio_decoder_decode(audio_decoder) == RT_SUCCESS)
			{
				DecoderServiceCt[DECODER_MODE_CHANNEL].pcmDataSize	= audio_decoder->song_info.pcm_data_length * DecoderServiceCt[DECODER_MODE_CHANNEL].Channel;
				DecoderServiceCt[DECODER_MODE_CHANNEL].savedSize		= 0;
				DecoderServiceCt[DECODER_MODE_CHANNEL].toSavePos		= (int16_t *)audio_decoder->song_info.pcm_addr;
				state = 1;
				BtDecoderErrCnt = 0;
			}
			else
			{
				int32_t 	errCode;
				errCode = audio_decoder_get_error_code(audio_decoder);
				if(errCode != DecoderServiceCt[DECODER_MODE_CHANNEL].StreamEmptyErr)
				{
					APP_DBG("BtDecoder err code = %d\n", (int)errCode);
					
#ifdef CFG_DUMP_DEBUG_EN
					extern uint8_t gDumpProcessStop;
					gDumpProcessStop = 1;
#endif
				}
				SampleForRetry = 0;
				BtDecoderErrCnt++;
				break;
			}
		case 1:
			while(1)
			{
				int32_t		savedSize;  //采样点值
				savedSize = SaveDecodedPcmData(DecoderServiceCt[DECODER_MODE_CHANNEL].toSavePos,DECODER_MODE_CHANNEL,
						(DecoderServiceCt[DECODER_MODE_CHANNEL].pcmDataSize - DecoderServiceCt[DECODER_MODE_CHANNEL].savedSize) / DecoderServiceCt[DECODER_MODE_CHANNEL].Channel);
				if(savedSize > 0)
				{
					DecoderServiceCt[DECODER_MODE_CHANNEL].savedSize += savedSize * DecoderServiceCt[DECODER_MODE_CHANNEL].Channel;
					DecoderServiceCt[DECODER_MODE_CHANNEL].toSavePos += savedSize * DecoderServiceCt[DECODER_MODE_CHANNEL].Channel;
					if(SampleForRetry > savedSize)
					{
						SampleForRetry -= savedSize;
					}
					else
						SampleForRetry = 0;
					if(DecoderServiceCt[DECODER_MODE_CHANNEL].savedSize == DecoderServiceCt[DECODER_MODE_CHANNEL].pcmDataSize) //上次解码输出已全部 放入fifo
					{
						state = 0;
						break;
					}
				}
				else
				{
					SampleForRetry = 0;
					break;
				}
			}
			break;
	}
	}while(SampleForRetry);
}

int32_t BtDecoderInit(void *io_handle,int32_t decoder_type)
{
	int32_t 	ret;//= DECODER_NORMAL;
	int32_t 	errCode;

	audio_decoder = (AudioDecoderContext*)DecoderServiceCt[DECODER_MODE_CHANNEL].decoder_buf;
	ret = audio_decoder_initialize((uint8_t*)DecoderServiceCt[DECODER_MODE_CHANNEL].decoder_buf, io_handle, (int32_t)IO_TYPE_MEMORY, decoder_type);

	if(audio_decoder_get_memory_size(decoder_type) >= DecoderServiceCt[DECODER_MODE_CHANNEL].DecoderSize)
	{
		APP_DBG("Decodersize set error!!!!!!!");
		return RT_FAILURE;//安全监测
	}
	if(ret == RT_SUCCESS)
	{
		APP_DBG("[SONG_INFO]: ChannelCnt : %6d\n",		  (int)audio_decoder->song_info.num_channels);
		APP_DBG("[SONG_INFO]: SampleRate : %6d Hz\n",	  (int)audio_decoder->song_info.sampling_rate);

		MCUCircular_Config(&DecoderServiceCt[DECODER_MODE_CHANNEL].DecoderCircularBuf, DecoderServiceCt[DECODER_MODE_CHANNEL].DecoderFifo, DecoderServiceCt[DECODER_MODE_CHANNEL].DecoderFifoSize);
		DecoderServiceCt[DECODER_MODE_CHANNEL].Channel = audio_decoder->song_info.num_channels;//配置本地fifo 声道数（init后固定不变）
		//if (audio_decoder->song_info.sampling_rate != CFG_PARA_SAMPLE_RATE)
		{
			AudioCoreSourceChange(APP_SOURCE_NUM, audio_decoder->song_info.num_channels, audio_decoder->song_info.sampling_rate);
		}
		BtDecoderFlag = 1;
		BtDecodedPcmData(1);
		if(decoder_type == SBC_DECODER)
			DecoderServiceCt[DECODER_MODE_CHANNEL].StreamEmptyErr = SBC_ERROR_STREAM_EMPTY;
		else if(decoder_type == AAC_DECODER)
			DecoderServiceCt[DECODER_MODE_CHANNEL].StreamEmptyErr = AAC_ERROR_STREAM_EMPTY;
		else
			DecoderServiceCt[DECODER_MODE_CHANNEL].StreamEmptyErr = 0;
	}
	else
	{
		//int32_t 	errCode;
		errCode = audio_decoder_get_error_code(audio_decoder);
		APP_DBG("BtAACDecoderInit init err code = %d\n", (int)errCode);
	}
	return ret;
}

uint16_t BtDecodedPcmDataGet(void * pcmData, uint16_t sampleLen)
{
	int16_t		dataSize;
	uint32_t	getSize;
	uint32_t 	SpacLen = 0;
	uint32_t 	ValLen = 0;

	//BtDecodedPcmData(0);
	getSize = sampleLen * 2 * DecoderServiceCt[DECODER_MODE_CHANNEL].Channel;//采样点数 * 2byte深度* 通道数
	if(getSize == 0)
	{
		return 0;
	}

	osMutexLock(DecoderServiceCt[DECODER_MODE_CHANNEL].DecoderPcmFifoMutex);
	SpacLen = MCUCircular_GetSpaceLen(&DecoderServiceCt[DECODER_MODE_CHANNEL].DecoderCircularBuf);

	ValLen = DecoderServiceCt[DECODER_MODE_CHANNEL].DecoderFifoSize - SpacLen;

	if(getSize > ValLen)
	{
		getSize = ValLen;
	}

	dataSize = MCUCircular_GetData(&DecoderServiceCt[DECODER_MODE_CHANNEL].DecoderCircularBuf, pcmData, getSize);
	osMutexUnlock(DecoderServiceCt[DECODER_MODE_CHANNEL].DecoderPcmFifoMutex);

	return dataSize / (2 * DecoderServiceCt[DECODER_MODE_CHANNEL].Channel);
}

uint16_t BtDecodedPcmDataLenGet(void)
{
	BtDecodedPcmData(0);
	return MCUCircular_GetDataLen(&DecoderServiceCt[DECODER_MODE_CHANNEL].DecoderCircularBuf) / (2 * DecoderServiceCt[DECODER_MODE_CHANNEL].Channel);
}
#endif



