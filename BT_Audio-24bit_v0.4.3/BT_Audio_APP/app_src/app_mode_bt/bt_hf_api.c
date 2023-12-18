/**
 **************************************************************************************
 * @file    bt_hf_api.c
 * @brief   蓝牙通话模式
 *
 * @author  KK
 * @version V1.0.1
 *
 * $Created: 2019-3-28 18:00:00$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include "type.h"
#include "app_config.h"
#include "app_message.h"
#include "mvintrinsics.h"
//driver
#include "chip_info.h"
#include "dac.h"
#include "gpio.h"
#include "dma.h"
#include "dac.h"
#include "audio_adc.h"
#include "debug.h"
//middleware
#include "main_task.h"
#include "audio_vol.h"
#include "rtos_api.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "bt_manager.h"
#include "resampler.h"
#include "mcu_circular_buf.h"
#include "audio_core_api.h"
#include "audio_decoder_api.h"
#include "sbcenc_api.h"
#include "bt_config.h"
#include "cvsd_plc.h"
#include "ctrlvars.h"
//application
#include "bt_hf_mode.h"
#include "decoder.h"
#include "audio_core_service.h"
#include "audio_core_api.h"
#include "bt_hf_api.h"
#include "blue_aec.h"
//framework
#include "bt_stack_service.h"
#include "powercontroller.h"

#if (defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT == ENABLE))
//msbc encoder
#define MSBC_CHANNE_MODE	1 		// mono
#define MSBC_SAMPLE_REATE	16000	// 16kHz
#define MSBC_BLOCK_LENGTH	15

// sco sync header H2
uint8_t sco_sync_header[4][2] = 
{
	{0x01, 0x08}, 
	{0x01, 0x38}, 
	{0x01, 0xc8}, 
	{0x01, 0xf8}
};
	
extern uint8_t lc_sco_data_error_flag; //0=CORRECTLY_RX_FLAG; 1=POSSIBLY_INVALID_FLAG; 2=NO_RX_DATA_FLAG; 3=PARTIALLY_LOST_FLAG;
extern uint32_t hfModeSuspend;
extern uint32_t gSpecificDevice;
extern uint32_t BtHfModeExitFlag;
int32_t HfpChangeDelayCount = 0;

/*******************************************************************************
 * AudioCore Sink Channel2 :输出为需要发送的HF数据
 * unit: sample
 ******************************************************************************/
void BtHf_SinkScoDataClear(void)
{
	osMutexLock(gBtHfCt->ScoOutPcmFifoMutex);
	memset(gBtHfCt->ScoOutPcmFifo, 0, BT_SCO_PCM_FIFO_LEN);
	MCUCircular_Config(&gBtHfCt->ScoOutPcmFIFOCircular, gBtHfCt->ScoOutPcmFifo, BT_SCO_PCM_FIFO_LEN);
	gBtHfCt->hfpSendDataCnt = 0;
	osMutexUnlock(gBtHfCt->ScoOutPcmFifoMutex);
}

uint16_t BtHf_SinkScoDataSet(void* InBuf, uint16_t InLen)
{
	if(InLen == 0)
		return 0;

	osMutexLock(gBtHfCt->ScoOutPcmFifoMutex);
	MCUCircular_PutData(&gBtHfCt->ScoOutPcmFIFOCircular, InBuf, InLen * 2);
	osMutexUnlock(gBtHfCt->ScoOutPcmFifoMutex);

	return InLen;
}

void BtHf_SinkScoDataGet(void* OutBuf, uint16_t OutLen)
{
	if(OutLen == 0)
		return;

	osMutexLock(gBtHfCt->ScoOutPcmFifoMutex);
	MCUCircular_GetData(&gBtHfCt->ScoOutPcmFIFOCircular, OutBuf, OutLen*2);
	osMutexUnlock(gBtHfCt->ScoOutPcmFifoMutex);
}


uint16_t BtHf_SinkScoDataSpaceLenGet(void)
{
	return MCUCircular_GetSpaceLen(&gBtHfCt->ScoOutPcmFIFOCircular) / 2;
}

uint16_t BtHf_SinkScoDataLenGet(void)
{
	return MCUCircular_GetDataLen(&gBtHfCt->ScoOutPcmFIFOCircular) / 2;
}

/*******************************************************************************
 * MIC处理后，缓存到待发送缓存FIFO
 * unit: Bytes
 ******************************************************************************/
