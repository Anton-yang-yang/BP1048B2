/**
 **************************************************************************************
 * @file    media_play_mode.c
 * @brief   
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2017-12-28 18:00:00$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include "type.h"
#include "irqn.h"
#include "app_config.h"
#include "app_message.h"
#include "chip_info.h"
#include "dac.h"
#include "gpio.h"
#include "dma.h"
#include "dac.h"
#include "audio_adc.h"
#include "rtos_api.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "debug.h"
#include "audio_effect.h"
#include "main_task.h"
#include "audio_core_api.h"
#include "media_play_mode.h"
#include "decoder.h"
#include "audio_core_service.h"
#include "media_play_api.h"
#include "remind_sound.h"
#include "powercontroller.h"
#include "deepsleep.h"
#include "backup_interface.h"
#include "breakpoint.h"
#include "otg_device_hcd.h"
#include "otg_device_standard_request.h"
#include "ctrlvars.h"
#include "audio_vol.h"
#include "timeout.h"
#include "ffpresearch.h"
#include "mode_task.h"
#include "mode_task_api.h"
#include "device_detect.h"
#include "watchdog.h"
#include "audio_effect_flash_param.h"
#include "bt_app_tws.h"

#if defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN)
typedef struct _mediaPlayContext
{

	DEV_ID				Device;	//当前播放的设备
	//QueueHandle_t 		audioMutex;
	//QueueHandle_t		pcmBufMutex;
	uint8_t				SourceNum;//播放器和回放使用不同通道，前者有音效
	AudioCoreContext 	*AudioCoreMediaPlay;
	//play
	uint32_t 			SampleRate;
	//used Service 
}MediaPlayContext;
static MediaPlayContext*	sMediaPlayCt = NULL;
static uint32_t sPlugOutTimeOutCount = 0; 
extern uint32_t CmdErrCnt;
extern volatile uint32_t gInsertEventDelayActTimer;

/**根据appconfig缺省配置:DMA 8个通道配置**/
/*1、cec需PERIPHERAL_ID_TIMER3*/
/*2、SD卡录音需PERIPHERAL_ID_SDIO RX/TX*/
/*3、在线串口调音需PERIPHERAL_ID_UART1 RX/TX,建议使用USB HID，节省DMA资源*/
/*4、线路输入需PERIPHERAL_ID_AUDIO_ADC0_RX*/
/*5、Mic开启需PERIPHERAL_ID_AUDIO_ADC1_RX，mode之间通道必须一致*/
/*6、Dac0开启需PERIPHERAL_ID_AUDIO_DAC0_TX mode之间通道必须一致*/
/*7、DacX需开启PERIPHERAL_ID_AUDIO_DAC1_TX mode之间通道必须一致*/
/*注意DMA 8个通道配置冲突:*/
/*a、UART在线调音和DAC-X有冲突,默认在线调音使用USB HID*/
static const uint8_t DmaChannelMap[29] =
{
	255,//PERIPHERAL_ID_SPIS_RX = 0,	//0
	255,//PERIPHERAL_ID_SPIS_TX,		//1

#ifdef CFG_APP_HDMIIN_MODE_EN
	5,//PERIPHERAL_ID_TIMER3,			//2
#else
	255,//PERIPHERAL_ID_TIMER3,			//2
#endif

	4,//PERIPHERAL_ID_SDIO_RX,			//3
	4,//PERIPHERAL_ID_SDIO_TX,			//4

	255,//PERIPHERAL_ID_UART0_RX,		//5
	255,//PERIPHERAL_ID_TIMER1,			//6
	255,//PERIPHERAL_ID_TIMER2,			//7
	255,//PERIPHERAL_ID_SDPIF_RX,		//8 SPDIF_RX /TX same chanell
	255,//PERIPHERAL_ID_SDPIF_TX,		//8 SPDIF_RX /TX same chanell
	255,//PERIPHERAL_ID_SPIM_RX,		//9
	255,//PERIPHERAL_ID_SPIM_TX,		//10
	255,//PERIPHERAL_ID_UART0_TX,		//11
	255,//PERIPHERAL_ID_UART1_RX,		//12
	255,//PERIPHERAL_ID_UART1_TX,		//13
	255,//PERIPHERAL_ID_TIMER4,			//14
	255,//PERIPHERAL_ID_TIMER5,			//15
	255,//PERIPHERAL_ID_TIMER6,			//16
	255,//PERIPHERAL_ID_AUDIO_ADC0_RX,	//17
	1,//PERIPHERAL_ID_AUDIO_ADC1_RX,	//18
	2,//PERIPHERAL_ID_AUDIO_DAC0_TX,	//19
	3,//PERIPHERAL_ID_AUDIO_DAC1_TX,	//20

#if defined (CFG_FUNC_I2S_MIX_MODE) && defined (CFG_RES_AUDIO_I2S0IN_EN)
	6,//PERIPHERAL_ID_I2S0_RX,			//21
#else
	255,//PERIPHERAL_ID_I2S0_RX,		//21
#endif

#if	((defined(CFG_RES_AUDIO_I2SOUT_EN )&&(CFG_RES_I2S == 0)) || defined(CFG_RES_AUDIO_I2S0OUT_EN))
	7,//PERIPHERAL_ID_I2S0_TX,			//22
#else	
	255,//PERIPHERAL_ID_I2S0_TX,		//22
#endif

#if defined (CFG_FUNC_I2S_MIX_MODE) && defined (CFG_RES_AUDIO_I2S1IN_EN)
	255,//PERIPHERAL_ID_I2S1_RX,			//23
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


uint8_t MediaPlayDevice(void)
{
	if(sMediaPlayCt == NULL)
	{
		return 255;
	}
	return sMediaPlayCt->Device;
}

static  void MediaPlayerSetAudioSource()
{
	AudioCoreSourceEnable(sMediaPlayCt->SourceNum);
	//AudioCoreSourceUnmute(sMediaPlayCt->SourceNum, TRUE, TRUE);
	#ifdef BT_TWS_SUPPORT
	//AudioCoreSourceUnmute(TWS_SOURCE_NUM, TRUE, TRUE);
	#endif

#ifdef CFG_FUNC_DISPLAY_EN
	MessageContext		msgSend;
	msgSend.msgId = MSG_DISPLAY_SERVICE_FILE_NUM;
	MessageSend(GetSysModeMsgHandle(), &msgSend);
#endif
	
}

static void MediaPlayMsgProcess(uint16_t msgId)
{
#ifdef CFG_FUNC_DISPLAY_EN
	MessageContext	msgSend;
#endif
	switch(msgId)
	{
		case MSG_REPEAT:
			APP_DBG("Media:MSG_REPEAT\n");
			MediaPlayerSwitchPlayMode(PLAY_MODE_SUM);
#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_REPEAT;
            MessageSend(GetSysModeMsgHandle(), &msgSend);
#endif
			break;

		case MSG_FOLDER_PRE:
			if(CmdErrCnt >= 3)//临时处理U盘异常但是U盘插着的情况
			{
				if(!IsMediaPlugOut())
				{
					SendModeKeyMsg();
				}
				SoftFlagRegister(SoftFlagMediaDevicePlutOut);
				CmdErrCnt = 0;
				return;
			}
			MediaPlayerPreFolder();
			break;

		case MSG_FOLDER_NEXT:
			if(CmdErrCnt >= 3)//临时处理U盘异常但是U盘插着的情况
			{
				if(!IsMediaPlugOut())
				{
					SendModeKeyMsg();
				}
				SoftFlagRegister(SoftFlagMediaDevicePlutOut);
				CmdErrCnt = 0;
				return;
			}
			MediaPlayerNextFolder();
			break;
		case MSG_PLAY_PAUSE:
			APP_DBG("Media:MSG_PLAY_PAUSE\n");
			MediaPlayerPlayPause();
#ifdef CFG_FUNC_DISPLAY_EN
			msgSend.msgId = MSG_DISPLAY_SERVICE_MEDIA;
			MessageSend(GetSysModeMsgHandle(), &msgSend);
#endif
			break;

		case MSG_STOP:
			APP_DBG("Media:MSG_STOP\n");
			MediaPlayerStop();
			break;

		case MSG_DECODER_SERVICE_MUTLI_EMPTY:
			APP_DBG("Media:Disk ERROR\n");
			MediaPlayerStop();
			SendModeKeyMsg();
			break;
		case MSG_DECODER_SERVICE_SONG_PLAY_END:
		//case MSG_DECODER_SERVICE_FF_END_SONG:
			if(GetDecoderState(DECODER_MODE_CHANNEL) == DecoderStatePause)//确保播放结束 ，依旧暂停态，减少stop消息交叉
			{
				APP_DBG("Media:Last end, play next\n");
#ifdef	CFG_APP_USB_PLAY_MODE_EN
				if(CmdErrCnt >= 3)//临时处理U盘异常但是U盘插着的情况
				{
					APP_DBG("cmd error >= 3\n");
					if(!IsMediaPlugOut())
					{
						SendModeKeyMsg();
					}
					SoftFlagRegister(SoftFlagMediaDevicePlutOut);
					CmdErrCnt = 0;
					return;
				}
#endif
				if(gMediaPlayer->CurPlayMode==PLAY_MODE_REPEAT_OFF)
				{
					//gMediaPlayer->CurFileIndex;
					if(gMediaPlayer->CurFileIndex+1 > gMediaPlayer->TotalFileSumInDisk)
					{
						gMediaPlayer->CurFileIndex = 1;
						MediaPlayerStop();
					}
					else
					{
						DecoderMuteAndStop(DECODER_MODE_CHANNEL);
						MediaPlayerNextSong(TRUE);
					}
				}
				else
				{
					DecoderMuteAndStop(DECODER_MODE_CHANNEL);
					MediaPlayerNextSong(TRUE);
				}
			}
			break;
		case MSG_NEXT:
#ifdef CFG_FUNC_REMIND_SOUND_EN
			if(CmdErrCnt < 3)
			{
				DecoderMuteAndStop(DECODER_MODE_CHANNEL);
				if(RemindSoundServiceItemRequest(SOUND_REMIND_XIAYISOU, REMIND_PRIO_NORMAL|REMIND_ATTR_NEED_MUTE_APP_SOURCE))//RemindSound request
				{
					gMediaPlayer->RemindSoundFlag = 1;
					break;
				}
			}
#endif
		case MSG_SOFT_NEXT: 	
			APP_DBG("Media:play next\n");
			if(CmdErrCnt >= 3)//临时处理U盘异常但是U盘插着的情况
			{
				if(!IsMediaPlugOut())
				{
					SendModeKeyMsg();
				}
				SoftFlagRegister(SoftFlagMediaDevicePlutOut);
				CmdErrCnt = 0;
				return;
			}
			DecoderMuteAndStop(DECODER_MODE_CHANNEL);
			MediaPlayerNextSong(FALSE);
			break;
		
		case MSG_PRE:
#ifdef CFG_FUNC_REMIND_SOUND_EN
			if(CmdErrCnt < 3)
			{
				DecoderMuteAndStop(DECODER_MODE_CHANNEL);
				if(RemindSoundServiceItemRequest(SOUND_REMIND_SHANGYIS, REMIND_PRIO_NORMAL|REMIND_ATTR_NEED_MUTE_APP_SOURCE))//RemindSound request
				{
					gMediaPlayer->RemindSoundFlag = 2;
					break;
				}
			}
#endif
		case MSG_SOFT_PRE:
			APP_DBG("Media:MSG_PRE\n");
			if(CmdErrCnt >= 3)//临时处理U盘异常但是U盘插着的情况
			{
				if(!IsMediaPlugOut())
				{
					SendModeKeyMsg();
				}
				SoftFlagRegister(SoftFlagMediaDevicePlutOut);
				CmdErrCnt = 0;
				return;
			}
			
			DecoderMuteAndStop(DECODER_MODE_CHANNEL);
			MediaPlayerPreSong();
			break;
#ifdef CFG_FUNC_REMIND_SOUND_EN
		case MSG_REMIND_PLAY_END:
			if(gMediaPlayer->RemindSoundFlag == 1)
			{
				MessageContext	msgSend;
				msgSend.msgId = MSG_SOFT_NEXT;
				MessageSend(GetSysModeMsgHandle(), &msgSend);
			}
			if(gMediaPlayer->RemindSoundFlag == 2)
			{
				MessageContext	msgSend;
				msgSend.msgId = MSG_SOFT_PRE;
				MessageSend(GetSysModeMsgHandle(), &msgSend);
			}
			gMediaPlayer->RemindSoundFlag = 0;
			break;
#endif
		case MSG_FF_START:
			MediaPlayerFastForward();
			APP_DBG("Media:MSG_FF_START\n");
			break;
		
		case MSG_FB_START:
			MediaPlayerFastBackward();
			APP_DBG("Media:MSG_FB_START\n");
			break;

		case MSG_FF_FB_END:
			MediaPlayerFFFBEnd();
			APP_DBG("Media:MSG_FF_FB_END\n");
			break;

#ifdef CFG_RES_IR_NUMBERKEY
		case MSG_NUM_0:
		case MSG_NUM_1:
		case MSG_NUM_2:
		case MSG_NUM_3:
		case MSG_NUM_4:
		case MSG_NUM_5:
		case MSG_NUM_6:
		case MSG_NUM_7:
		case MSG_NUM_8:
		case MSG_NUM_9:
			if(!Number_select_flag)
				Number_value = msgId - MSG_NUM_0;
			else
				Number_value = Number_value * 10 + (msgId - MSG_NUM_0);
			if(Number_value > 9999)
				Number_value = 0;
			Number_select_flag = 1;
			TimeOutSet(&Number_selectTimer, 2000);
            #ifdef CFG_FUNC_DISPLAY_EN
             msgSend.msgId = MSG_DISPLAY_SERVICE_NUMBER;
             MessageSend(GetSysModeMsgHandle(), &msgSend);
            #endif
			break;
#endif

		case MSG_DECODER_SERVICE_UPDATA_PLAY_TIME:
			MediaPlayerTimeUpdate();
            #ifdef CFG_FUNC_DISPLAY_EN
			msgSend.msgId = MSG_DISPLAY_SERVICE_MEDIA;
			MessageSend(GetSysModeMsgHandle(), &msgSend);
            #endif
			break;

		case MSG_DECODER_MODE_PLAYING:
			AudioCoreSourceUnmute(sMediaPlayCt->SourceNum, TRUE, TRUE);
			break;

		case MSG_REPEAT_AB:
			APP_DBG("MSG_REPEAT_AB\n");
			MediaPlayerRepeatAB();
			break;
			
		default:
			if(gMediaPlayer->RepeatAB.RepeatFlag == REPEAT_OPENED
				&& (GetSystemMode() == ModeCardAudioPlay || GetSystemMode() == ModeUDiskAudioPlay))
			{
				MediaPlayerTimerCB();
			}
			CommonMsgProccess(msgId);
			break;
	}
#ifdef CFG_RES_IR_NUMBERKEY
	if(Number_select_flag)
	{
		if(IsTimeOut(&Number_selectTimer))
		{
			if((gMediaPlayer->CurPlayMode ==PLAY_MODE_REPEAT_FOLDER) || (gMediaPlayer->CurPlayMode == PLAY_MODE_RANDOM_FOLDER))
			{
				if((Number_value <= (gMediaPlayer->PlayerFolder.FirstFileIndex + gMediaPlayer->PlayerFolder.FileNumLen - 1)) && (Number_value))
				{
					DecoderMuteAndStop(DECODER_MODE_CHANNEL);
					gMediaPlayer->CurFileIndex = Number_value;
					gMediaPlayer->CurPlayTime = 0;
					gMediaPlayer->SongSwitchFlag = 0;
					SetMediaPlayerState(PLAYER_STATE_PLAYING);
					MediaPlayerSong();
				}
			}
#ifdef FUNC_BROWSER_PARALLEL_EN
			else if(GetBrowserPlay_state()==Browser_Play_None)
			{
				APP_DBG("browser normal playing,not support ir number key select song \n");
			}
#endif
			else
			{
				if((Number_value <= gMediaPlayer->TotalFileSumInDisk) && (Number_value))
				{
					DecoderMuteAndStop(DECODER_MODE_CHANNEL);
					gMediaPlayer->CurFileIndex = Number_value;
					gMediaPlayer->CurPlayTime = 0;
					gMediaPlayer->SongSwitchFlag = 0;
					SetMediaPlayerState(PLAYER_STATE_PLAYING);
					MediaPlayerSong();
				}
			}
			Number_select_flag = 0;
		}
	}
#endif
}

/**
 * @func        MediaPlayInit
 * @brief       Media模式参数配置，资源初始化
 * @param       MessageHandle   
 * @Output      None
 * @return      bool
 * @Others      任务块、Dac、AudioCore配置，数据源自DecoderService
 * @Others      数据流从Decoder到audiocore配有函数指针，audioCore到Dac同理，由audiocoreService任务按需驱动
 * Record
 */
bool MediaPlayInit(void)
{

	bool ret = FALSE;
	
	if(sMediaPlayCt != NULL)
	{
		return FALSE;
	}

#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	//音效参数遍历，确定系统帧长，待修改，sam, mark
#endif
#ifdef BT_TWS_SUPPORT
	tws_delay = BT_TWS_DELAY_DEFAULT;
#endif


	//DMA channel
	DMA_ChannelAllocTableSet((uint8_t*)DmaChannelMap);

	if(!ModeCommonInit())
	{
		DBG("Common Audio error!\n");
		return FALSE;
	}

	if(gInsertEventDelayActTimer)
	{
#ifdef CFG_FUNC_UDISK_DETECT
		if(GetUdiscState()==DETECT_STATE_OUT && ((GetSystemMode()==ModeUDiskAudioPlay)
#ifdef CFG_FUNC_RECORDER_EN
					||(GetSystemMode()==ModeUDiskPlayBack)
#endif
					))
		{
			APP_DBG(" not have usb,return\n");
			return FALSE;
		}
#endif
#ifdef CFG_FUNC_CARD_DETECT
		if(GetCardState()==DETECT_STATE_OUT && ((GetSystemMode()==ModeCardAudioPlay)
#ifdef CFG_FUNC_RECORDER_EN
				||(GetSystemMode()==ModeCardPlayBack)
#endif
				))
		{
			APP_DBG(" not have sd,return\n");
			return FALSE;
		}
#endif
	}

	APP_DBG("Media Play Init\n");

	sMediaPlayCt = (MediaPlayContext*)osPortMalloc(sizeof(MediaPlayContext));
	if(sMediaPlayCt == NULL)
	{
		return FALSE;
	}
	memset(sMediaPlayCt, 0, sizeof(MediaPlayContext));
	sMediaPlayCt->SampleRate = CFG_PARA_SAMPLE_RATE;
	sMediaPlayCt->AudioCoreMediaPlay = (AudioCoreContext*)&AudioCore;

	//Audio init Soure.
//	sMediaPlayCt->SourceNum = MEDIA_PLAY_DECODER_SOURCE_NUM;

#ifdef CFG_FUNC_RECORDER_EN
	if(GetSystemMode() == ModeCardPlayBack || GetSystemMode() == ModeUDiskPlayBack)// || GetSystemMode() == AppModeFlashFsPlayBack)
	{
		sMediaPlayCt->SourceNum = PLAYBACK_SOURCE_NUM;
	}
	else
#endif
	{
		sMediaPlayCt->SourceNum = MEDIA_PLAY_DECODER_SOURCE_NUM;
	}
	DecoderSourceNumSet(sMediaPlayCt->SourceNum, DECODER_MODE_CHANNEL);
	AudioCoreIO	AudioIOSet;
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	AudioIOSet.Adapt = SRC_ONLY;
	AudioIOSet.Sync = FALSE;
	AudioIOSet.Channels = 2;
	AudioIOSet.Net = DefaultNet;
	AudioIOSet.DataIOFunc = ModeDecoderPcmDataGet;
	AudioIOSet.LenGetFunc = ModeDecoderPcmDataLenGet;
	AudioIOSet.SampleRate = CFG_PARA_SAMPLE_RATE;//初始值
#ifdef	CFG_AUDIO_WIDTH_24BIT
	AudioIOSet.IOBitWidth = 0;//0,16bit,1:24bit
#ifdef BT_TWS_SUPPORT
	AudioIOSet.IOBitWidthConvFlag = 0;//tws不需要数据进行位宽扩展，会在TWS_SOURCE_NUM以后统一转成24bit
#else
	AudioIOSet.IOBitWidthConvFlag = 1;//需要数据进行位宽扩展
#endif
#endif
	if(!AudioCoreSourceInit(&AudioIOSet, sMediaPlayCt->SourceNum))
	{
		DBG("mediaplay source error!\n");
		return FALSE;
	}
	
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	sMediaPlayCt->AudioCoreMediaPlay->AudioEffectProcess = (AudioCoreProcessFunc)AudioMusicProcess;
#else
	sMediaPlayCt->AudioCoreMediaPlay->AudioEffectProcess = (AudioCoreProcessFunc)AudioBypassProcess;
#endif
	
	sMediaPlayCt->Device = (GetSystemMode() == ModeUDiskAudioPlay
#ifdef CFG_FUNC_RECORDER_EN
			|| GetSystemMode() == ModeUDiskPlayBack
#endif
			) ? DEV_ID_USB : DEV_ID_SD;
	APP_DBG("Media:Dev %d\n", sMediaPlayCt->Device);
	if(gMediaPlayer != NULL)
	{
		APP_DBG("player is reopened\n");
	}
	else
	{
		gMediaPlayer = osPortMalloc(sizeof(MEDIA_PLAYER));
		if(gMediaPlayer == NULL)
		{
			APP_DBG("gMediaPlayer malloc error\n");
			return FALSE;
		}
	}
	memset(gMediaPlayer, 0, sizeof(MEDIA_PLAYER));
	
	gMediaPlayer->AccRamBlk = (uint8_t*)osPortMalloc(MAX_ACC_RAM_SIZE);
	if(gMediaPlayer->AccRamBlk == NULL)
	{
		APP_DBG("AccRamBlk malloc error\n");
		return FALSE;
	}
#if defined(CFG_FUNC_BREAKPOINT_EN) && (defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN))
	if(GetSystemMode() == ModeUDiskAudioPlay || GetSystemMode() == ModeCardAudioPlay)
	{
		gMediaPlayer->CurPlayMode = BPDiskSongPlayModeGet();
	}
#endif

#ifdef CFG_RES_IR_NUMBERKEY
	Number_select_flag = 0;
	Number_value = 0;
#endif
	SoftFlagDeregister(SoftFlagMediaDevicePlutOut);
	SoftFlagRegister(SoftFlagMediaModeRead);

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
	
#ifdef FUNC_BROWSER_PARALLEL_EN
	BrowserVarDefaultSet();
#endif

#ifdef CFG_FUNC_DISPLAY_EN
	DispDevSymbol();
#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(GetSystemMode() == ModeUDiskAudioPlay)
		ret = RemindSoundServiceItemRequest(SOUND_REMIND_UPANMODE, REMIND_ATTR_NEED_MUTE_APP_SOURCE);
	if(GetSystemMode() == ModeCardAudioPlay)
		ret = RemindSoundServiceItemRequest(SOUND_REMIND_CARDMODE, REMIND_ATTR_NEED_MUTE_APP_SOURCE);
#ifdef CFG_FUNC_RECORDER_EN
	if(GetSystemMode() == ModeUDiskPlayBack || GetSystemMode() == ModeCardPlayBack)
		ret = RemindSoundServiceItemRequest(SOUND_REMIND_RECHUIFA, REMIND_ATTR_NEED_MUTE_APP_SOURCE);
#endif
	gMediaPlayer->WaitFlag = ret;
	if(ret == FALSE)
	{
		if(IsAudioPlayerMute() == TRUE)
		{
			HardWareMuteOrUnMute();
		}
	}
#endif
	
#ifndef CFG_FUNC_REMIND_SOUND_EN
	if(IsAudioPlayerMute() == TRUE)
	{
		HardWareMuteOrUnMute();
	}
#endif
	
	return TRUE;
}

void MediaPlayRun(uint16_t msgId)
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
#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(gMediaPlayer->WaitFlag)
	{//等提示音播完
		if(!RemindSoundIsPlay())
		{
			gMediaPlayer->WaitFlag = FALSE;
		}
		return;
	}
#endif
	if(SoftFlagGet(SoftFlagMediaModeRead))
	{
#ifdef CFG_FUNC_RECORDER_EN
		if(GetSystemMode() == ModeCardPlayBack || GetSystemMode() == ModeUDiskPlayBack )//|| GetSystemMode() ==AppModeFlashFsPlayBack)
		{
			DecoderServiceInit(GetSysModeMsgHandle(),DECODER_MODE_CHANNEL, DECODER_BUF_SIZE_MP3, DECODER_FIFO_SIZE_FOR_MP3);//DECODER_BUF_SIZE_MP3
		}
		else
#endif
		{
			DecoderServiceInit(GetSysModeMsgHandle(),DECODER_MODE_CHANNEL, DECODER_BUF_SIZE, DECODER_FIFO_SIZE_FOR_PLAYER);// decode step1
		}
		if(!MediaPlayerInitialize(sMediaPlayCt->Device, 1, 1))// decode step 2: include : DecoderInit(&gMediaPlayer->PlayerFile,DECODER_MODE_CHANNEL,(int32_t)IO_TYPE_FILE, (int32_t)SongFileType)
		{
			APP_DBG("Media decoder init error ,exit media init\n");
			if(!IsMediaPlugOut() && (GetSystemMode() == ModeUDiskAudioPlay || GetSystemMode() == ModeCardAudioPlay))
			{
				SendModeKeyMsg();
			}
			SoftFlagRegister(SoftFlagMediaDevicePlutOut);
			SoftFlagDeregister(SoftFlagMediaModeRead);
			return ;
		}
		SoftFlagDeregister(SoftFlagMediaModeRead);
		MediaPlayerSetAudioSource();
		DecoderPlay(DECODER_MODE_CHANNEL);//  decode step 3
		APP_DBG("Media Play run\n");
	}
	if(!SoftFlagGet(SoftFlagUpgradeOK)&&(SoftFlagGet(SoftFlagMvaInUDisk)||SoftFlagGet(SoftFlagMvaInCard)))
	{
		return;
	}
	if(SoftFlagGet(SoftFlagMediaDevicePlutOut))
	{
		sPlugOutTimeOutCount++;
		if(sPlugOutTimeOutCount > 600) 
		{
			SendModeKeyMsg();
			sPlugOutTimeOutCount = 0;
		}	
		return;
	}

	MediaPlayMsgProcess(msgId);	
#if (defined(FUNC_BROWSER_PARALLEL_EN) || defined(FUNC_BROWSER_TREE_EN))
	BrowserMsgProcess(msgId);
#endif
	ModeDecodeProcess();//  decode step 4
}

