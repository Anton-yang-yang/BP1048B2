#include <string.h>
#include <nds32_intrinsic.h>
#include "math.h"
#include "debug.h"
#include "app_config.h"
#include "rtos_api.h"
#include "audio_effect_api.h"
#include "audio_effect_library.h"
#include "audio_core_api.h"
#include "main_task.h"
#include "audio_effect.h"
#include "tws_mode.h"
#include "bt_tws_api.h"
#include "remind_sound.h"
#include "ctrlvars.h"
#include "comm_param.h"
#include "bt_manager.h"
#include "mode_task.h"
#include "bt_hf_api.h"
#include "audio_effect_user.h"

#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
extern uint32_t music_eq_mode_unit;
#endif
extern PCM_DATA_TYPE * pcm_buf_1;
extern PCM_DATA_TYPE * pcm_buf_2;
extern PCM_DATA_TYPE * pcm_buf_3;
extern PCM_DATA_TYPE * pcm_buf_4;
extern PCM_DATA_TYPE * pcm_buf_5;

#ifdef CFG_AUDIO_WIDTH_24BIT
#define AUDIO_WIDTH    		  24
#else
#define AUDIO_WIDTH    		  16
#endif

void AudioEffectMutex_Lock(void)
{
	#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexLock(AudioEffectMutex);
	}
	#endif
}

void AudioEffectMutex_Unlock(void)
{
	#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexUnlock(AudioEffectMutex);
	}
	#endif
}

#ifdef CFG_FUNC_AUDIO_EFFECT_EN
__attribute__((optimize("Og")))
void AudioMusicProcess(AudioCoreContext *pAudioCore)
{
	uint16_t i;
	int16_t  s;
	uint16_t n = AudioCoreFrameSizeGet(AudioCore.CurrentMix);
	EffectNode*  pNode 			= NULL;
#ifdef BT_TWS_SUPPORT
	int16_t *music_in    		= NULL;//pBuf->music_in;///music input
	int16_t *tws_out    		= NULL;
	PCM_DATA_TYPE *tws_in		= NULL;
#else
	PCM_DATA_TYPE *music_in    	= NULL;//pBuf->music_in;///music input
#endif
	PCM_DATA_TYPE *monitor_out	= NULL;
	PCM_DATA_TYPE *record_out   = NULL;//pBuf->dacx_out;
	PCM_DATA_TYPE *remind_in    = NULL;//pBuf->remind_in;
	PCM_DATA_TYPE *i2s_out      = NULL;//pBuf->i2s0_out;
#ifdef CFG_FUNC_RECORDER_EN
	int16_t *local_rec_out  	= NULL;//pBuf->rec_out;
#endif

#ifdef CFG_APP_USB_AUDIO_MODE_EN
	int16_t *mic_pcm			= NULL;//for Usb audio out
	int16_t *usb_out			= NULL;//usb 上行

	if(pAudioCore->AudioSource[MIC_SOURCE_NUM].Active == TRUE)////mic buff
	{
		mic_pcm = (int16_t *)pAudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf;//for usb audio out
	}
	if(pAudioCore->AudioSink[USB_AUDIO_SINK_NUM].Active == TRUE)   ////Usb out buff
	{
		usb_out = (int16_t *)pAudioCore->AudioSink[USB_AUDIO_SINK_NUM].PcmOutBuf;
		if(mic_pcm)
		{
			for(s = 0; s < n; s++)
			{
				usb_out[2*s + 0] = mic_pcm[2*s + 0];
				usb_out[2*s + 1] = mic_pcm[2*s + 1];
			}
		}
	}
#endif

#ifdef CFG_FUNC_RECORDER_EN
	if(GetSystemMode() == ModeCardPlayBack || GetSystemMode() == ModeUDiskPlayBack)// || GetSystemMode() == ModeFlashFsPlayBack)
	{
		if(pAudioCore->AudioSource[PLAYBACK_SOURCE_NUM].Enable == TRUE)
		{
			app_in = pAudioCore->AudioSource[PLAYBACK_SOURCE_NUM].PcmInBuf;// include usb/sd source
		}
	}
	else
#endif
	{
		if(pAudioCore->AudioSource[APP_SOURCE_NUM].Active == TRUE)////music effect in
		{
			#ifdef BT_TWS_SUPPORT
			music_in = (int16_t *)pAudioCore->AudioSource[APP_SOURCE_NUM].PcmInBuf;
			#else
			music_in = pAudioCore->AudioSource[APP_SOURCE_NUM].PcmInBuf;
			#endif
		}
	}
	
	if(pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].Active == TRUE)   ////dac0 buff
	{
		monitor_out = pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf;
	}
	
#ifdef CFG_RES_AUDIO_DACX_EN
	if(pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].Active == TRUE)   ////dacx buff
	{
		record_out = pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].PcmOutBuf;
	}
#endif

#if defined(CFG_FUNC_REMIND_SOUND_EN)
	if(pAudioCore->AudioSource[REMIND_SOURCE_NUM].Active == TRUE)////remind buff
	{
		remind_in = pAudioCore->AudioSource[REMIND_SOURCE_NUM].PcmInBuf;
	}
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN
	if(pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].Active == TRUE) ////i2s buff
	{
		i2s_out = pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].PcmOutBuf;
	}
#endif

