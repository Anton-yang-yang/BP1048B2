/**
 **************************************************************************************
 * @file    decoder_service.h
 * @brief   
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2016-6-29 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef __DECODER_SERVICE_H__
#define __DECODER_SERVICE_H__

#include "type.h"
#include "rtos_api.h"
#include "audio_decoder_api.h"
#include "mvstdio.h"
#include "app_message.h"

typedef enum
{
	DecoderStateNone = 0,
	DecoderStateInitialized,
	DecoderStateDeinitializing,
	DecoderStateStop,
	DecoderStatePlay,
	DecoderStatePause,
	DecoderStateDecoding,
	DecoderStateToSavePcmData,
	DecoderStateSavePcmData,

}DecoderState;


typedef enum
{
	DS_EVENT_SERVICE_INITED,
	DS_EVENT_SERVICE_STARTED,
	DS_EVENT_DECODE_INITED,
	DS_EVENT_DECODE_DECODED,
} DecoderServiceEvent;

typedef enum
{
	DECODER_MODE_CHANNEL,
#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY	
	DECODER_REMIND_CHANNEL,
#endif	
	DECODER_MAX_CHANNELS,
}DecoderChannels;

#define DECODER_BUF_SIZE   					(1024 * 28)	//解码器开销，Unit:  BYTE，需充分测试。
#define DECODER_BUF_SIZE_MP3				(1024 * 19)
#define DECODER_BUF_SIZE_SBC				(1024 * 7)

//WMA:28536,AAC:28444,APE:25340,FLA:22580,MP3:19136,AIF:11236,WAV:11228,SBC:5624,实际块大小会变，比如adpcm的wav。

#define DECODER_FIFO_SIZE_MIN				(CFG_PARA_MAX_SAMPLES_PER_FRAME * 4)//最小 fifo 设置
#define DECODER_FIFO_SIZE_FOR_PLAYER 		(CFG_PARA_MAX_SAMPLES_PER_FRAME * 24)	//无损读取、高压缩解码阻塞情况，要求大缓冲。如：Flac24bit 1.5Mbps要改为*20;1.7Mbps要改为*24
#define	DECODER_FIFO_SIZE_FOR_SBC			(CFG_PARA_MAX_SAMPLES_PER_FRAME * 6)
#define DECODER_FIFO_SIZE_FOR_MP3			(CFG_PARA_MAX_SAMPLES_PER_FRAME * 8)

//memhandle 条件下 解码水位限制
#define SBC_DECODER_FIFO_MIN				(119*2)//(119*17)
#define AAC_DECODER_FIFO_MIN				(600)
#define MSBC_DECODER_FIFO_MIN				57*2//238
//流数据 报告 解码EMPTY错误计数设定	 定义参见 audio_decoder_error_code_summary.txt
#define MPX_ERROR_STREAM_EMPTY				-127
#define SBC_ERROR_STREAM_EMPTY				-123
#define AAC_ERROR_STREAM_EMPTY				-127
#define AMR_ERROR_STREAM_EMPTY				-127
#define WAV_ERROR_STREAM_EMPTY				-127
//输入数据最低限位不解码，不应该出现流数据EMPTY次数累积，当前保留这个消息机制。
#define MUTLI_EMPTY_FOR_MSG					300//统一设定次数，如有必要可init时参数传递特定值。

//void DecoderServiceStart(DecoderChannels DecoderChannel);
//void DecoderServicePause(DecoderChannels DecoderChannel);
//void DecoderServiceResume(DecoderChannels DecoderChannel);
void DecoderServiceStop(DecoderChannels DecoderChannel);
void DecoderServiceDeinit(DecoderChannels DecoderChannel);
MessageHandle GetDecoderServiceMsgHandle(void);
//TaskState GetDecoderServiceState(DecoderChannels DecoderChannel);


int32_t DecoderInit(void *io_handle, DecoderChannels DecoderChannel, int32_t ioType, int32_t decoderType);
void DecoderPlay(DecoderChannels DecoderChannel);
//void DecoderReset(DecoderChannels DecoderChannel);
void DecoderStop(DecoderChannels DecoderChannel);
void DecoderMuteAndStop(DecoderChannels DecoderChannel);
void DecoderPause(DecoderChannels DecoderChannel);

void DecoderResume(DecoderChannels DecoderChannel);

//注意，StepTime 单位为毫秒，实际传递值为整数秒
void DecoderFF(uint32_t StepTime,DecoderChannels DecoderChannel);

//注意，StepTime 单位为毫秒，实际传递值为整数秒
void DecoderFB(uint32_t StepTime,DecoderChannels DecoderChannel);

bool DecoderSeek(uint32_t Time,DecoderChannels DecoderChannel);

uint32_t DecoderServicePlayTimeGet(DecoderChannels DecoderChannel);

uint32_t GetDecoderState(DecoderChannels DecoderChannel);

uint8_t DecoderSourceNumGet(DecoderChannels DecoderChannel);

//解码器输出在audiocore source通路中配置的Num号
void DecoderSourceNumSet(uint8_t Num,DecoderChannels DecoderChannel);
bool DecoderServiceInit(MessageHandle ParentMsgHandle, DecoderChannels DecoderChannel,uint32_t BufSize, uint16_t FifoSize);
void RemindDecodeProcess(void);
void ModeDecodeProcess(void);
uint16_t ModeDecoderPcmDataGet(void * pcmData,uint16_t sampleLen);// call by audio core one by one
//uint16_t RemindDecoderPcmDataGet(void * pcmData,uint16_t sampleLen);// call by audio core one by one
uint16_t ModeDecoderPcmDataLenGet(void);
//uint16_t RemindDecoderPcmDataLenGet(void);


#endif /*__DECODER_SERVICE_H__*/