uint16_t BtHf_SendScoBufSet(void* InBuf, uint16_t InLen)
{
	if(InLen == 0)
		return 0;
	if(!GetScoConnectFlag())
	{
		MCUCircular_Config(&gBtHfCt->MsbcSendCircular, gBtHfCt->MsbcSendFifo, BT_SCO_PCM_FIFO_LEN);
		return 0;
	}
	MCUCircular_PutData(&gBtHfCt->MsbcSendCircular, InBuf, InLen);
	return InLen;
}

void BtHf_SendScoBufGet(void* OutBuf, uint16_t OutLen)
{
	if(OutLen == 0)
		return;

	MCUCircular_GetData(&gBtHfCt->MsbcSendCircular, OutBuf, OutLen);
	if(!GetScoConnectFlag())
		memset(OutBuf, 0,OutLen);
}


uint16_t BtHf_SendScoBufSpaceLenGet(void)
{
	return MCUCircular_GetSpaceLen(&gBtHfCt->MsbcSendCircular);
}

uint16_t BtHf_SendScoBufLenGet(void)
{
	return MCUCircular_GetDataLen(&gBtHfCt->MsbcSendCircular);
}

/***********************************************************************************
 * msbc receive fifo in bytes
 **********************************************************************************/
uint16_t BtHf_SbcRecvBufSet(void* InBuf, uint16_t InLen)
{
	if(InLen == 0)
		return 0;

	MCUCircular_PutData(&gBtHfCt->msbcRecvFifoCircular, InBuf, InLen);
	return InLen;
}

void BtHf_SbcRecvBufGet(void* OutBuf, uint16_t OutLen)
{
	if(OutLen == 0)
		return;

	MCUCircular_GetData(&gBtHfCt->msbcRecvFifoCircular, OutBuf, OutLen);
}


uint16_t BtHf_SbcRecvBufSpaceLenGet(void)
{
	return MCUCircular_GetSpaceLen(&gBtHfCt->msbcRecvFifoCircular);
}

uint16_t BtHf_SbcRecvBufLenGet(void)
{
	return MCUCircular_GetDataLen(&gBtHfCt->msbcRecvFifoCircular);
}



/***********************************************************************************
 * msbc Encoder
 **********************************************************************************/
void BtHf_MsbcEncoderInit(void)
{
	//encoder init
	int32_t samplesPerFrame;
	int32_t ret;

	gBtHfCt->sbc_encode_ct = (SBCEncoderContext*)osPortMalloc(sizeof(SBCEncoderContext));
	if(gBtHfCt->sbc_encode_ct == NULL)
	{
		APP_DBG("sbc encode init malloc error\n");
	}
	memset(gBtHfCt->sbc_encode_ct, 0, sizeof(SBCEncoderContext));

	//if(gBtHfCt->codecType == HFP_AUDIO_DATA_PCM)
	//	return;
	
	ret = sbc_encoder_initialize(gBtHfCt->sbc_encode_ct, MSBC_CHANNE_MODE, MSBC_SAMPLE_REATE, MSBC_BLOCK_LENGTH, SBC_ENC_QUALITY_MIDDLE, &samplesPerFrame);
	APP_DBG("encoder sample:%ld\n", samplesPerFrame);
	if(ret != SBC_ENC_ERROR_OK)
	{
		APP_DBG("sbc encode init error\n");
		return;
	}
	gBtHfCt->mSbcEncoderStart = 1;
}

void BtHf_MsbcEncoderDeinit(void)
{
	if(gBtHfCt->sbc_encode_ct)
	{
		osPortFree(gBtHfCt->sbc_encode_ct);
		gBtHfCt->sbc_encode_ct = NULL;
	}
	
	gBtHfCt->mSbcEncoderStart = 0;
}

/*******************************************************************************
 * MSBC Decoder interface memhandler
 ******************************************************************************/
void BtHf_MsbcMemoryReset(void)
{
	if(gBtHfCt->msbcInFifo == NULL)
		return;
	memset(gBtHfCt->msbcInFifo, 0, BT_MSBC_IN_FIFO_SIZE);
	
	gBtHfCt->msbcInMemHandle.addr = gBtHfCt->msbcInFifo;
	gBtHfCt->msbcInMemHandle.mem_capacity = BT_MSBC_IN_FIFO_SIZE;
	gBtHfCt->msbcInMemHandle.mem_len = 0;
	gBtHfCt->msbcInMemHandle.p = 0;
}

