/**
 **************************************************************************************
 * @file    audio_core.c
 * @brief   audio core 
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2016-6-29 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include <nds32_intrinsic.h>
#include "type.h"
#include "freertos.h"
#include "app_config.h"
#include "debug.h"
#include "ctrlvars.h"
#include "audio_effect.h"
#include "main_task.h"
#include "mcu_circular_buf.h"
#include "beep.h"
#include "dma.h"
#include "audio_core_service.h"
#include "audio_core_api.h"
#ifdef CFG_APP_BT_MODE_EN
#include "bt_config.h"
#include "bt_play_api.h"
#include "bt_manager.h"
#ifdef BT_TWS_SUPPORT
#include "bt_tws_api.h"
#include "bt_app_tws.h"
#include "tws_mode.h"
#include "mode_task_api.h"
#endif
#if (BT_HFP_SUPPORT == ENABLE)
#include "bt_hf_api.h"
#endif
#endif
#include "dac_interface.h"
#include "audio_vol.h"


/*******************************************************************************************************************************
 *
 *				 |***GetAdapter***|	  			 |***********CoreProcess***********|			  |***SetAdapter***|
 * ************	 ******************	 **********	 ***********************************  **********  ******************  **********
 *	SourceFIFO*->*SRCFIFO**SRAFIFO*->*PCMFrame*->*PreGainEffect**MixNet**GainEffect*->*PCMFrame*->*SRAFIFO**SRCFIFO*->*SinkFIFO*
 * ************  ******************	 **********	 ***********************************  **********  ******************  **********
 * 				 |*Context*|																			 |*Context*|
 *
 *******************************************************************************************************************************/

typedef enum
{
	AC_RUN_CHECK,//用于检测是否需要暂停任务，如果需要暂停任务，则停留再该状态
	AC_RUN_GET,
	AC_RUN_PROC,
	AC_RUN_PUT,
}AudioCoreRunState;

static AudioCoreRunState AudioState = AC_RUN_CHECK;
AudioCoreContext		AudioCore;

extern uint32_t gSysTick;
extern int32_t tws_get_pcm_delay(void);

void AudioCoreSourcePcmFormatConfig(uint8_t Index, uint16_t Format)
{
	if(Index < AUDIO_CORE_SOURCE_MAX_NUM)
	{
		AudioCore.AudioSource[Index].Channels = Format;
	}
}

void AudioCoreSourceEnable(uint8_t Index)
{
	if(Index < AUDIO_CORE_SOURCE_MAX_NUM)
	{
		AudioCore.AudioSource[Index].Enable = TRUE;
	}
}

void AudioCoreSourceDisable(uint8_t Index)
{
	if(Index < AUDIO_CORE_SOURCE_MAX_NUM)
	{
		AudioCore.AudioSource[Index].Enable = FALSE;
		//AudioCore.AudioSource[Index].LeftCurVol = 0;
		//AudioCore.AudioSource[Index].RightCurVol = 0;
	}
}

bool AudioCoreSourceIsEnable(uint8_t Index)
{
	return AudioCore.AudioSource[Index].Enable;
}

void AudioCoreSourceMute(uint8_t Index, bool IsLeftMute, bool IsRightMute)
{
	if(IsLeftMute)
	{
		AudioCore.AudioSource[Index].LeftMuteFlag = TRUE;
	}
	if(IsRightMute)
	{
		AudioCore.AudioSource[Index].RightMuteFlag = TRUE;
	}
}

void AudioCoreSourceUnmute(uint8_t Index, bool IsLeftUnmute, bool IsRightUnmute)
{
	if(IsLeftUnmute)
	{
		AudioCore.AudioSource[Index].LeftMuteFlag = FALSE;
		AudioCore.AudioSource[Index].LeftCurVol	  = 0; //淡入
	}
	if(IsRightUnmute)
	{
		AudioCore.AudioSource[Index].RightMuteFlag = FALSE;
		AudioCore.AudioSource[Index].RightCurVol   = 0; //淡入
	}
}