#ifdef CFG_FUNC_RECORDER_EN
	if(pAudioCore->AudioSink[AUDIO_RECORDER_SINK_NUM].Active == TRUE)
	{
		local_rec_out = pAudioCore->AudioSink[AUDIO_RECORDER_SINK_NUM].PcmOutBuf;
	}
#endif

#ifdef CFG_RES_AUDIO_I2S0IN_EN
	#if (CFG_FUNC_I2S0_MIX_EN == 1)
	int16_t 		*i2s0_in  	= NULL;
	#else
	PCM_DATA_TYPE 	*i2s0_in  	= NULL;
	#endif

	if(pAudioCore->AudioSource[I2S0_SOURCE_NUM].Active == TRUE)
	{
		i2s0_in = pAudioCore->AudioSource[I2S0_SOURCE_NUM].PcmInBuf;
	}

	#if (CFG_FUNC_I2S0_MIX_EN == 1)
	if(i2s0_in)
	{
		if(music_in)
		{
			for(s = 0; s < n * 2; s++)
			{
				music_in[s] = __nds32__clips((((int32_t)i2s0_in[s] + (int32_t)music_in[s])), 16-1);
			}
		}
		else
			music_in = i2s0_in;
	}
	#endif
#endif
#ifdef CFG_RES_AUDIO_I2S1IN_EN
	#if (CFG_FUNC_I2S1_MIX_EN == 1)
	int16_t 		*i2s1_in  	= NULL;
	#else
	PCM_DATA_TYPE 	*i2s1_in  	= NULL;
	#endif

	if(pAudioCore->AudioSource[I2S1_SOURCE_NUM].Active == TRUE)
	{
		i2s1_in = pAudioCore->AudioSource[I2S1_SOURCE_NUM].PcmInBuf;
	}	

	#if (CFG_FUNC_I2S1_MIX_EN == 1)
	if(i2s1_in)
	{
		if(music_in)
		{
			for(s = 0; s < n * 2; s++)
			{
				music_in[s] = __nds32__clips((((int32_t)i2s1_in[s] + (int32_t)music_in[s])), 16-1);
			}
		}
		else
			music_in = i2s1_in;
	}
	#endif
#endif

#ifdef CFG_RES_AUDIO_I2S0OUT_EN
	PCM_DATA_TYPE 	*i2s0_out  	= NULL;
	if(pAudioCore->AudioSink[AUDIO_I2S0_OUT_SINK_NUM].Active == TRUE)
	{
		i2s0_out = pAudioCore->AudioSink[AUDIO_I2S0_OUT_SINK_NUM].PcmOutBuf;
	}	
	if (i2s0_out)
	{
		memset(i2s0_out, 0, n * sizeof(PCM_DATA_TYPE) * 2);
	}
#endif
#ifdef CFG_RES_AUDIO_I2S1OUT_EN
	PCM_DATA_TYPE 	*i2s1_out  	= NULL;
	if(pAudioCore->AudioSink[AUDIO_I2S1_OUT_SINK_NUM].Active == TRUE)
	{
		i2s1_out = pAudioCore->AudioSink[AUDIO_I2S1_OUT_SINK_NUM].PcmOutBuf;
	}	
	if (i2s1_out)
	{
		memset(i2s1_out, 0, n * sizeof(PCM_DATA_TYPE) * 2);
	}
#endif

#ifdef BT_TWS_SUPPORT
	if(pAudioCore->AudioSource[TWS_SOURCE_NUM].Active == TRUE)
	{
		tws_in = pAudioCore->AudioSource[TWS_SOURCE_NUM].PcmInBuf;
	}
	if(pAudioCore->AudioSink[TWS_SINK_NUM].Active == TRUE)
	{
		tws_out = (int16_t*)pAudioCore->AudioSink[TWS_SINK_NUM].PcmOutBuf;
	}
    if(tws_out)
	{
    	memset(tws_out, 0, n * sizeof(PCM_DATA_TYPE) * 2);
    }
#endif

    if(monitor_out){
    	memset(monitor_out, 0, n * sizeof(PCM_DATA_TYPE) * 2);
    }
    if(record_out){
    	memset(record_out, 0, n * sizeof(PCM_DATA_TYPE) * 2);
    }
    if(i2s_out){
    	memset(i2s_out, 0, n * sizeof(PCM_DATA_TYPE) * 2);//mono*2 stereo*4
	}
	
#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
	if((music_in)&&(mainAppCt.EqModeBak != mainAppCt.EqMode))
	{		
		EqModeSet(mainAppCt.EqMode);
		mainAppCt.EqModeBak = mainAppCt.EqMode;
	}
#endif