int32_t BtHf_MsbcDecoderInit(void)
{
	gBtHfCt->msbcInFifo = osPortMalloc(BT_MSBC_IN_FIFO_SIZE);
	if(gBtHfCt->msbcInFifo == NULL)
		return -1;
	memset(gBtHfCt->msbcInFifo, 0, BT_MSBC_IN_FIFO_SIZE);
	
	gBtHfCt->msbcInMemHandle.addr = gBtHfCt->msbcInFifo;
	gBtHfCt->msbcInMemHandle.mem_capacity = BT_MSBC_IN_FIFO_SIZE;
	gBtHfCt->msbcInMemHandle.mem_len = 0;
	gBtHfCt->msbcInMemHandle.p = 0;
	
	gBtHfCt->msbcDecoderInitFlag = TRUE;
	BtHf_MsbcDecoderStartedSet(FALSE);

	return 0;
}

int32_t BtHf_MsbcDecoderDeinit(void)
{
	gBtHfCt->msbcInMemHandle.addr = NULL;
	gBtHfCt->msbcInMemHandle.mem_capacity = 0;
	gBtHfCt->msbcInMemHandle.mem_len = 0;
	gBtHfCt->msbcInMemHandle.p = 0;
	
	gBtHfCt->msbcDecoderInitFlag = FALSE;
	BtHf_MsbcDecoderStartedSet(FALSE);

	if(gBtHfCt->msbcInFifo)
	{
		osPortFree(gBtHfCt->msbcInFifo);
		gBtHfCt->msbcInFifo = NULL;
	}
	return 0;
}

void BtHf_MsbcDecoderStartedSet(bool flag)
{
	gBtHfCt->msbcDecoderStarted = flag;
}

bool BtHf_MsbcDecoderStartedGet(void)
{
	return gBtHfCt->msbcDecoderStarted;
}

bool BtHf_MsbcDecoderIsInitialized(void)
{
	return gBtHfCt->msbcDecoderInitFlag;
}

static MemHandle * BtHf_MsbcDecoderMemHandleGet(void)
{
	if(BtHf_MsbcDecoderIsInitialized())
	{
		return &gBtHfCt->msbcInMemHandle;
	}
	return NULL;
}
// bytes
uint32_t BtHf_MsbcDataLenGet(void)
{
	uint32_t	dataSize = 0;
	if(gBtHfCt->msbcDecoderInitFlag)
	{
		dataSize = mv_msize(&gBtHfCt->msbcInMemHandle);
	}
	return dataSize;
}

int32_t BtHf_MsbcDecoderStart(void)
{
	int32_t 		ret = 0;
	
	ret = DecoderInit(BtHf_MsbcDecoderMemHandleGet(),DECODER_MODE_CHANNEL, (int32_t)IO_TYPE_MEMORY, MSBC_DECODER);
	if(ret != RT_SUCCESS)
	{
	//	APP_DBG("[BT]<SBC>: audio_decoder_initialize error code:%ld!\n", audio_decoder_get_error_code());
		BtHf_MsbcDecoderStartedSet(FALSE);
		return -1;
	}
	DecoderPlay(DECODER_MODE_CHANNEL);
//	APP_DBG("[INFO]: sample rate from %u Hz, Channel:%d, type:%d\n", (unsigned int)audio_decoder->song_info->sampling_rate, audio_decoder->song_info->num_channels, audio_decoder->song_info->stream_type);

	APP_DBG("Decoder Service Start...\n");

	//enable hfp source channel
//	AudioCoreSourceEnable(DecoderSourceNumGet());

	BtHf_MsbcDecoderStartedSet(TRUE);
	
	return 0;
}

/*******************************************************************************
 * AEC 
 * 用于进行AEC的缓存远端发送数据
 * uint:sample
 ******************************************************************************/
void BtHf_AECEffectInit(void)
{
//	gCtrlVars.mic_aec_unit.enable				 = 1;
//	gCtrlVars.mic_aec_unit.es_level    			 = BT_HFP_AEC_ECHO_LEVEL;
//	gCtrlVars.mic_aec_unit.ns_level     		 = BT_HFP_AEC_NOISE_LEVEL;
//	AudioEffectAecInit(&gCtrlVars.mic_aec_unit, 1, 16000);//固定为16K采样率
}