void AudioCoreSourceVolSet(uint8_t Index, uint16_t LeftVol, uint16_t RightVol)
{
	AudioCore.AudioSource[Index].LeftVol = LeftVol;
	AudioCore.AudioSource[Index].RightVol = RightVol;
}

void AudioCoreSourceVolGet(uint8_t Index, uint16_t* LeftVol, uint16_t* RightVol)
{
	*LeftVol = AudioCore.AudioSource[Index].LeftCurVol;
	*RightVol = AudioCore.AudioSource[Index].RightCurVol;
}

void AudioCoreSourceConfig(uint8_t Index, AudioCoreSource* Source)
{
	memcpy(&AudioCore.AudioSource[Index], Source, sizeof(AudioCoreSource));
}

void AudioCoreSinkEnable(uint8_t Index)
{
	AudioCore.AudioSink[Index].Enable = TRUE;
}

void AudioCoreSinkDisable(uint8_t Index)
{
	AudioCore.AudioSink[Index].Enable = FALSE;
	AudioCore.AudioSink[Index].LeftCurVol = 0;
	AudioCore.AudioSink[Index].RightCurVol = 0;
}

bool AudioCoreSinkIsEnable(uint8_t Index)
{
	return AudioCore.AudioSink[Index].Enable;
}

void AudioCoreSinkMute(uint8_t Index, bool IsLeftMute, bool IsRightMute)
{
	/*if(IsLeftMute)
	{
		AudioCore.AudioSink[Index].LeftMuteFlag = TRUE;
	}
	if(IsRightMute)
	{
		AudioCore.AudioSink[Index].RightMuteFlag = TRUE;
	}
	*/
}

void AudioCoreSinkUnmute(uint8_t Index, bool IsLeftUnmute, bool IsRightUnmute)
{
	/*if(IsLeftUnmute)
	{
		AudioCore.AudioSink[Index].LeftMuteFlag = FALSE;
	}
	if(IsRightUnmute)
	{
		AudioCore.AudioSink[Index].RightMuteFlag = FALSE;
	}
	*/
}

void AudioCoreSinkVolSet(uint8_t Index, uint16_t LeftVol, uint16_t RightVol)
{
	AudioCore.AudioSink[Index].LeftVol = LeftVol;
	AudioCore.AudioSink[Index].RightVol = RightVol;
}

void AudioCoreSinkVolGet(uint8_t Index, uint16_t* LeftVol, uint16_t* RightVol)
{
	*LeftVol = AudioCore.AudioSink[Index].LeftCurVol;
	*RightVol = AudioCore.AudioSink[Index].RightCurVol;
}

void AudioCoreSinkConfig(uint8_t Index, AudioCoreSink* Sink)
{
	memcpy(&AudioCore.AudioSink[Index], Sink, sizeof(AudioCoreSink));
}


void AudioCoreProcessConfig(AudioCoreProcessFunc AudioEffectProcess)
{
	AudioCore.AudioEffectProcess = AudioEffectProcess;
}

///**
// * @func        AudioCoreConfig
// * @brief       AudioCore参数块，本地化API
// * @param       AudioCoreContext *AudioCoreCt
// * @Output      None
// * @return      None
// * @Others      外部配置的参数块，复制一份到本地
// */
//void AudioCoreConfig(AudioCoreContext *AudioCoreCt)
//{
//	memcpy(&AudioCore, AudioCoreCt, sizeof(AudioCoreContext));
//}

bool AudioCoreInit(void)
{
	DBG("AudioCore init\n");
	return TRUE;
}

void AudioCoreDeinit(void)
{
	AudioState = AC_RUN_CHECK;
}

/**
 * @func        AudioCoreRun
 * @brief       音源拉流->音效处理+混音->推流
 * @param       None
 * @Output      None
 * @return      None
 * @Others      当前由audioCoreservice任务保障此功能有效持续。
 * Record
 */