#ifdef BT_TWS_SUPPORT
#if (CFG_EFFECT_MUSIC_MASTER == 0)//主从各自做各自的音效
	if(music_in)
	{
		AudioCoreAppSourceVolApply(APP_SOURCE_NUM, music_in, n, 2);//增益控制在信号能量检测之后处理
#ifdef CFG_FUNC_RECORDER_EN
		if(GetSystemMode() == ModeCardPlayBack || GetSystemMode() == ModeUDiskPlayBack)// || GetSystemMode() == ModeFlashFsPlayBack)
		{
			AudioCoreAppSourceVolApply(PLAYBACK_SOURCE_NUM, music_in, n, 2);
		}
		if(local_rec_out)
		{
			for(s = 0; s < n; s++)
			{
				local_rec_out[2*s + 0] = music_in[2*s + 0];
				local_rec_out[2*s + 1] = music_in[2*s + 1];
			}
		}
#endif
		AudioEffectMutex_Lock();

		//app通路数字预增益处理
		for(i=0; i<AUDIO_EFFECT_NODE_NUM; i++)
		{
			pNode = &gEffectNodeList[0].EffectNode[i];//伴奏使用第0组音效列表
			if((pNode->Enable == FALSE) || (pNode->EffectUnit == NULL))
			{
				continue;
			}
			pNode->FuncAudioEffect(pNode->EffectUnit, music_in, music_in, n);
		}

		AudioEffectMutex_Unlock();
	}
	if(tws_in)//音效用于tws_in通路
	{
		AudioEffectMutex_Lock();

		for(i=0; i<AUDIO_EFFECT_NODE_NUM; i++)
		{
			pNode = &gEffectNodeList[1].EffectNode[i];//伴奏使用第1组音效列表
			if((pNode->Enable == FALSE) || (pNode->EffectUnit == NULL))
			{
				if(pNode->NodeType == NodeType_Bifurcate)
				{
					//系统增益控制，根据从工经验，打散放到音效处理中来处理
					AudioCoreAppSourceVolApply(TWS_SOURCE_NUM, (int16_t *)tws_in, n, 2);//增益控制在信号能量检测之后处理
					//注意增益控制处理在代码中放的位置一定要注意，需要根据音效列表来合理放置
					if(record_out)
					{
						memcpy(record_out, tws_in, n * sizeof(PCM_DATA_TYPE) * 2);//降噪音效之后第一个分叉节点
					}
				}
				continue;
			}

			pNode->FuncAudioEffect(pNode->EffectUnit, (int16_t *)tws_in, (int16_t *)tws_in, n);
			//分叉节点处理
			if(pNode->NodeType == NodeType_Bifurcate)
			{
				//系统增益控制，根据从工经验，打散放到音效处理中来处理
				AudioCoreAppSourceVolApply(TWS_SOURCE_NUM, (int16_t *)tws_in, n, 2);//增益控制在信号能量检测之后处理
				//注意增益控制处理在代码中放的位置一定要注意，需要根据音效列表来合理放置
				if(record_out)
				{
					memcpy(record_out, tws_in, n * sizeof(PCM_DATA_TYPE) * 2);//降噪音效之后第一个分叉节点
				}
			}
		}

		AudioEffectMutex_Unlock();
	}
#else//CFG_EFFECT_MUSIC_MASTER == 0//主做音效，从无音效
//伴奏信号音效处理
	if(music_in)
	{
#ifdef CFG_FUNC_RECORDER_EN
		if(GetSystemMode() == ModeCardPlayBack || GetSystemMode() == ModeUDiskPlayBack)// || GetSystemMode() == ModeFlashFsPlayBack)
		{
			AudioCoreAppSourceVolApply(PLAYBACK_SOURCE_NUM, music_in, n, 2);
		}
		if(local_rec_out)
		{
			for(s = 0; s < n; s++)
			{
				local_rec_out[2*s + 0] = music_in[2*s + 0];
				local_rec_out[2*s + 1] = music_in[2*s + 1];
			}
		}
#endif

		if(tws_get_role() == BT_TWS_MASTER)
		{
			AudioEffectMutex_Lock();

			//app通路数字预增益处理
			for(i=0; i<AUDIO_EFFECT_NODE_NUM; i++)
			{
				pNode = &gEffectNodeList[0].EffectNode[i];//伴奏使用第0组音效列表
				if((pNode->Enable == FALSE) || (pNode->EffectUnit == NULL))
				{
					continue;
				}
				pNode->FuncAudioEffect(pNode->EffectUnit, music_in, music_in, n);
			}
			AudioEffectMutex_Unlock();
		}
	}

	if(tws_in)
	{
		if(tws_get_role() == BT_TWS_MASTER)
		{
			AudioEffectMutex_Lock();

			for(i=0; i<AUDIO_EFFECT_NODE_NUM; i++)
			{
				pNode = &gEffectNodeList[1].EffectNode[i];//伴奏使用第1组音效列表
				if((pNode->Enable == FALSE) || (pNode->EffectUnit == NULL))
				{
					if(pNode->NodeType == NodeType_Bifurcate)
					{
						//系统增益控制，根据从工经验，打散放到音效处理中来处理
						AudioCoreAppSourceVolApply(APP_SOURCE_NUM, (int16_t *)tws_in, n, 2);//增益控制在信号能量检测之后处理
						//注意增益控制处理在代码中放的位置一定要注意，需要根据音效列表来合理放置
						if(record_out)
						{
							memcpy(record_out, tws_in, n * sizeof(PCM_DATA_TYPE) * 2);//降噪音效之后第一个分叉节点
						}
					}
					continue;
				}

				pNode->FuncAudioEffect(pNode->EffectUnit, tws_in, tws_in, n);
				//分叉节点处理
				if(pNode->NodeType == NodeType_Bifurcate)
				{
					//系统增益控制，根据从工经验，打散放到音效处理中来处理
					AudioCoreAppSourceVolApply(APP_SOURCE_NUM, (int16_t *)tws_in, n, 2);//增益控制在信号能量检测之后处理
					//注意增益控制处理在代码中放的位置一定要注意，需要根据音效列表来合理放置
					if(record_out)
					{
						memcpy(record_out, tws_in, n * sizeof(PCM_DATA_TYPE) * 2);//降噪音效之后第一个分叉节点
					}
				}
			}

			AudioEffectMutex_Unlock();
		}
		else//slave
		{
			//do nothing
			AudioCoreAppSourceVolApply(APP_SOURCE_NUM, (int16_t *)tws_in, n, 2);//增益控制在信号能量检测之后处理
		}
	}