bool BtHf_AECInit(void)
{
	if(!gBtHfCt->AecDelayBuf)
		gBtHfCt->AecDelayBuf = (uint8_t*)osPortMalloc(AEC_BLK_LEN*LEN_PER_SAMPLE*MAX_DELAY_BLOCK);  //64*2*32
	
	if(gBtHfCt->AecDelayBuf == NULL)
	{
		return FALSE;
	}
	memset(gBtHfCt->AecDelayBuf, 0, (AEC_BLK_LEN*LEN_PER_SAMPLE*MAX_DELAY_BLOCK));
	
	gBtHfCt->AecDelayRingBuf.addr = gBtHfCt->AecDelayBuf;
	gBtHfCt->AecDelayRingBuf.mem_capacity = (AEC_BLK_LEN*LEN_PER_SAMPLE*MAX_DELAY_BLOCK);
	gBtHfCt->AecDelayRingBuf.mem_len = AEC_BLK_LEN*LEN_PER_SAMPLE*DEFAULT_DELAY_BLK;
	gBtHfCt->AecDelayRingBuf.p = 0;
	
	gBtHfCt->SourceBuf_Aec = (uint16_t*)osPortMalloc(CFG_BTHF_PARA_SAMPLES_PER_FRAME * 2);
	if(gBtHfCt->SourceBuf_Aec == NULL)
	{
		return FALSE;
	}
	memset(gBtHfCt->SourceBuf_Aec, 0, CFG_BTHF_PARA_SAMPLES_PER_FRAME * 2);
	return TRUE;
}

void BtHf_AECReset(void)
{
	memset(gBtHfCt->AecDelayBuf, 0, (AEC_BLK_LEN*LEN_PER_SAMPLE*MAX_DELAY_BLOCK));

	gBtHfCt->AecDelayRingBuf.addr = gBtHfCt->AecDelayBuf;
	gBtHfCt->AecDelayRingBuf.mem_capacity = (AEC_BLK_LEN*LEN_PER_SAMPLE*MAX_DELAY_BLOCK);
	gBtHfCt->AecDelayRingBuf.mem_len = AEC_BLK_LEN*LEN_PER_SAMPLE*DEFAULT_DELAY_BLK;
	gBtHfCt->AecDelayRingBuf.p = 0;

	memset(gBtHfCt->SourceBuf_Aec, 0, CFG_BTHF_PARA_SAMPLES_PER_FRAME * 2);
}

void BtHf_AECDeinit(void)
{
	if(gBtHfCt->SourceBuf_Aec)
	{
		osPortFree(gBtHfCt->SourceBuf_Aec);
		gBtHfCt->SourceBuf_Aec = NULL;
	}
	
	gBtHfCt->AecDelayRingBuf.addr = NULL;
	gBtHfCt->AecDelayRingBuf.mem_capacity = 0;
	gBtHfCt->AecDelayRingBuf.mem_len = 0;
	gBtHfCt->AecDelayRingBuf.p = 0;
	
	if(gBtHfCt->AecDelayBuf)
	{
		osPortFree(gBtHfCt->AecDelayBuf);
		gBtHfCt->AecDelayBuf = NULL;
	}
}


uint32_t BtHf_AECRingDataSet(void *InBuf, uint16_t InLen)
{
	if(InLen == 0)
		return 0;
	
	return mv_mwrite(InBuf, 1, InLen*2, &gBtHfCt->AecDelayRingBuf);
}

uint32_t BtHf_AECRingDataGet(void* OutBuf, uint16_t OutLen)
{
	if(OutLen == 0)
		return 0;
	
	return mv_mread(OutBuf, 1, OutLen*2, &gBtHfCt->AecDelayRingBuf);
}

int32_t BtHf_AECRingDataSpaceLenGet(void)
{
	return mv_mremain(&gBtHfCt->AecDelayRingBuf)/2;
}

int32_t BtHf_AECRingDataLenGet(void)
{
	return mv_msize(&gBtHfCt->AecDelayRingBuf)/2;
}

int16_t *BtHf_AecInBuf(void)
{
	if(BtHf_AECRingDataLenGet() >= CFG_BTHF_PARA_SAMPLES_PER_FRAME)
	{
		BtHf_AECRingDataGet(gBtHfCt->SourceBuf_Aec , CFG_BTHF_PARA_SAMPLES_PER_FRAME);
	}
	else
	{
		memset(gBtHfCt->SourceBuf_Aec, 0, CFG_BTHF_PARA_SAMPLES_PER_FRAME * 2);
	}
	return (int16_t *)gBtHfCt->SourceBuf_Aec;
}

/*******************************************************************************
 * hf volume sync
 ******************************************************************************/
void SetBtHfSyncVolume(uint8_t gain)
{
	BT_MANAGER_ST *	tempBtManager = NULL;
	uint32_t volume = gain;
	//gBtHfCt->BtSyncVolume = volume;
	
	tempBtManager = GetBtManager();

	if(tempBtManager == NULL)
		return;

	tempBtManager->volGain = (uint8_t)((volume*15)/CFG_PARA_MAX_VOLUME_NUM);
	APP_DBG("hf volume:%d\n", tempBtManager->volGain);
	
	BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_HF_VOLUME_SET);
}