bool MediaPlayDeinit(void)
{
	if(sMediaPlayCt == NULL)
	{
		return TRUE;
	}
	APP_DBG("Media Play Deinit\n");
	
	if(IsAudioPlayerMute() == FALSE)
	{
		HardWareMuteOrUnMute();
	}
	
	CmdErrCnt = 0;
	SoftFlagDeregister(SoftFlagMediaModeRead);
	SoftFlagDeregister(SoftFlagMediaNextOrPrev);
	
	DecoderServiceStop(DECODER_MODE_CHANNEL);
	PauseAuidoCore();
	MediaPlayerCloseSongFile();

//注意此处，如果在TaskStateCreated,进入stop，它尚未init。
	AudioCoreProcessConfig((void*)AudioNoAppProcess);
	AudioCoreSourceDisable(sMediaPlayCt->SourceNum);
	//AudioCoreSourceUnmute(sMediaPlayCt->SourceNum, TRUE, TRUE);
	
#ifndef CFG_FUNC_MIXER_SRC_EN
#ifdef CFG_RES_AUDIO_DACX_EN
	AudioDAC_SampleRateChange(ALL, CFG_PARA_SAMPLE_RATE);//恢复
#endif
#ifdef CFG_RES_AUDIO_DAC0_EN
	AudioDAC_SampleRateChange(DAC0, CFG_PARA_SAMPLE_RATE);//恢复
#endif
#endif
//	AudioCoreSourceDeinit(REMIND_SOURCE_NUM);
	AudioCoreSourceDeinit(sMediaPlayCt->SourceNum);
	//Kill used services
	DecoderServiceDeinit(DECODER_MODE_CHANNEL);
	if(GetSysModeState(ModeUDiskAudioPlay) == ModeStateDeinit
	#ifdef CFG_FUNC_RECORDER_EN
				|| GetSysModeState(ModeUDiskPlayBack) == ModeStateDeinit
	#endif
		 )
	{
#ifdef	CFG_APP_USB_PLAY_MODE_EN
		f_unmount(MEDIA_VOLUME_STR_U);
		osMutexUnlock(UDiskMutex);
#ifdef CFG_FUNC_UDISK_DETECT
		if(!IsUDiskLink())
		{
			SoftFlagDeregister(SoftFlagUpgradeOK);
		}
#endif
		APP_DBG("unmount u disk\n");
#endif
	}
	else if(GetSysModeState(ModeCardAudioPlay) == ModeStateDeinit
#ifdef CFG_FUNC_RECORDER_EN
			|| GetSysModeState(ModeCardPlayBack) == ModeStateDeinit
#endif
		)
	{
#if (defined(CFG_APP_CARD_PLAY_MODE_EN) )
		f_unmount(MEDIA_VOLUME_STR_C);
		SDCardDeinit(CFG_RES_CARD_GPIO);
		osMutexUnlock(SDIOMutex);
#ifdef CFG_FUNC_CARD_DETECT
		if(GetCardState() == DETECT_STATE_OUT)
#endif
		{
			SoftFlagDeregister(SoftFlagUpgradeOK);
		}
		APP_DBG("unmount sd card\n");
#endif
	}
	
	//PortFree
	ffpresearch_deinit();
	if(gMediaPlayer->AccRamBlk!= NULL)
	{
		osPortFree(gMediaPlayer->AccRamBlk);
		gMediaPlayer->AccRamBlk = NULL;
	}
	MediaPlayerDeinitialize();

	//osPortFree(sMediaPlayCt.AudioCoreMediaPlay);
	sMediaPlayCt->AudioCoreMediaPlay = NULL;

#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	AudioEffectsDeInit();
#endif
	ModeCommonDeinit();

	osPortFree(sMediaPlayCt);
	sMediaPlayCt = NULL;
#ifdef BT_TWS_SUPPORT
	tws_delay = BT_TWS_DELAY_DEINIT;
#endif
	return TRUE;

}

#endif