#endif//CFG_EFFECT_MUSIC_MASTER == 0//主从各自做各自的音效
#else//BT_TWS_SUPPORT
	if(music_in)
	{
#ifdef CFG_FUNC_RECORDER_EN
		if(GetSystemMode() == ModeCardPlayBack || GetSystemMode() == ModeUDiskPlayBack)// || GetSystemMode() == ModeFlashFsPlayBack)
		{
			AudioCoreAppSourceVolApply(PLAYBACK_SOURCE_NUM, music_in, n, 2);
		}
		if(local_rec_out)
		{
			for(s = 0; s < n; s++)
			{
				local_rec_out[2*s + 0] = music_in[2*s + 0];
				local_rec_out[2*s + 1] = music_in[2*s + 1];
			}
		}
#endif
		AudioEffectMutex_Lock();

		//app通路数字预增益处理
		for(i=0; i<AUDIO_EFFECT_NODE_NUM; i++)
		{
			pNode = &gEffectNodeList[0].EffectNode[i];//伴奏使用第0组音效列表
			if((pNode->Enable == FALSE) || (pNode->EffectUnit == NULL))
			{
				continue;
			}
			pNode->FuncAudioEffect(pNode->EffectUnit, (int16_t *)music_in, (int16_t *)music_in, n);
		}

		for(i=0; i<AUDIO_EFFECT_NODE_NUM; i++)
		{
			pNode = &gEffectNodeList[1].EffectNode[i];//伴奏使用第1组音效列表
			if((pNode->Enable == FALSE) || (pNode->EffectUnit == NULL))
			{
				if(pNode->NodeType == NodeType_Bifurcate)
				{
					//系统增益控制，根据从工经验，打散放到音效处理中来处理
					AudioCoreAppSourceVolApply(APP_SOURCE_NUM, (int16_t *)music_in, n, 2);//增益控制在信号能量检测之后处理
					//注意增益控制处理在代码中放的位置一定要注意，需要根据音效列表来合理放置
					if(record_out)
					{
						memcpy(record_out, music_in, n * sizeof(PCM_DATA_TYPE) * 2);//降噪音效之后第一个分叉节点
					}
				}
				continue;
			}

			pNode->FuncAudioEffect(pNode->EffectUnit, (int16_t *)music_in, (int16_t *)music_in, n);
			//分叉节点处理
			if(pNode->NodeType == NodeType_Bifurcate)
			{
				//系统增益控制，根据从工经验，打散放到音效处理中来处理
				AudioCoreAppSourceVolApply(APP_SOURCE_NUM, (int16_t *)music_in, n, 2);//增益控制在信号能量检测之后处理
				//注意增益控制处理在代码中放的位置一定要注意，需要根据音效列表来合理放置
				if(record_out)
				{
					memcpy(record_out, music_in, n * sizeof(PCM_DATA_TYPE) * 2);//降噪音效之后第一个分叉节点
				}
			}
		}

		AudioEffectMutex_Unlock();
	}