extern uint32_t 	IsAudioCorePause;
extern uint32_t 	IsAudioCorePauseMsgSend;
extern bool AudioCoreSinkMuted(void);
void AudioProcessMain(void);
__attribute__((optimize("Og")))
void AudioCoreRun(void)
{
	bool ret;
	switch(AudioState)
	{
		case AC_RUN_CHECK:
			if(IsAudioCorePause == TRUE && ((!mainAppCt.gSysVol.MuteFlag) || AudioCoreSinkMuted()))
			{
				if(IsAudioCorePauseMsgSend == TRUE)
				{
					MessageContext		msgSend;
					msgSend.msgId		= MSG_AUDIO_CORE_HOLD;
					MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);

					IsAudioCorePauseMsgSend = FALSE;
				}
				return;
			}
		case AC_RUN_GET:
			AudioCoreIOLenProcess();
			ret = AudioCoreSourceSync();
			if(ret == FALSE)
			{
				return;
			}

		case AC_RUN_PROC:
			AudioProcessMain();
			AudioState = AC_RUN_PUT;

		case AC_RUN_PUT:
			ret = AudioCoreSinkSync();
			if(ret == FALSE)
			{
//				AudioCoreIOLenProcess();
				return;
			}
			AudioState = AC_RUN_CHECK;
			break;
		default:
			break;
	}
}

//音效处理函数，主入口
//将mic通路数据剥离出来统一处理
//mic通路数据和具体模式无关
//提示音通路无音效，剥离后在sink端混音。
void AudioProcessMain(void)
{	
	AudioCoreSourceVolApply();
#ifdef CFG_FUNC_RECORDER_EN
	if(AudioCore.AudioSource[PLAYBACK_SOURCE_NUM].Enable == TRUE)
	{
		if(AudioCore.AudioSource[PLAYBACK_SOURCE_NUM].Channels == 1)
		{
			uint16_t i;
			for(i = SOURCEFRAME(PLAYBACK_SOURCE_NUM) * 2 - 1; i > 0; i--)
			{
				AudioCore.AudioSource[PLAYBACK_SOURCE_NUM].PcmInBuf[i] = AudioCore.AudioSource[PLAYBACK_SOURCE_NUM].PcmInBuf[i / 2];
			}
		}
	}
#endif
	if(AudioCore.AudioSource[APP_SOURCE_NUM].Active == TRUE)////music buff
	{
		#if (BT_HFP_SUPPORT == ENABLE) && defined(CFG_APP_BT_MODE_EN)
		if(GetSystemMode() != ModeBtHfPlay)
		#endif
		{
			if(AudioCore.AudioSource[APP_SOURCE_NUM].Channels == 1)
			{
				uint16_t i;
				for(i = SOURCEFRAME(APP_SOURCE_NUM) * 2 - 1; i > 0; i--)
				{
					AudioCore.AudioSource[APP_SOURCE_NUM].PcmInBuf[i] = AudioCore.AudioSource[APP_SOURCE_NUM].PcmInBuf[i / 2];
				}
			}
		}
	}	
		
#if defined(CFG_FUNC_REMIND_SOUND_EN)
	if(AudioCore.AudioSource[REMIND_SOURCE_NUM].Active == TRUE)////remind buff
	{
		if(AudioCore.AudioSource[REMIND_SOURCE_NUM].Channels == 1)
		{
			uint16_t i;
			for(i = SOURCEFRAME(REMIND_SOURCE_NUM) * 2 - 1; i > 0; i--)
			{
				AudioCore.AudioSource[REMIND_SOURCE_NUM].PcmInBuf[i] = AudioCore.AudioSource[REMIND_SOURCE_NUM].PcmInBuf[i / 2];
			}
		}
	}	
#endif


	if(AudioCore.AudioEffectProcess != NULL)
	{
		AudioCore.AudioEffectProcess((AudioCoreContext*)&AudioCore);
	}
	
    #ifdef CFG_FUNC_BEEP_EN
    if(AudioCore.AudioSink[AUDIO_DAC0_SINK_NUM].Active == TRUE)   ////dacx buff
	{
		Beep(AudioCore.AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf, SINKFRAME(AUDIO_DAC0_SINK_NUM));
	}
    #endif

    AudioCoreSinkVolApply();//调音量
}

