/**
 **************************************************************************************
 * @file    bt_hf_mode.h
 * @brief	蓝牙通话模式
 *
 * @author  kk
 * @version V1.0.0
 *
 * $Created: 2018-7-17 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __BT_HF_MODE_H__
#define __BT_HF_MODE_H__

#include "type.h"
#include "rtos_api.h"

#include "audio_vol.h"
#include "rtos_api.h"
#include "resampler_polyphase.h"
#include "mcu_circular_buf.h"
#include "audio_core_api.h"
#include "audio_decoder_api.h"
#include "sbcenc_api.h"
#include "bt_config.h"
#include "cvsd_plc.h"
#include "ctrlvars.h"
#include "blue_ns.h"

#define BT_HF_SOURCE_NUM			APP_SOURCE_NUM


#define BTHF_CVSD_SAMPLE_RATE		8000
#define BTHF_MSBC_SAMPLE_RATE		16000
#define BTHF_DAC_SAMPLE_RATE		16000

#define BT_SCO_PCM_FIFO_LEN			(CFG_BTHF_PARA_SAMPLES_PER_FRAME * 2 * 2) //nomo 4倍空间

#define SCO_IN_DATA_SIZE			120 //BT_CVSD_PACKET_LEN = 120, BT_MSBC_PACKET_LEN = 60
#define MSBC_PACKET_PCM_SIZE		240 //msbc 编码的pcm输入

#define BT_CVSD_SAMPLE_SIZE			60
#define BT_CVSD_PACKET_LEN			120

#define BT_MSBC_IN_FIFO_SIZE		2*1024

#define BT_MSBC_LEVEL_START			(57*20)
#define BT_MSBC_PACKET_LEN			60
#define BT_MSBC_MIC_INPUT_SAMPLE	120 //stereo:480bytes -> mono:240bytes

#define BT_MSBC_ENCODER_FIFO_LEN	(CFG_BTHF_PARA_SAMPLES_PER_FRAME * 2 * 2)

//sbc receive fifo
#define BT_SBC_RECV_FIFO_SIZE		(60*6)


//AEC
#define FRAME_SIZE					AEC_BLK_LEN
#define AEC_SAMPLE_RATE				16000
#define LEN_PER_SAMPLE				2 //mono
#define MAX_DELAY_BLOCK				BT_HFP_AEC_MAX_DELAY_BLK
#define DEFAULT_DELAY_BLK			BT_HFP_AEC_DELAY_BLK

//PITCH SHIFTER
#define MAX_PITCH_SHIFTER_STEP		25

typedef enum 
{
	BT_HF_CALLING_NONE = 0,
	BT_HF_CALLING_ACTIVE,
	BT_HF_CALLING_SUSPEND,
} BT_HFP_CALLING_STATE;

typedef struct _btHfContext
{

	uint8_t				codecType;

//	TaskState			state;
	BT_HFP_CALLING_STATE callingState;

	//QueueHandle_t 		audioMutex;
	//QueueHandle_t		pcmBufMutex;

	uint8_t 			DecoderSourceNum;
	
	uint32_t 			SampleRate;
	uint8_t				BtSyncVolume;

	//adc fifo
	uint32_t			*ADCFIFO;	//adc input fifo (stereo) //

	//sco Fifo
	MCU_CIRCULAR_CONTEXT ScoInPcmFIFOCircular;
	int8_t*				ScoInPcmFIFO;
	uint8_t*			ScoInDataBuf;//处理 收数据 CVSD or MSBC         //cvsd=120, msbc=60
	uint16_t			ScoInputBytes;

	//cvsd plc
	CVSD_PLC_State		*cvsdPlcState;
	uint8_t				BtHfScoBufBk[120];
	
	//sco send to phone buffer 120bytes
	uint8_t				scoSendBuf[BT_CVSD_PACKET_LEN];
#ifdef CFG_FUNC_REMIND_SOUND_EN
	TIMER				CallRingTmr;	
	bool 				RemindSoundStart; 
	bool				WaitFlag;	
	uint8_t				PhoneNumber[20];
	int16_t				PhoneNumberLen;
#endif

	//MSBC
	//SBC receive buffer
	MCU_CIRCULAR_CONTEXT	msbcRecvFifoCircular;
	uint8_t*				msbcRecvFifo;
	uint8_t					msbcPlcCnt;

	//msbcIn decoder
	uint8_t				msbcDecoderStarted;
	uint8_t				msbcDecoderInitFlag;
	MemHandle 			msbcInMemHandle;
	uint8_t				*msbcInFifo;

	//msbcOut encoder
	int16_t*			PcmBufForMsbc;//mSbc编码buf
	uint8_t				mSbcEncoderStart;
	uint8_t				mSbcEncoderSendStart;
	uint8_t				msbcSyncCount;
	uint8_t				msbcEncoderOut[BT_MSBC_PACKET_LEN];
	uint32_t			msbc_encoded_data_length;
//	uint32_t			test_msbc_encoded_data_length;
	
	uint8_t				btHfScoSendStart;//正式处理需要发送的数据
	uint8_t				btHfScoSendReady;//可以开始发送数据了,初始化发送的相关参数
	
	SBCEncoderContext	*sbc_encode_ct;

	uint32_t			BtHfModeRestart;
	
	MCU_CIRCULAR_CONTEXT	ScoOutPcmFIFOCircular;
	osMutexId			ScoOutPcmFifoMutex;
	int16_t*			ScoOutPcmFifo;

	MCU_CIRCULAR_CONTEXT MsbcSendCircular;
	uint8_t				*MsbcSendFifo;
	
#ifdef BT_HFP_CALL_DURATION_DISP
	uint32_t			BtHfTimerMsCount;
	uint32_t			BtHfActiveTime;
	bool				BtHfTimeUpdate;
#endif

	uint8_t				ModeKillFlag;
	uint8_t				CvsdInitFlag;
	uint8_t				MsbcInitFlag;
	
	//aec
	uint8_t				*AecDelayBuf;		//BLK_LEN*LEN_PER_SAMPLE*MAX_DELAY_BLOCK
	MemHandle			AecDelayRingBuf;
	uint16_t			*SourceBuf_Aec;

	uint8_t				btHfResumeCnt;
	uint8_t				btPhoneCallRing; //0=本地铃声  1=手机铃声

	//specific device(如WIN7 PC用于处理HFP数据-cvsd)
	uint8_t				scoSpecificFifo[3][120];
	uint32_t			scoSpecificIndex;
	uint32_t			scoSpecificCount;
	
	uint32_t 			SystemEffectMode;//用于标记system当前的音效模式，HFP音效单独

	uint8_t 			hfpSendDataCnt;//蓝牙sco发送计数器
}BtHfContext;
extern BtHfContext	*gBtHfCt;

MessageHandle GetBtHfMessageHandle(void);
void BtHfTaskResume(void);

void BtHfModeEnter_Index(uint8_t index);

void BtHfModeEnter(void);

void BtHfModeExit(void);

void BtHf_Timer1msProcess(void);

void BtHfRemindNumberStop(void);

void BtHfCodecTypeUpdate(uint8_t codecType);

void BtHfModeRunningResume(void);

void DelayExitBtHfModeSet(void);

bool GetDelayExitBtHfMode(void);

void DelayExitBtHfModeCancel(void);

void DelayExitBtHfMode(void);

 bool BtHfInit(void);
 
 void BtHfRun(uint16_t msgId);
 
 bool BtHfDeinit(void);

#endif /*__BT_HF_MODE_H__*/