#endif//BT_TWS_SUPPORT

	//DAC0立体声监听音效处理
	if(monitor_out || i2s_out)
	{
//		if(monitor_out == NULL)
//			monitor_out = i2s_out;
#ifdef CFG_FUNC_REMIND_SOUND_EN
		if(remind_in)
		{
			//系统增益控制，根据从工经验，打散放到音效处理中来处理
			AudioCoreAppSourceVolApply(REMIND_SOURCE_NUM, (int16_t *)remind_in, n, 2);

#ifdef BT_TWS_SUPPORT
			if((tws_out) && (music_in))
			{
				for(s = 0; s < n; s++)
				{
					tws_out[2*s + 0] = music_in[2*s + 0];
					tws_out[2*s + 1] = music_in[2*s + 1];
				}
			}
			if(tws_in)
			{
				for(s = 0; s < n; s++)
				{
					monitor_out[2*s + 0] = __nds32__clips((((int32_t)tws_in[2*s + 0] + (int32_t)remind_in[2*s + 0])), AUDIO_WIDTH - 1);
					monitor_out[2*s + 1] = __nds32__clips((((int32_t)tws_in[2*s + 1] + (int32_t)remind_in[2*s + 1])), AUDIO_WIDTH - 1);
				}
			}
#else
			if(music_in)
			{
				for(s = 0; s < n; s++)
				{
					monitor_out[2*s + 0] = __nds32__clips((((int32_t)music_in[2*s + 0] + (int32_t)remind_in[2*s + 0])), AUDIO_WIDTH - 1);
					monitor_out[2*s + 1] = __nds32__clips((((int32_t)music_in[2*s + 1] + (int32_t)remind_in[2*s + 1])), AUDIO_WIDTH - 1);
				}
			}
			else
			{
				for(s = 0; s < n; s++)
				{
					monitor_out[2*s + 0] = remind_in[2*s + 0];
					monitor_out[2*s + 1] = remind_in[2*s + 1];
				}
			}
#endif//BT_TWS_SUPPORT
		}
		else
#endif//CFG_FUNC_REMIND_SOUND_EN
		{
#ifdef BT_TWS_SUPPORT
			if((tws_out) && (music_in))
			{
				for(s = 0; s < n; s++)
				{
					tws_out[2*s + 0] = music_in[2*s + 0];
					tws_out[2*s + 1] = music_in[2*s + 1];
				}
			}
			if(tws_in)
			{
				for(s = 0; s < n; s++)
				{
					monitor_out[2*s + 0] = tws_in[2*s + 0];
					monitor_out[2*s + 1] = tws_in[2*s + 1];
				}
			}
#else
			if(music_in)
			{
				for(s = 0; s < n; s++)
				{
					monitor_out[2*s + 0] = music_in[2*s + 0];
					monitor_out[2*s + 1] = music_in[2*s + 1];
				}
			}
#endif//BT_TWS_SUPPORT
		}

		#if defined(CFG_RES_AUDIO_I2S0IN_EN)&&(CFG_FUNC_I2S0_MIX_EN == 0)
		if(i2s0_in)
		{
			for(s = 0; s < n; s++)
			{
				monitor_out[2*s + 0] = __nds32__clips((((int32_t)monitor_out[2*s + 0] + (int32_t)i2s0_in[2*s + 0])), AUDIO_WIDTH - 1);
				monitor_out[2*s + 1] = __nds32__clips((((int32_t)monitor_out[2*s + 1] + (int32_t)i2s0_in[2*s + 1])), AUDIO_WIDTH - 1);
			}
		}		
		#endif

		#if defined(CFG_RES_AUDIO_I2S1IN_EN)&&(CFG_FUNC_I2S1_MIX_EN == 0)
		if(i2s1_in)
		{
			for(s = 0; s < n; s++)
			{
				monitor_out[2*s + 0] = __nds32__clips((((int32_t)monitor_out[2*s + 0] + (int32_t)i2s1_in[2*s + 0])), AUDIO_WIDTH-1);
				monitor_out[2*s + 1] = __nds32__clips((((int32_t)monitor_out[2*s + 1] + (int32_t)i2s1_in[2*s + 1])), AUDIO_WIDTH-1);
			}
		}			
		#endif

		#ifdef CFG_RES_AUDIO_I2S0OUT_EN
		if (i2s0_out)
		{
			for(s = 0; s < n; s++)
			{
				i2s0_out[2*s + 0] = monitor_out[2*s + 0];
				i2s0_out[2*s + 1] = monitor_out[2*s + 1];
			}
		}
		#endif

		#ifdef CFG_RES_AUDIO_I2S1OUT_EN
		if (i2s1_out)
		{
			for(s = 0; s < n; s++)
			{
				i2s1_out[2*s + 0] = monitor_out[2*s + 0];
				i2s1_out[2*s + 1] = monitor_out[2*s + 1];
			}
		}	
		#endif

		#ifdef CFG_RES_AUDIO_I2SOUT_EN
		if (i2s_out)
		{
			for(s = 0; s < n; s++)
			{
				i2s_out[2*s + 0] = monitor_out[2*s + 0];
				i2s_out[2*s + 1] = monitor_out[2*s + 1];
			}
		}
		#endif
	}

#ifdef CFG_RES_AUDIO_DACX_EN
	//DAC X单声道录音音效处理
	if(record_out)
	{
		for(s = 0; s < n; s++)
		{
			record_out[s] = __nds32__clips(((int32_t)record_out[2*s] + (int32_t)record_out[2*s + 1] +1 ) >>1, AUDIO_WIDTH - 1);
		}

		AudioEffectMutex_Lock();

		for(i=0; i<AUDIO_EFFECT_NODE_NUM; i++)
		{
			pNode = &gEffectNodeList[2].EffectNode[i];//DACX使用第2组音效
			if((pNode->Enable == FALSE) || (pNode->EffectUnit == NULL))
			{
				continue;
			}
			pNode->FuncAudioEffect(pNode->EffectUnit, (int16_t *)record_out, (int16_t *)record_out, n);
		}

		AudioEffectMutex_Unlock();

		#ifdef CFG_FUNC_REMIND_SOUND_EN
		if(remind_in)
		{
			if(RemindSoundIsMix())
			{
				PCM_DATA_TYPE temp;
				for(s = 0; s < n; s++)
				{
					temp 			= __nds32__clips(((int32_t)remind_in[2*s] + (int32_t)remind_in[2*s + 1] + 1) >> 1, AUDIO_WIDTH - 1);
					record_out[s]	= __nds32__clips(((int32_t)temp + (int32_t)record_out[s]), AUDIO_WIDTH - 1);
				}
			}
			else
			{
				for(s = 0; s < n; s++)
				{
					record_out[s] 	= __nds32__clips(((int32_t)remind_in[2*s] + (int32_t)remind_in[2*s + 1] + 1)>>1, AUDIO_WIDTH - 1);
				}
			}
		}
		#endif
	}
#endif
}

