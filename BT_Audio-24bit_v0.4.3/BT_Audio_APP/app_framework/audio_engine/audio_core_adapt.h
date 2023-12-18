/*
 * audio_core_adapt.h
 *
 *  Created on: Mar 11, 2021
 *      Author: piwang
 */

#ifndef BT_AUDIO_APP_SRC_INC_AUDIO_CORE_ADAPT_H_
#define BT_AUDIO_APP_SRC_INC_AUDIO_CORE_ADAPT_H_
#include "type.h"
#include "resampler_polyphase.h"
#include "mcu_circular_buf.h"
//#include "sra.h"
#include "resampler_farrow.h"
#include "app_config.h"
#define SRA_MAX_CHG 		2
#define SRA_BLOCK 			128
#define POLYNOMIAL_ORDER	 4

typedef enum _MIX_NET_
{
	DefaultNet,
//	SeparateNet,
//......
	MaxNet,
} MIX_NET;

typedef enum _AUDIO_ADAPT_
{
	STD = 0,	//one frame /Direct
	SRC_ONLY,
	SRA_ONLY,
	CLK_ADJUST_ONLY,
	SRC_SRA,
	SRC_ADJUST,
} AUDIO_ADAPT;


typedef struct _SRC_ADAPTER_
{
	uint32_t					SampleRate;
	ResamplerPolyphaseContext	SrcCt;
	MCU_CIRCULAR_CONTEXT		SrcBufHandler;
} SRC_ADAPTER;

typedef struct _SRA_ADAPTER_
{
	bool						Enable;//使能开关，通路开启时有效
	ResamplerFarrowContext		SraResFarCt;
	MCU_CIRCULAR_CONTEXT		SraBufHandler;
	uint32_t					TotalNum;//微调水位统计--旨在多次求平均
	uint16_t					Count;//微调水位统计计数器
	int8_t						AdjustVal;//软件微调 插点和删点数值，源于周期性水位判断
	uint32_t					Depth;//LenGetFunc()最大采样点深度
	uint8_t						HighLevelCent;
	uint8_t						LowLevelCent;
} SRA_ADAPTER;

typedef struct _CLK_ADJUST_ADAPTER
{
	bool						Enable;//使能开关，通路开启时有效
	uint32_t					TotalNum;//微调水位统计--旨在多次求平均
	uint16_t					Count;//微调水位统计计数器
	uint32_t					Depth;//LenGetFunc()最大采样点深度
	uint8_t						HighLevelCent;
	uint8_t						LowLevelCent;
	uint32_t					LastLevel;//前一次平均水位
	int8_t						RiseTimes;//连续上涨次数或下跌次数
	int8_t						AdjustVal;//分频寄存器小数部分
} CLK_ADJUST_ADAPTER;

typedef struct _AudioCoreIO_
{
	uint8_t						Channels;//1/2 注意：init必须配置最大值，通道续存期间可以随音源改变。
	bool						Sync; //TRUE:每帧同步，不足时混音通路阻塞。 FALSE:帧操作，Source数据不足时填零，Sink：空间不足时丢弃
	MIX_NET						Net; //缺省：DefaultNet，混音通路分组
	AUDIO_ADAPT					Adapt; //缺省:STD,通路采样和微调功能设置
#ifdef	CFG_AUDIO_WIDTH_24BIT
	uint8_t						IOBitWidth;//输入音频位宽，0,16bit，1,24bit
	uint8_t						IOBitWidthConvFlag;//输入音频位宽，是否需要扩充到24bit
#endif
	void						*DataIOFunc;//音频输入输出 必须配置  @sample
	void						*LenGetFunc;//source数据长度 sink空间长度 必须配置 @sample
	//微调参数设置，Adapt不支持微调时缺省0
	uint8_t						HighLevelCent;//高水位%：建议60
	uint8_t						LowLevelCent;//低水位%：建议40
	uint16_t					Depth;//LenGetFunc()最大值 @sample
	//转采样参数，Adapt不支持转采样时缺省0
	uint32_t					SampleRate;//DataIOFunc()所在fifo数据的采样率
	bool						Resident;//sink，不跟随应用频繁申请释放帧buf时 设TRUE
} AudioCoreIO;//音频通路接口参数 组合

//使用通路 位段组合和MASK
#define SOURCE_BIT_EN(Value, Index) 		(Value) |= (1<<(Index))
#define SOURCE_BIT_DIS(Value, Index)		(Value) &= ~(1<<(Index))
#define	SOURCE_BIT_GET(Value, Index)		(((Value) >> (Index)) & 0x01)
#define	SOURCE_BIT_MASK(Value)				((Value) & ((1<<AUDIO_CORE_SOURCE_MAX_NUM) - 1))
#define SINK_BIT_EN(Value, Index) 			(Value) |= (1<<(AUDIO_CORE_SOURCE_MAX_NUM + Index))
#define SINK_BIT_DIS(Value, Index)			(Value) &= ~(1<<(AUDIO_CORE_SOURCE_MAX_NUM + Index))
#define	SINK_BIT_GET(Value, Index)			(((Value) >> (AUDIO_CORE_SOURCE_MAX_NUM + Index)) & 0x01)
#define SINK_BIT_MASK(Value)			((Value) & (((1<<(AUDIO_CORE_SOURCE_MAX_NUM + AUDIO_CORE_SINK_MAX_NUM)) - 1) - ((1<<AUDIO_CORE_SOURCE_MAX_NUM) - 1)))

//#define SOURCES(Index)	(AudioCore.AudioSource[Index])
//#define SINKS(Index)	(AudioCore.AudioSink[Index])
#define SOURCEFRAME(Index)				(AudioCore.FrameSize[AudioCore.AudioSource[Index].Net])
#define SINKFRAME(Index)				(AudioCore.FrameSize[AudioCore.AudioSink[Index].Net])