uint8_t GetBtHfSyncVolume(void)
{
	return gBtHfCt->BtSyncVolume;
}


/***********************************************************************************
 * get sco data(CVSD)
 **********************************************************************************/
//Len/Length都是Samples(单声道)
uint16_t BtHf_ScoDataGet(void *OutBuf,uint16_t OutLen)
{
	if(gBtHfCt->codecType == HFP_AUDIO_DATA_mSBC)
	{
		return ModeDecoderPcmDataGet(OutBuf, OutLen);
	}
	else
	{
		uint16_t NumSamples = 0;
		NumSamples = MCUCircular_GetData(&gBtHfCt->ScoInPcmFIFOCircular,OutBuf,OutLen*2);
		return NumSamples/2;
	}
}

void BtHf_ScoDataSet(void *InBuf,uint16_t InLen)
{
	MCUCircular_PutData(&gBtHfCt->ScoInPcmFIFOCircular,InBuf,InLen);
}

uint16_t BtHf_ScoDataLenGet(void)
{
	if(gBtHfCt->codecType == HFP_AUDIO_DATA_mSBC)
	{
		return ModeDecoderPcmDataLenGet();
	}
	else
	{
		uint16_t NumSamples = 0;
		NumSamples = MCUCircular_GetDataLen(&gBtHfCt->ScoInPcmFIFOCircular);
		return NumSamples/2;
	}
}

uint16_t BtHf_ScoDataSpaceLenGet(void)
{
	uint16_t NumSamples = 0;
	NumSamples = MCUCircular_GetSpaceLen(&gBtHfCt->ScoInPcmFIFOCircular);
	return NumSamples;
}

/***********************************************************************************
 * 发送SCO数据函数
 **********************************************************************************/
void BtHf_SendData(void)
{
	/*if(gBtHfCt->hfpSendDataCnt)
	{
		gBtHfCt->hfpSendDataCnt--;
		if(gBtHfCt->codecType == HFP_AUDIO_DATA_PCM)
		{
			memset(gBtHfCt->scoSendBuf,0,BT_CVSD_PACKET_LEN);
			MCUCircular_GetData(&gBtHfCt->ScoOutPcmFIFOCircular, gBtHfCt->scoSendBuf, BT_CVSD_PACKET_LEN);
			HfpSendScoData(BtCurIndex_Get(),gBtHfCt->scoSendBuf, BT_CVSD_PACKET_LEN);
		}
		else
		{
			memset(gBtHfCt->scoSendBuf,0,BT_MSBC_PACKET_LEN);
			BtHf_SendScoBufGet(gBtHfCt->scoSendBuf,BT_MSBC_PACKET_LEN);
			HfpSendScoData(BtCurIndex_Get(),gBtHfCt->scoSendBuf, BT_MSBC_PACKET_LEN);
		}
	}*/
}


/***********************************************************************************
 * 接收到HFP SCO数据,插入缓存fifo， 从输出fifo 准备数据 send packet
 **********************************************************************************/