#if defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT == ENABLE)
__attribute__((optimize("Og")))
void AudioEffectProcessBTHF(AudioCoreContext *pAudioCore)
{
	int16_t  s;
	uint16_t i;
	uint16_t n = AudioCoreFrameSizeGet(AudioCore.CurrentMix);
	EffectNode*  pNode 		= NULL;
	int16_t *remind_in      = NULL;//pBuf->remind_in;
	int16_t *monitor_out    = NULL;//pBuf->dac0_out;
	int16_t *record_out     = NULL;//pBuf->dacx_out;
	int16_t *i2s_out        = NULL;//pBuf->i2s0_out;
	int16_t *usb_out        = NULL;//pBuf->usb_out;

	int16_t  *hf_mic_in     = NULL;//pBuf->hf_mic_in;//蓝牙通话mic采样buffer
	int16_t  *hf_pcm_in     = NULL;//pBuf->hf_pcm_in;//蓝牙通话下传buffer
	int16_t  *hf_aec_in		= NULL;//pBuf->hf_aec_in;;//蓝牙通话下传delay buffer,专供aec算法处理
	int16_t  *hf_pcm_out    = NULL;//pBuf->hf_pcm_out;//蓝牙通话上传buffer
	int16_t  *hf_dac_out    = NULL;//pBuf->hf_dac_out;//蓝牙通话DAC的buffer
	int16_t  *hf_rec_out    = NULL;//pBuf->hf_rec_out;//蓝牙通话送录音的buffer
	int16_t  *u_pcm_tmp     = (int16_t *)pcm_buf_4;
	int16_t  *d_pcm_tmp     = (int16_t *)pcm_buf_5;


	if(pAudioCore->AudioSource[MIC_SOURCE_NUM].Active == TRUE)////mic buff
	{
		hf_mic_in = (int16_t *)pAudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf;
	}

	if(pAudioCore->AudioSource[APP_SOURCE_NUM].Active == TRUE)////music buff
	{
		//hf sco: nomo
		hf_pcm_in = (int16_t *)pAudioCore->AudioSource[APP_SOURCE_NUM].PcmInBuf;

		//aec process:push the new data, pop the old data
		if(BtHf_AECRingDataSpaceLenGet() > CFG_BTHF_PARA_SAMPLES_PER_FRAME)
			BtHf_AECRingDataSet(hf_pcm_in, CFG_BTHF_PARA_SAMPLES_PER_FRAME);
		hf_aec_in = BtHf_AecInBuf();
	}

#if defined(CFG_FUNC_REMIND_SOUND_EN)
	if(pAudioCore->AudioSource[REMIND_SOURCE_NUM].Active == TRUE)////remind buff
	{
		remind_in = (int16_t *)pAudioCore->AudioSource[REMIND_SOURCE_NUM].PcmInBuf;
	}
#endif

#ifdef CFG_RES_AUDIO_DAC0_EN
    if(pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].Active == TRUE)   ////dac0 buff
	{
		//hf mode
		hf_dac_out = (int16_t *)pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf;
		hf_pcm_out = (int16_t *)pAudioCore->AudioSink[AUDIO_HF_SCO_SINK_NUM].PcmOutBuf;
	}
#else
	hf_pcm_out = pAudioCore->AudioSink[AUDIO_HF_SCO_SINK_NUM].PcmOutBuf;
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
	if(pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].Active == TRUE)   ////dacx buff
	{
		#if (BT_HFP_SUPPORT == ENABLE)
		if(GetSystemMode() == ModeBtHfPlay)
		{
			record_out = NULL;
		}
		else
		#endif
		{
			record_out = (int16_t *)pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].PcmOutBuf;
		}
	}
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN
	if(pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].Active == TRUE)	////i2s buff
	{
		i2s_out = (int16_t *)pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].PcmOutBuf;
	}