#define	ADJUST_PERIOD				(512*3)		//以采样点计数 监测消抖周期，平衡波动和速度，SRA:增减一点的周期,对应频繁幅度
#define ADJUST_APLL_PERIOD			(512*10)	//以采样点监测消抖周期，用于硬件微调。
#define ADJUST_DIV_MAX				6			//硬件微调限幅度对应万分之五的频偏，源于div的小数分频调整。
#define	ADJUST_SHRESHOLD			2			//2~4 Low～high之间 抑制硬件微调的震荡，连续单向水位变化，反向抑制。

#define SRC_INPUT_MAX		128 //即 MAX_FRAME_SAMPLES @resampler_polyphase.h
#define SRC_INPUT_MIN		8//低于这两个数值，不做转换处理 减少累积误差
#define SRC_OUPUT_MIN		4
#define SRC_OUPUT_JITTER	3//输出采样点偏差 正常是舍入偏差1点， 第一帧可能delay引入2点左右


#define SRC_SCALE_MAX		(6)			//48000	<--	8000 RESAMPLER_POLYPHASE_SRC_RATIO_6_1  @file	resampler_polyphase.h
#define	SRC_SCALE_MIN		(147/640)	 //44100 <-- 192000	RESAMPLER_POLYPHASE_SRC_RATIO_147_640

#define SRC_FIFO_SIZE(FRAME)			(FRAME + SRC_INPUT_MIN * SRC_SCALE_MAX + SRC_OUPUT_JITTER)

#define SRA_FIFO_SIZE(FRAME)			(FRAME + SRA_BLOCK + SRA_MAX_CHG + 1)

//配置通路参数，申请所需buf
bool AudioCoreSourceInit(AudioCoreIO * AudioIO, uint8_t Index);

//清理参数，释放动态buf
void AudioCoreSourceDeinit(uint8_t Index);

//配置通路参数，申请所需buf
bool AudioCoreSinkInit(AudioCoreIO * AudioIO, uint8_t Index);

//清理参数，释放动态buf
void AudioCoreSinkDeinit(uint8_t Index);

//实现audiocore适配层Adapter数据迁移，数据帧监测
void AudioCoreIOLenProcess(void);

//混音通路 同步检测，驱动不同混音通路组合 进入AudioProcessMain
bool AudioCoreSourceSync(void);

//驱动sink通路组合处理
bool AudioCoreSinkSync(void);

//调整音频参数 设置非0值，设置0时保持原值
void AudioCoreSourceChange(uint8_t Index, uint8_t Channels, uint32_t SampleRate);

//调整音频参数 设置非0值，设置0时保持原值
void AudioCoreSinkChange(uint8_t Index, uint8_t Channels, uint32_t SampleRate);

//调整LenGetFunc()对应的最大值  @sample
void AudioCoreSourceDepthChange(uint8_t Index, uint32_t NewDepth);

//调整LenGetFunc()对应的最大值  @sample
void AudioCoreSinkDepthChange(uint8_t Index, uint32_t NewDepth);

//通路已配置，实际以帧buf申请与否为基准
bool AudioCoreSourceIsInit(uint8_t Index);

bool AudioCoreSinkIsInit(uint8_t Index);

//通路微调开关，需init时Adapt类型及其参数配合，通路关闭时不生效。
void AudioCoreSourceAdjust(uint8_t Index, bool OnOff);

//通路微调开关，需init时Adapt类型及其参数配合，通路关闭时不生效。
void AudioCoreSinkAdjust(uint8_t Index, bool OnOff);

//AudioCore.FrameSize[DefaultNet]必须先配置，必定多通路最大值，原因是音效buf共用 @Samples
//此API只改变通路帧长，mix相关通路帧长buf需要应用层先释放，配置此api 后重新申请。
bool AudioCoreFrameSizeSet(MIX_NET Nets, uint16_t Size);

//缺省参数 DefaultNet
uint16_t AudioCoreFrameSizeGet(MIX_NET MixNet);

//audiocore mix的采样率设置, 相关通路带转采样时，此api重新init转采样参数。
void AudioCoreMixSampleRateSet(MIX_NET MixNet, uint32_t SampleRate);

uint32_t AudioCoreMixSampleRateGet(MIX_NET MixNet);

MIX_NET AudioCoreSourceMixNetGet(uint8_t Index);

MIX_NET AudioCoreSinkMixNetGet(uint8_t Index);

//Source此通道，左右声道设置为mute， muted数据已进入Sink，可以关闭
bool AudioCoreSourceReadyForClose(uint8_t Index);

//Sink此通道，左右声道同为mute，muted已进入fifo(未mute时亦为真)。可以关闭通路。
bool AudioCoreSinkReadyForClose(uint8_t Index);

//Sink此通道，左右声道同为mute，muted已至播放(未mute时亦为真)。可以清理fifo。
bool AudioCoreSinkReadyFifoClear(uint8_t Index);

//audiocore左右声道设置为mute的通路， muted数据已进入FIFO，可以关闭
bool AudioCoreMutedForClose(void);

//audiocore左右声道设置为mute的通路，全部已播空至muted数据，可以清理fifo
bool AudioCoreMutedForClear(void);

void AudioFadein(int16_t* pcm_in, uint16_t pcm_length, uint16_t ch);

void AudioFadeout(int16_t* pcm_in, uint16_t pcm_length, uint16_t ch);
#endif /* BT_AUDIO_APP_SRC_INC_AUDIO_CORE_ADAPT_H_ */