int16_t BtHf_SaveScoData(uint8_t* data, uint16_t len)
{
	uint32_t	insertLen = 0;
	int32_t 	remainLen = 0;
	uint16_t	msbcLen = 0;
	
	if((BtHfModeExitFlag) || (gBtHfCt == NULL))
		return 0;

#ifdef CFG_FUNC_REMIND_SOUND_EN
	if((sys_parameter.bt_CallinRingType != USE_LOCAL_AND_PHONE_RING)
		&&(GetHfpState(BtCurIndex_Get()) == BT_HFP_STATE_INCOMING)
		&&(gBtHfCt->btPhoneCallRing == 0))
	{
		return -1;
	}
#endif

	gBtHfCt->btPhoneCallRing = 1;
	if((DecoderSourceNumGet(DECODER_MODE_CHANNEL)!=BT_HF_SOURCE_NUM))
		return -1;

	gBtHfCt->ScoInputBytes = len;

	//延时开启source
	if(gBtHfCt->btHfResumeCnt)
	{
		gBtHfCt->btHfResumeCnt++;
		if(gBtHfCt->btHfResumeCnt == 10)
		{
			AudioCoreSourceEnable(BT_HF_SOURCE_NUM);
		}
		
		if(gBtHfCt->btHfResumeCnt>=35)
		{
			gBtHfCt->btHfResumeCnt=0;
			//AudioCoreSourceUnmute(BT_HF_SOURCE_NUM, TRUE, TRUE);
		}
	}

	if(gBtHfCt->codecType == HFP_AUDIO_DATA_PCM)
	{
		if(len == 120)
		{
			if(gSpecificDevice)
			{
				if(memcmp(&gBtHfCt->scoSpecificFifo[gBtHfCt->scoSpecificIndex][0],data, 120) == 0)
				{
					gBtHfCt->scoSpecificCount++;
				}
				else
				{
					/*if(gBtHfCt->scoSpecificCount>=20)
					{
						APP_DBG("unmute-resume...\n");
						//AudioCoreSourceUnmute(BT_HF_SOURCE_NUM, TRUE, TRUE);
					}
					*/
					gBtHfCt->scoSpecificCount=0;
				}
				memcpy(&gBtHfCt->scoSpecificFifo[gBtHfCt->scoSpecificIndex][0],data, 120);
				gBtHfCt->scoSpecificIndex++;
				if(gBtHfCt->scoSpecificIndex>=3)
					gBtHfCt->scoSpecificIndex=0;

				if(gBtHfCt->scoSpecificCount == 20)
				{
					APP_DBG("cvsd-mute\n");
					//AudioCoreSourceMute(BT_HF_SOURCE_NUM, TRUE, TRUE);
				}
			}
			//cvsd - plc
			if(lc_sco_data_error_flag)
			{
				//APP_DBG("sco_error:%d\n", lc_sco_data_error_flag);
				lc_sco_data_error_flag=0;
				cvsd_plc_bad_frame(gBtHfCt->cvsdPlcState, (int16_t *)gBtHfCt->ScoInDataBuf);
			}
			else
			{
				cvsd_plc_good_frame(gBtHfCt->cvsdPlcState, (int16_t *)data, (int16_t *)gBtHfCt->ScoInDataBuf);
			}

			gBtHfCt->ScoInputBytes = len;

			if((BtHf_ScoDataSpaceLenGet() > len)&&(HfpChangeDelayCount == 0))
			{
				BtHf_ScoDataSet(gBtHfCt->ScoInDataBuf, len);
			}
			else
			{
				DBG("Sco in fifo is full\n");
			}

			//send data(120Bytes)
			if(MCUCircular_GetDataLen(&gBtHfCt->ScoOutPcmFIFOCircular) > BT_CVSD_PACKET_LEN)
			{
				memset(gBtHfCt->scoSendBuf,0,BT_CVSD_PACKET_LEN);
				MCUCircular_GetData(&gBtHfCt->ScoOutPcmFIFOCircular, gBtHfCt->scoSendBuf, BT_CVSD_PACKET_LEN);
				HfpSendScoData(BtCurIndex_Get(),gBtHfCt->scoSendBuf, BT_CVSD_PACKET_LEN);
			}
		}
		else if(len == 60)
		{
			//KK: 某些第三方测试仪发送包每次60Bytes
			//send data(60Bytes)
			if(MCUCircular_GetDataLen(&gBtHfCt->ScoOutPcmFIFOCircular) >= BT_CVSD_SAMPLE_SIZE)
			{
				memset(gBtHfCt->scoSendBuf,0,BT_CVSD_SAMPLE_SIZE);
				MCUCircular_GetData(&gBtHfCt->ScoOutPcmFIFOCircular, gBtHfCt->scoSendBuf, BT_CVSD_SAMPLE_SIZE);
				HfpSendScoData(BtCurIndex_Get(),gBtHfCt->scoSendBuf, BT_CVSD_SAMPLE_SIZE);
			}

			gBtHfCt->scoSpecificIndex++;
			if(gBtHfCt->scoSpecificIndex%2)
			{
				memcpy(&gBtHfCt->BtHfScoBufBk[0], data, 60);
				return 0;
			}
			else
			{
				memcpy(&gBtHfCt->BtHfScoBufBk[60], data, 60);
			}
			
			//cvsd - plc
			if(lc_sco_data_error_flag)
			{
				//APP_DBG("sco_error:%d\n", lc_sco_data_error_flag);
				lc_sco_data_error_flag=0;
				cvsd_plc_bad_frame(gBtHfCt->cvsdPlcState, (int16_t *)gBtHfCt->ScoInDataBuf);
			}
			else
			{
				cvsd_plc_good_frame(gBtHfCt->cvsdPlcState, (int16_t *)gBtHfCt->BtHfScoBufBk, (int16_t *)gBtHfCt->ScoInDataBuf);
			}
			
			gBtHfCt->ScoInputBytes = 120;

			if(BtHf_ScoDataSpaceLenGet() > gBtHfCt->ScoInputBytes &&(HfpChangeDelayCount == 0))
			{
				BtHf_ScoDataSet(gBtHfCt->ScoInDataBuf, gBtHfCt->ScoInputBytes);
			}
			else
			{
				DBG("Sco in fifo is full\n");
			}
		}
		else
		{
			APP_DBG("[warm] save len %d error!\n", len);
		}
	}
	else
	{
		if(gBtHfCt->ScoInputBytes == BT_MSBC_PACKET_LEN)
		{
			//send data(60Bytes)
			if((BtHf_SendScoBufLenGet() > BT_MSBC_PACKET_LEN)&&(HfpChangeDelayCount == 0))
			{
				memset(gBtHfCt->scoSendBuf,0,BT_MSBC_PACKET_LEN);
				BtHf_SendScoBufGet(gBtHfCt->scoSendBuf,BT_MSBC_PACKET_LEN);
				HfpSendScoData(BtCurIndex_Get(),gBtHfCt->scoSendBuf, BT_MSBC_PACKET_LEN);

				//gBtHfCt->hfpSendDataCnt++;
				//printf("s");
			}
		}
		
		//接收到的数据需要做如下判断
		//1.全0 丢弃
		//2.长度非60的，需要放入缓存
		//3.长度为60的直接处理
		if((!lc_sco_data_error_flag)&&(!BtHf_MsbcDecoderStartedGet()))
		{
			//为0数据丢弃
			if((data[0]==0)&&(data[1]==0)&&(data[2]==0)&&(data[3]==0)&&(data[len-2]==0)&&(data[len-1]==0))
				return -1;
		}

		//长度
		if(gBtHfCt->ScoInputBytes != BT_MSBC_PACKET_LEN)
		{
			if((data[0] != 0x01)||(data[2] != 0xad)||(data[3] != 0x00))
			{
				APP_DBG("msbc discard...\n");
				return -1;
			}
		}

		{
			//数据格式: 0-1:sync header;  2-58:msbc data(57bytes - 有效数据); 59:tail
			memcpy(gBtHfCt->ScoInDataBuf, data, 60);
			
			//接收到错误的msbc数据包,则将该次收到的数据全部清空为0
			if(lc_sco_data_error_flag)
			{
				//APP_DBG("msbc e:%d\n",lc_sco_data_error_flag);
				lc_sco_data_error_flag=0;

				gBtHfCt->ScoInDataBuf[0] = 0x01;
				gBtHfCt->ScoInDataBuf[1] = 0x08;
				
				memset(&gBtHfCt->ScoInDataBuf[2], 0, 58);

				//if((!BtHf_MsbcDecoderStartedGet())||(lc_sco_data_error_flag == 2))
				if(!BtHf_MsbcDecoderStartedGet())
					return -1;

				gBtHfCt->msbcPlcCnt = 2;
			}

			if(BtHf_SbcRecvBufSpaceLenGet()<=60)
			{
				APP_DBG("msbc recv fifo full-1\n");
				return -1;
			}
			BtHf_SbcRecvBufSet(gBtHfCt->ScoInDataBuf, 60);

			if(BtHf_SbcRecvBufLenGet()<60*2)
				return -1;

			if(gBtHfCt->msbcPlcCnt == 0)
			{
				uint8_t i;
				for(i=0;i<60;i++)
				{
					BtHf_SbcRecvBufGet(&gBtHfCt->ScoInDataBuf[0], 1);
					if(gBtHfCt->ScoInDataBuf[0] == 0x01)
					{
						BtHf_SbcRecvBufGet(&gBtHfCt->ScoInDataBuf[0], 1);
						gBtHfCt->ScoInDataBuf[0]&=0x0f;
						if(gBtHfCt->ScoInDataBuf[0] == 0x08)
						{
							BtHf_SbcRecvBufGet(&gBtHfCt->ScoInDataBuf[0], 58);
							break;
						}
					}
				}

				if(i==60)
					return -1;
			}
			else
			{
				gBtHfCt->msbcPlcCnt--;
				BtHf_SbcRecvBufGet(&gBtHfCt->ScoInDataBuf[0], 60);

				memset(&gBtHfCt->ScoInDataBuf[0], 0, 60);
			}
		}

		msbcLen = 57;
		
		remainLen = mv_mremain(&gBtHfCt->msbcInMemHandle);
		if(remainLen <= msbcLen)
		{
			APP_DBG("msbc receive fifo is full and reset MsbcMemory\n");
			BtHf_MsbcMemoryReset();//bkd  add
			//return -1; //bkd //
		}
		insertLen = mv_mwrite(gBtHfCt->ScoInDataBuf, msbcLen, 1, &gBtHfCt->msbcInMemHandle);
		if(insertLen != msbcLen)
		{
			APP_DBG("insert data len err! i:%ld,d:%d\n", insertLen, msbcLen);
		}

		//decoder start
		if(!BtHf_MsbcDecoderStartedGet())
		{
			if(BtHf_MsbcDataLenGet()>BT_MSBC_LEVEL_START)
			{
				int32_t ret=0;
				ret = BtHf_MsbcDecoderStart();
				if(ret == 0)
				{
					APP_DBG("msbc decoder start success\n");
					BtHf_MsbcDecoderStartedSet(TRUE);
					AudioCoreSourceEnable(BT_HF_SOURCE_NUM);
				}
				else
				{
					APP_DBG("msbc decoder start fail\n");
				}
				
			}
		}
	}

	return 0;
}