#endif

    if(monitor_out)
	{
		memset(monitor_out, 0, n * 2 * 2);
    }

    if(record_out)
    {
		memset(record_out, 0, n * 2);
    }

    if(usb_out)
    {
		memset(usb_out, 0, n * 2 * 2);//mono*2 stereo*4
    }

    if(i2s_out)
    {
		memset(i2s_out, 0, n * 2 * 2);//mono*2 stereo*4
	}

	if(hf_pcm_out)
	{
		memset(hf_pcm_out, 0, n * 2);
	}

	if(hf_dac_out)
	{
		memset(hf_dac_out, 0, n * 2 * 2);
	}

	if(hf_rec_out)
	{
		memset(hf_rec_out, 0, n * 2);
	}

	if(hf_mic_in && hf_pcm_in && hf_pcm_out && (hf_dac_out || i2s_out))
	{
		AudioCoreAppSourceVolApply(APP_SOURCE_NUM, hf_pcm_in, n, 1);

		if(hf_dac_out == NULL)
			hf_dac_out = i2s_out;
#if defined(CFG_FUNC_REMIND_SOUND_EN)
		if(remind_in)
		{
			AudioCoreAppSourceVolApply(REMIND_SOURCE_NUM, remind_in, n, 2);
			for(s = 0; s < n; s++)
			{
				hf_dac_out[2*s + 0] = __nds32__clips((((int32_t)hf_pcm_in[s] + (int32_t)remind_in[2*s + 0])), 16-1);
				hf_dac_out[2*s + 1] = __nds32__clips((((int32_t)hf_pcm_in[s] + (int32_t)remind_in[2*s + 1])), 16-1);
			}
		}
		else
#endif
		{
			for(s = 0; s < n; s++)
			{
				hf_dac_out[s*2 + 0] = hf_pcm_in[s];
				hf_dac_out[s*2 + 1] = hf_pcm_in[s];
			}
		}

		for(s = 0; s < n; s++)
		{
			hf_pcm_out[s] = __nds32__clips((((int32_t)hf_mic_in[2 * s + 0] + (int32_t)hf_mic_in[2 * s + 1])), 16-1);
		}

		for(i=0; i<AUDIO_EFFECT_NODE_NUM; i++)
		{
			pNode = &gEffectNodeList[0].EffectNode[i];//伴奏使用第0组音效列表
			if((pNode->Enable == FALSE) || (pNode->EffectUnit == NULL))
			{
				continue;
			}
#if CFG_AUDIO_EFFECT_AEC_EN
			if(pNode->NodeType == NodeType_AEC)
			{
				for(s = 0; s < n; s++)
				{
					d_pcm_tmp[s] = hf_pcm_out[s];
					u_pcm_tmp[s] = hf_aec_in[s];
				}
				AudioEffectAECApply(pNode->EffectUnit, u_pcm_tmp , d_pcm_tmp, hf_pcm_out, n);
			}
			else
#endif
			{
				pNode->FuncAudioEffect(pNode->EffectUnit, hf_pcm_out, hf_pcm_out, n);
				if(pNode->NodeType == NodeType_Vol)
				{
					AudioCoreAppSourceVolApply(MIC_SOURCE_NUM, hf_pcm_out, n, 1);
				}
			}
		}
	}
	else if(hf_dac_out || i2s_out)
	{
		if(hf_dac_out == NULL)
			hf_dac_out = i2s_out;	
	#if defined(CFG_FUNC_REMIND_SOUND_EN)//提示音音效处理
		if(remind_in)
		{
			AudioCoreAppSourceVolApply(REMIND_SOURCE_NUM, remind_in, n, 2);
			for(s = 0; s < n; s++)
			{
				hf_dac_out[2*s + 0] = remind_in[2*s + 0];
				hf_dac_out[2*s + 1] = remind_in[2*s + 1];
			}
		}
	#endif
	}
	
	if(i2s_out && hf_dac_out != i2s_out)
	{
		for(s = 0; s < n; s++)
		{
			i2s_out[2*s + 0] = hf_dac_out[2*s + 0];
			i2s_out[2*s + 1] = hf_dac_out[2*s + 1];
		}
	}

	#ifdef CFG_RES_AUDIO_DACX_EN
	//DAC X单声道录音音效处理
	if(record_out)
	{
		for(s = 0; s < n; s++)
		{
			record_out[s] 	= __nds32__clips(((int32_t)hf_dac_out[2*s] + (int32_t)hf_dac_out[2*s + 1] + 1)>>1, 16-1);
		}
	}
	#endif
}

#endif
#endif
/*
****************************************************************
* 无音效音频处理主函数
*
****************************************************************
*/
void AudioBypassProcess(AudioCoreContext *pAudioCore)
{
	int16_t  s;
	uint16_t n = AudioCoreFrameSizeGet(AudioCore.CurrentMix);
	int16_t *mic_pcm    	= NULL;//pBuf->mic_in;///mic input
	int16_t *monitor_out    = NULL;//pBuf->dac0_out;
	int16_t *remind_in = NULL;
	int16_t *music_pcm    	= NULL;
#ifdef CFG_RES_AUDIO_DACX_EN
	int16_t *record_out     = NULL;//pBuf->dacx_out;
#endif
	int16_t *i2s_out       	= NULL;//pBuf->i2s0_out;

	if(pAudioCore->AudioSource[APP_SOURCE_NUM].Active == TRUE)////music buff
	{
		music_pcm = (int16_t *)pAudioCore->AudioSource[APP_SOURCE_NUM].PcmInBuf;// include line/bt/usb/sd/spdif/hdmi/i2s/radio source
	}
	if(pAudioCore->AudioSource[MIC_SOURCE_NUM].Active == TRUE)////mic buff
	{
		mic_pcm = (int16_t *)pAudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf;//双mic输入
	}

    if(pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].Active == TRUE)   ////dac0 buff
	{
    	monitor_out = (int16_t *)pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf;
	}
#ifdef CFG_RES_AUDIO_DACX_EN
	if(pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].Active == TRUE)   ////dacx buff
	{
		record_out = (int16_t *)pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].PcmOutBuf;
	}
#endif	
#if defined(CFG_FUNC_REMIND_SOUND_EN)
	if(pAudioCore->AudioSource[REMIND_SOURCE_NUM].Active == TRUE)////remind buff
	{
		remind_in = (int16_t *)pAudioCore->AudioSource[REMIND_SOURCE_NUM].PcmInBuf;
	}
#endif
#ifdef CFG_RES_AUDIO_I2SOUT_EN
	if(pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].Active == TRUE) ////i2s buff
	{
		i2s_out = (int16_t *)pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].PcmOutBuf;
	}