//音量淡入淡出					
#define MixerFadeVolume(a, b, c)  	\
    if(a > b + c)		    \
	{						\
		a -= c;				\
	}						\
	else if(a + c < b)	   	\
	{						\
		a += c;				\
	}						\
	else					\
	{						\
		a = b;				\
	}


void AudioCoreSourceVolApply(void)
{
#if 0
	uint32_t i, j;
    
	uint16_t LeftVol, RightVol, VolStep = 4096 / mainAppCt.SamplesPreFrame + 1;

#ifdef CFG_APP_BT_MODE_EN
#if (BT_HFP_SUPPORT == ENABLE)
	if(GetSystemMode() == ModeBtHfPlay)
	{
		AudioCore.AudioSource[0].PreGain = BT_HFP_MIC_DIGIT_GAIN;
		AudioCore.AudioSource[1].PreGain = BT_HFP_INPUT_DIGIT_GAIN;
	}
	else
	{
		AudioCore.AudioSource[0].PreGain = 4095;//0db
		AudioCore.AudioSource[1].PreGain = 4095;
	}
#endif
#endif

#ifdef BT_TWS_SUPPORT
//使用APP的音量作为TWS的音量
	AudioCore.AudioSource[TWS_SOURCE_NUM].LeftVol 		= AudioCore.AudioSource[APP_SOURCE_NUM].LeftVol;
	AudioCore.AudioSource[TWS_SOURCE_NUM].RightVol 		= AudioCore.AudioSource[APP_SOURCE_NUM].RightVol;
#endif

	for(j=0; j<AUDIO_CORE_SOURCE_MAX_NUM; j++)
	{
		if(!AudioCore.AudioSource[j].Active)
		{
			continue;
		}
		if(AudioCore.AudioSource[j].LeftMuteFlag == TRUE)
		{
			LeftVol = 0;
		}
		else
		{
			LeftVol = AudioCore.AudioSource[j].LeftVol;
		}
		if(AudioCore.AudioSource[j].RightMuteFlag == TRUE)
		{
			RightVol = 0;
		}
		else
		{
			RightVol = AudioCore.AudioSource[j].RightVol;
		}

		if(AudioCore.AudioSource[j].PcmFormat == 2)//立体声
		{
			for(i=0; i<mainAppCt.SamplesPreFrame; i++)
			{
				AudioCore.AudioSource[j].PcmInBuf[2 * i + 0] = __nds32__clips((((((int32_t)AudioCore.AudioSource[j].PcmInBuf[2 * i + 0]) * AudioCore.AudioSource[j].LeftCurVol + 2048) >> 12) * AudioCore.AudioSource[j].PreGain + 2048) >> 12, (16)-1);//__SSAT((((((int32_t)AudioCore.AudioSource[j].PcmInBuf[2 * i] * AudioCore.AudioSource[j].LeftCurVol) >> 12) * AudioCore.AudioSource[j].PreGain) >> 12), 16);
				AudioCore.AudioSource[j].PcmInBuf[2 * i + 1] = __nds32__clips((((((int32_t)AudioCore.AudioSource[j].PcmInBuf[2 * i + 1]) * AudioCore.AudioSource[j].RightCurVol + 2048) >> 12) * AudioCore.AudioSource[j].PreGain + 2048) >> 12, (16)-1);//__SSAT((((((int32_t)AudioCore.AudioSource[j].PcmInBuf[2 * i + 1] * AudioCore.AudioSource[j].RightCurVol) >> 12) * AudioCore.AudioSource[j].PreGain) >> 12), 16);
				
				MixerFadeVolume(AudioCore.AudioSource[j].LeftCurVol, LeftVol, VolStep);
				MixerFadeVolume(AudioCore.AudioSource[j].RightCurVol, RightVol, VolStep);
			}
		}
		else
		{
			for(i=0; i<mainAppCt.SamplesPreFrame; i++)
			{
				AudioCore.AudioSource[j].PcmInBuf[i] = __nds32__clips((((((int32_t)AudioCore.AudioSource[j].PcmInBuf[i]) * AudioCore.AudioSource[j].LeftCurVol + 2048) >> 12) * AudioCore.AudioSource[j].PreGain + 2048) >> 12, (16)-1);//__SSAT((((((int32_t)AudioCore.AudioSource[j].PcmInBuf[i] * AudioCore.AudioSource[j].LeftCurVol) >> 12) * AudioCore.AudioSource[j].PreGain) >> 12), 16);
				MixerFadeVolume(AudioCore.AudioSource[j].LeftCurVol, LeftVol, VolStep);
			}
		}
	}
#endif
}