/***********************************************************************************
 * 处理HFP需要发送的数据
 * 数据来源:从Audio Core Sink输出;
 * 数据处理: MSBC格式: 进行Encoder,并加入Header 数据处理完毕后,缓存到待发送fifo
 * 		   PCM格式 无需处理，直接从sinkFifo等待发送。
 **********************************************************************************/
//2-EV3 = 60bytes/packet
//处理发送数据函数入口
void BtHf_EncodeProcess(void)
{
	int32_t return_flag = 0;
	
	if(HfpChangeDelayCount > 0)
	{
		HfpChangeDelayCount--;
		return;
	}
	
	if(gSpecificDevice)
	{
		if((!gBtHfCt->CvsdInitFlag) && (!gBtHfCt->MsbcInitFlag))
		{
			return_flag = 1;
		
		}
		if(GetHfpState(BtCurIndex_Get()) < BT_HFP_STATE_INCOMING)
		{
			return_flag = 1;
		}
	}
	else
	{
		if(GetHfpState(BtCurIndex_Get()) < BT_HFP_STATE_ACTIVE)
		{
			return_flag = 1;
		}
	}
	
	if(return_flag)
	{
		AudioCoreSinkDisable(AUDIO_HF_SCO_SINK_NUM);
		return;
	}
	
	if(gBtHfCt->btHfScoSendReady)
	{
		gBtHfCt->btHfScoSendReady = 0;
		gBtHfCt->btHfScoSendStart = 1;
		return_flag = 1;
	}

	if(!gBtHfCt->btHfScoSendStart)
	{
		return_flag = 1;
	}

	if(return_flag)
	{
		AudioCoreSinkDisable(AUDIO_HF_SCO_SINK_NUM);
		return;
	}
	
	//sink2 channel
	if(!AudioCoreSinkIsEnable(AUDIO_HF_SCO_SINK_NUM))
	{
		BtHf_SinkScoDataClear();
		AudioCoreSinkEnable(AUDIO_HF_SCO_SINK_NUM);
	}

	//将需要上传的数据(audio core sink)转换,并搬运到send buf,等待发送
	if(gBtHfCt->codecType == HFP_AUDIO_DATA_mSBC)
	{
		if(gBtHfCt->mSbcEncoderStart)
		{
			//msbc encoder: 120 Sample
			if(BtHf_SinkScoDataLenGet()>BT_MSBC_MIC_INPUT_SAMPLE)
			{

				//1.encoder
				BtHf_SinkScoDataGet(gBtHfCt->PcmBufForMsbc, BT_MSBC_MIC_INPUT_SAMPLE);
				memset(gBtHfCt->msbcEncoderOut, 0, BT_MSBC_PACKET_LEN);
				
				//2. add sync header
				if(gBtHfCt->msbcSyncCount>3) gBtHfCt->msbcSyncCount = 0;
				memcpy(&gBtHfCt->msbcEncoderOut[0], sco_sync_header[gBtHfCt->msbcSyncCount], 2);
				gBtHfCt->msbcSyncCount++;
				gBtHfCt->msbcSyncCount %= 4;

				//3. encode
				sbc_encoder_encode(gBtHfCt->sbc_encode_ct, gBtHfCt->PcmBufForMsbc, &gBtHfCt->msbcEncoderOut[2], &gBtHfCt->msbc_encoded_data_length);

				//4.put data to send fifo
				if(BtHf_SendScoBufSpaceLenGet() > BT_MSBC_PACKET_LEN)
				{
					BtHf_SendScoBufSet(gBtHfCt->msbcEncoderOut, BT_MSBC_PACKET_LEN);
				}
				else
				{
					MCUCircular_Config(&gBtHfCt->MsbcSendCircular, gBtHfCt->MsbcSendFifo, BT_SCO_PCM_FIFO_LEN);//bkd add
					//BT_DBG("Send ScoBuf is full ,reset buffer\n");
				}
			}
		}
	}
}
#endif