#endif

    if(monitor_out)
	{
		memset(monitor_out, 0, n * 2 * 2);
    }

	//DAC0立体声监听音效处理
	if(monitor_out || i2s_out)
	{
		if(monitor_out == NULL)
			monitor_out = i2s_out;

		//系统增益控制，根据从工经验，打散放到音效处理中来处理
		AudioCoreAppSourceVolApply(APP_SOURCE_NUM, music_pcm, n, 2);
		//系统增益控制，根据从工经验，打散放到音效处理中来处理
#ifdef CFG_FUNC_REMIND_SOUND_EN
		AudioCoreAppSourceVolApply(REMIND_SOURCE_NUM, remind_in, n, 2);
#endif
		for(s = 0; s < n; s++)
		{
			#if defined(CFG_FUNC_REMIND_SOUND_EN)
			if(remind_in)
			{
				monitor_out[2*s + 0] = remind_in[2*s + 0];
				monitor_out[2*s + 1] = remind_in[2*s + 1];
			}
			else
			#endif			
			if(mic_pcm)
			{
				monitor_out[2*s + 0] = mic_pcm[2*s + 0];
				monitor_out[2*s + 1] = mic_pcm[2*s + 1];
			}

			if(music_pcm)
			{
				monitor_out[2*s + 0] = __nds32__clips((((int32_t)monitor_out[2 * s + 0] + (int32_t)music_pcm[2 * s + 0])), 16-1);
				monitor_out[2*s + 1] = __nds32__clips((((int32_t)monitor_out[2 * s + 1] + (int32_t)music_pcm[2 * s + 1])), 16-1);
			}
		}

		if(i2s_out && i2s_out != monitor_out)
		{
			for(s = 0; s < n; s++)
			{
				i2s_out[2*s + 0] = monitor_out[2*s + 0];
				i2s_out[2*s + 1] = monitor_out[2*s + 1];
			}
		}		
	}

#ifdef CFG_RES_AUDIO_DACX_EN
	//DAC X
	if(record_out && monitor_out)
	{
		for(s = 0; s < n; s++)
		{
			record_out[s] = __nds32__clips((( (int32_t)monitor_out[2*s+0] + (int32_t)monitor_out[2*s+1])), 16-1);					    
		}		
	}
#endif	
}

//无App通路音频处理主函数
void AudioNoAppProcess(AudioCoreContext *pAudioCore)
{
	int16_t  s;
	uint16_t n = AudioCoreFrameSizeGet(AudioCore.CurrentMix);
	int16_t *mic_pcm    	= NULL;//pBuf->mic_in;///mic input
	int16_t *monitor_out    = NULL;//pBuf->dac0_out;
	int16_t *remind_in 		= NULL;
#ifdef CFG_RES_AUDIO_DACX_EN
	int16_t *record_out     = NULL;//pBuf->dacx_out;
#endif
	int16_t *i2s_out       	= NULL;//pBuf->i2s0_out;

	if(pAudioCore->AudioSource[MIC_SOURCE_NUM].Active == TRUE)////mic buff
	{
		mic_pcm = (int16_t *)pAudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf;//双mic输入
	}

    if(pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].Active == TRUE)   ////dac0 buff
	{
    	monitor_out = (int16_t *)pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf;
	}
#ifdef CFG_RES_AUDIO_DACX_EN
	if(pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].Active == TRUE)   ////dacx buff
	{
		record_out = (int16_t *)pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].PcmOutBuf;
	}
#endif	

#if defined(CFG_FUNC_REMIND_SOUND_EN)
	if(pAudioCore->AudioSource[REMIND_SOURCE_NUM].Active == TRUE)////remind buff
	{
		remind_in = (int16_t *)pAudioCore->AudioSource[REMIND_SOURCE_NUM].PcmInBuf;
	}
#endif
#ifdef CFG_RES_AUDIO_I2SOUT_EN
	if(pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].Active == TRUE)	////i2s buff
	{
		i2s_out = (int16_t *)pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].PcmOutBuf;
	}
#endif


    if(monitor_out)
	{
		memset(monitor_out, 0, n * 2 * 2);
    }

	//DAC0立体声监听音效处理
	if(monitor_out || i2s_out)
	{
		if(monitor_out == NULL)
			monitor_out = i2s_out;
		for(s = 0; s < n; s++)
		{
			#if defined(CFG_FUNC_REMIND_SOUND_EN)
			if(remind_in)
			{
				monitor_out[2*s + 0] = remind_in[2*s + 0];
				monitor_out[2*s + 1] = remind_in[2*s + 1];
			}
			else
			#endif			
			if(mic_pcm)
			{
				monitor_out[2*s + 0] = mic_pcm[2*s + 0];
				monitor_out[2*s + 1] = mic_pcm[2*s + 1];
			}
			else
			{
				monitor_out[2*s + 0] = 0;
				monitor_out[2*s + 1] = 0;
			}
		}

		if(i2s_out && i2s_out != monitor_out)
		{
			for(s = 0; s < n; s++)
			{
				i2s_out[2*s + 0] = monitor_out[2*s + 0];
				i2s_out[2*s + 1] = monitor_out[2*s + 1];
			}
		}		
	}

#ifdef CFG_RES_AUDIO_DACX_EN
	//DAC X
	if(record_out && monitor_out)
	{
		for(s = 0; s < n; s++)
		{
			record_out[s] = __nds32__clips((( (int32_t)monitor_out[2*s+0] + (int32_t)monitor_out[2*s+1])), 16-1);					    
		}	
	}
#endif	//CFG_RES_AUDIO_DACX_EN
}