void AudioCoreAppSourceVolApply(uint16_t Source,int16_t *pcm_in,uint16_t n,uint16_t Channel)
{

#if 1
	uint32_t i, j;
    
	uint16_t LeftVol, RightVol,	LeftVolStep, RightVolStep;

#ifdef CFG_APP_BT_MODE_EN
#if (BT_HFP_SUPPORT == ENABLE)
	if(GetSystemMode() == ModeBtHfPlay)
	{
		AudioCore.AudioSource[0].PreGain = BT_HFP_MIC_DIGIT_GAIN;
		AudioCore.AudioSource[1].PreGain = BT_HFP_INPUT_DIGIT_GAIN;
	}
	else
	{
		AudioCore.AudioSource[0].PreGain = 4095;//0db
		AudioCore.AudioSource[1].PreGain = 4095;
	}
#endif
#endif

    if(pcm_in == NULL) return;
	
	j = Source;

	if(!AudioCore.AudioSource[j].Active)
	{
		return;
	}

	if(AudioCore.AudioSource[j].LeftMuteFlag == TRUE)
	{
		LeftVol = 0;
	}
	else
	{
		LeftVol = AudioCore.AudioSource[j].LeftVol;
	}
	if(AudioCore.AudioSource[j].RightMuteFlag == TRUE)
	{
		RightVol = 0;
	}
	else
	{
		RightVol = AudioCore.AudioSource[j].RightVol;
	}

	LeftVolStep = LeftVol > AudioCore.AudioSource[j].LeftCurVol ? (LeftVol - AudioCore.AudioSource[j].LeftCurVol) : (AudioCore.AudioSource[j].LeftCurVol - LeftVol);
	LeftVolStep = LeftVolStep / SOURCEFRAME(j) + (LeftVolStep % SOURCEFRAME(j) ? 1 : 0);
	RightVolStep = RightVol > AudioCore.AudioSource[j].RightCurVol ? (RightVol - AudioCore.AudioSource[j].RightCurVol) : (AudioCore.AudioSource[j].RightCurVol - RightVol);
	RightVolStep = RightVolStep / SOURCEFRAME(j) + (RightVolStep % SOURCEFRAME(j) ? 1 : 0);

#ifdef CFG_AUDIO_WIDTH_24BIT
	if( AudioCore.AudioSource[j].BitWidth == PCM_DATA_24BIT_WIDTH //24bit 数据
	  || AudioCore.AudioSource[j].BitWidthConvFlag // 已在AudioCoreSourceGet扩充到24bit
#ifdef BT_TWS_SUPPORT
	  //app_source通道传入的是AudioSource[TWS_SOURCE_NUM].PcmInBuf
	  || (j != TWS_SOURCE_NUM && pcm_in == (int16_t *)AudioCore.AudioSource[TWS_SOURCE_NUM].PcmInBuf && AudioCore.AudioSource[TWS_SOURCE_NUM].BitWidthConvFlag)
#endif
     )
	{
		int32_t *pcm = (int32_t *)pcm_in;
		if(Channel == 2)//立体声
		{
			for(i=0; i<SOURCEFRAME(j); i++)
			{
				pcm[2 * i + 0] = __nds32__clips((((((int64_t)pcm[2 * i + 0]) * AudioCore.AudioSource[j].LeftCurVol + 2048) >> 12) * AudioCore.AudioSource[j].PreGain + 2048) >> 12, (24)-1);
				pcm[2 * i + 1] = __nds32__clips((((((int64_t)pcm[2 * i + 1]) * AudioCore.AudioSource[j].RightCurVol + 2048) >> 12) * AudioCore.AudioSource[j].PreGain + 2048) >> 12, (24)-1);

				MixerFadeVolume(AudioCore.AudioSource[j].LeftCurVol, LeftVol, LeftVolStep);
				MixerFadeVolume(AudioCore.AudioSource[j].RightCurVol, RightVol, RightVolStep);
			}
		}
		else
		{
			for(i=0; i<SOURCEFRAME(j); i++)
			{
				pcm[i] = __nds32__clips((((((int64_t)pcm[i]) * AudioCore.AudioSource[j].LeftCurVol + 2048) >> 12) * AudioCore.AudioSource[j].PreGain + 2048) >> 12, (24)-1);
				MixerFadeVolume(AudioCore.AudioSource[j].LeftCurVol, LeftVol, LeftVolStep);
			}
		}
	}
	else
#endif
	{
		if(Channel == 2)//立体声
		{
			for(i=0; i<SOURCEFRAME(j); i++)
			{
				pcm_in[2 * i + 0] = __nds32__clips((((((int32_t)pcm_in[2 * i + 0]) * AudioCore.AudioSource[j].LeftCurVol + 2048) >> 12) * AudioCore.AudioSource[j].PreGain + 2048) >> 12, (16)-1);
				pcm_in[2 * i + 1] = __nds32__clips((((((int32_t)pcm_in[2 * i + 1]) * AudioCore.AudioSource[j].RightCurVol + 2048) >> 12) * AudioCore.AudioSource[j].PreGain + 2048) >> 12, (16)-1);

				MixerFadeVolume(AudioCore.AudioSource[j].LeftCurVol, LeftVol, LeftVolStep);
				MixerFadeVolume(AudioCore.AudioSource[j].RightCurVol, RightVol, RightVolStep);
			}
		}
		else
		{
			for(i=0; i<SOURCEFRAME(j); i++)
			{
				pcm_in[i] = __nds32__clips((((((int32_t)pcm_in[i]) * AudioCore.AudioSource[j].LeftCurVol + 2048) >> 12) * AudioCore.AudioSource[j].PreGain + 2048) >> 12, (16)-1);
				MixerFadeVolume(AudioCore.AudioSource[j].LeftCurVol, LeftVol, LeftVolStep);
			}
		}
	}
#endif
}

void AudioCoreSinkVolApply(void)
{
	uint32_t i;
	uint8_t j;

	uint16_t LeftVol, RightVol,	LeftVolStep, RightVolStep;

	for(j=0; j<AUDIO_CORE_SINK_MAX_NUM; j++)
	{

		if(AudioCore.AudioSink[j].Active == TRUE)
		{
			if(AudioCore.AudioSink[j].LeftMuteFlag == TRUE || mainAppCt.gSysVol.MuteFlag)
			{
				LeftVol = 0;
			}
			else
			{
				LeftVol = AudioCore.AudioSink[j].LeftVol;
			}
			if(AudioCore.AudioSink[j].RightMuteFlag == TRUE || mainAppCt.gSysVol.MuteFlag)
			{
				RightVol = 0;
			}
			else
			{
				RightVol = AudioCore.AudioSink[j].RightVol;
			}

			LeftVolStep = LeftVol > AudioCore.AudioSink[j].LeftCurVol ? (LeftVol - AudioCore.AudioSink[j].LeftCurVol) : (AudioCore.AudioSink[j].LeftCurVol - LeftVol);
			LeftVolStep = LeftVolStep / SINKFRAME(j) + (LeftVolStep % SINKFRAME(j) ? 1 : 0);
			RightVolStep = RightVol > AudioCore.AudioSink[j].RightCurVol ? (RightVol - AudioCore.AudioSink[j].RightCurVol) : (AudioCore.AudioSink[j].RightCurVol - RightVol);
			RightVolStep = RightVolStep / SINKFRAME(j) + (RightVolStep % SINKFRAME(j) ? 1 : 0);
#ifdef CFG_AUDIO_WIDTH_24BIT
			if(AudioCore.AudioSink[j].BitWidth == PCM_DATA_24BIT_WIDTH //24bit 数据
			 //|| AudioCore.AudioSink[j].BitWidthConvFlag     //在AudioCoreSinkSet扩充到24bit,此时还是16bit
				)
			{
				if(AudioCore.AudioSink[j].Channels == 2)
				{
					for(i=0; i < SINKFRAME(j); i++)
					{
						AudioCore.AudioSink[j].PcmOutBuf[2 * i + 0] = __nds32__clips((((int64_t)AudioCore.AudioSink[j].PcmOutBuf[2 * i + 0]) * AudioCore.AudioSink[j].LeftCurVol + 2048) >> 12, (24)-1);
						AudioCore.AudioSink[j].PcmOutBuf[2 * i + 1] = __nds32__clips((((int64_t)AudioCore.AudioSink[j].PcmOutBuf[2 * i + 1]) * AudioCore.AudioSink[j].RightCurVol + 2048) >> 12, (24)-1);

						MixerFadeVolume(AudioCore.AudioSink[j].LeftCurVol,LeftVol, LeftVolStep);
						MixerFadeVolume(AudioCore.AudioSink[j].RightCurVol,RightVol, RightVolStep);
					}
				}
				else if(AudioCore.AudioSink[j].Channels == 1)
				{
					for(i=0; i<SINKFRAME(j); i++)
					{
						AudioCore.AudioSink[j].PcmOutBuf[i] = __nds32__clips((((int64_t)AudioCore.AudioSink[j].PcmOutBuf[i]) * AudioCore.AudioSink[j].LeftCurVol + 2048) >> 12, (24)-1);

						MixerFadeVolume(AudioCore.AudioSink[j].LeftCurVol,LeftVol, LeftVolStep);
					}
				}
			}
			else
#endif
			{
				int16_t * PcmOutBuf = (int16_t *)AudioCore.AudioSink[j].PcmOutBuf;
				if(AudioCore.AudioSink[j].Channels == 2)
				{
					for(i=0; i < SINKFRAME(j); i++)
					{
						PcmOutBuf[2 * i + 0] = __nds32__clips((((int32_t)PcmOutBuf[2 * i + 0]) * AudioCore.AudioSink[j].LeftCurVol + 2048) >> 12, (16)-1);
						PcmOutBuf[2 * i + 1] = __nds32__clips((((int32_t)PcmOutBuf[2 * i + 1]) * AudioCore.AudioSink[j].RightCurVol + 2048) >> 12, (16)-1);

						MixerFadeVolume(AudioCore.AudioSink[j].LeftCurVol,LeftVol, LeftVolStep);
						MixerFadeVolume(AudioCore.AudioSink[j].RightCurVol,RightVol, RightVolStep);
					}
				}
				else if(AudioCore.AudioSink[j].Channels == 1)
				{
					for(i=0; i<SINKFRAME(j); i++)
					{
						PcmOutBuf[i] = __nds32__clips((((int32_t)PcmOutBuf[i]) * AudioCore.AudioSink[j].LeftCurVol + 2048) >> 12, (16)-1);

						MixerFadeVolume(AudioCore.AudioSink[j].LeftCurVol,LeftVol, LeftVolStep);
					}
				}
			}
		}
	}
}
