/**
 **************************************************************************************
 * @file    media_play_api.c
 * @brief   
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2017-3-17 13:06:47$
 * 
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include "string.h"
#include "type.h"
#include "app_config.h"
#include "app_message.h"
#include "gpio.h"
#include "irqn.h"
#include "gpio.h"
#include "clk.h"
#include "dac.h"
#include "otg_host_standard_enum.h"
#include "otg_detect.h"
#include "delay.h"
#include "rtos_api.h"
#include "freertos.h"
#include "ff.h"
#include "ffpresearch.h"
#include "sd_card.h"
#include "debug.h"
#include "dac_interface.h"
#include "media_play_api.h"
#include "audio_core_api.h"
#include "decoder.h"
#include "device_detect.h"
#include "remind_sound.h"
#include "breakpoint.h"
#include "main_task.h"
#include "string_convert.h"
#include "random.h"
#include "timeout.h"
#include "audio_vol.h"
#include "browser_parallel.h"
#include "browser_tree.h"
#include "bt_app_tws.h"
#ifdef	CFG_FUNC_RECORDER_EN
#include "recorder_service.h"
#endif
#ifdef CFG_FUNC_LRC_EN
#include "lrc.h"
#endif
#include "bt_manager.h"
#if defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN)

#if  (defined(FUNC_BROWSER_PARALLEL_EN)||defined(FUNC_BROWSER_TREE_EN)||defined(FUNC_SPECIFY_FOLDER_PLAY_EN))
char current_vol[8];//disk volume like 0:/, 1:/
#else
static char current_vol[8];//disk volume like 0:/, 1:/
#endif
#ifdef CFG_FUNC_LRC_EN
static int32_t LrcStartTime = -1;
#endif
MEDIA_PLAYER* gMediaPlayer = NULL;
FileType SongFileType;

extern uint32_t CmdErrCnt;

//跳过下列文件夹
#define SYS_FOLDER_NAME "System Volume Information"
#define RECYCLE_NAME	"$RECYCLE.BIN"
#define LOST_NAME		"LOST.DIR"

//快进快退 针对解码器有效性实施不同策略:
//解码器播控有效时，优先使用消息驱动解码器，依据文件时长判断进度和上下首
#define FF_FB_STEP                  2000
#define FF_PLAY_TIME_MAX			0x7FFFFFFF//快进封顶值。封顶后不再变更，等结束后设下一首
#define FB_PLAY_LAST				0xFFFFFFFF //快退封底为0，封底后再退即FB_PLAY_LAST，等结束后设上一首

bool MediaPlayerFileDecoderInit(void);
extern AudioDecoderContext *gAudioDecoders[DECODER_MAX_CHANNELS];
extern uint8_t MediaPlayDevice(void);
extern bool IsUDiskLink(void);//device_detect.c
#ifdef CFG_FUNC_RECORDER_EN
FRESULT f_open_recfile_by_num(FIL *filehandle, UINT Index);
bool RecFilePlayerInit(void);
#endif

#ifdef FUNC_BROWSER_PARALLEL_EN
extern uint32_t gStartFolderIndex;
extern uint32_t gFolderFocusing;
extern uint32_t gFileFocusing;// 1-N
extern uint32_t gStartFileIndex;
extern ff_file_win gBrowserFileWin[GUI_ROW_CNT_MAX];
extern ff_dir_win gBrowserDirWin[GUI_ROW_CNT_MAX];
#endif

bool FolderExclude_callback(FILINFO *finfo)
{

#ifdef SYS_FOLDER_NAME
	if(!strcmp(finfo->fname, SYS_FOLDER_NAME))
	{
		return TRUE;
	}
#endif
#ifdef RECYCLE_NAME
	if(!strcmp(finfo->fname, RECYCLE_NAME))
	{
		return TRUE;
	}
#endif
#ifdef LOST_NAME
	if(!strcmp(finfo->fname, LOST_NAME))
	{
		return TRUE;
	}
#endif
	return FALSE;
}
void UAndSDDiskUpgradeJudge(void)
{

#if FLASH_BOOT_EN
	if(MvaCountEn)
	{
		if(GetSystemMode() == ModeCardAudioPlay && MvaCountEn > 1)
		{
			SoftFlagRegister(SoftFlagMvaInCard);
		}
		else if(GetSystemMode() == ModeUDiskAudioPlay && MvaCountEn > 1)
		{
			SoftFlagRegister(SoftFlagMvaInUDisk);
		}
		MvaCountEn = 0;//进子文件夹后 关闭mva检索
	}
#endif

}
static bool HardwareInit(DEV_ID DevId)
{
	uint8_t Retry = 8;
	APP_DBG("hardware init start.\n");
	switch(DevId)
	{
#ifdef CFG_RES_CARD_USE
		case DEV_ID_SD:
		{
			CardPortInit(CFG_RES_CARD_GPIO);
			if(SDCard_Init() != NONE_ERR)
			{
				APP_DBG("Card init err.\n");
				return FALSE;
			}
			APP_DBG("SDCard Init Success!\n");
			strcpy(current_vol, MEDIA_VOLUME_STR_C);

			while(f_mount(&gMediaPlayer->gFatfs_sd, current_vol, 1) != FR_OK && --Retry);
			if(!Retry)
			{
				APP_DBG("SD卡挂载 失败\n");
				return FALSE;
			}
			APP_DBG("SD卡挂载  成功\n");
			return TRUE;
		}
#endif
#ifdef CFG_FUNC_UDISK_DETECT
		case DEV_ID_USB:
			strcpy(current_vol, MEDIA_VOLUME_STR_U);
			do
			{
				if(GetUdiscState() == DETECT_STATE_IN)
				{
					APP_DBG("OTG Host Init again 0\n");
					if(!OTG_HostInit())
					{
						vTaskDelay(220);
						if(GetUdiscState() == DETECT_STATE_IN)
						{
							APP_DBG("OTG Host Init again 1\n");
							if(!OTG_HostInit())
							{
								APP_DBG("Host Init Disk Error\n");
								return FALSE;
							}
						}
						else
						{
							APP_DBG("Disk plug out and return\n");
							return FALSE;
						}
					}
				}
				else
				{
					APP_DBG("Disk plug out return and return\n");
					return FALSE;
				}
			}while(f_mount(&gMediaPlayer->gFatfs_u, current_vol, 1) != FR_OK && Retry--);
			if(!Retry )
			{
				APP_DBG("USB挂载  失败\n");
				return FALSE;
			}
			SoftFlagRegister(SoftFlagUDiskEnum);
			APP_DBG("USB挂载  成功\n");
			return TRUE;
#endif
		default:
			break;
	}
	return FALSE;
}


#ifdef CFG_FUNC_LRC_EN
static bool PlayerParseLrc(void)
{
	uint32_t SeekTime = gMediaPlayer->CurPlayTime * 1000;

	if((gMediaPlayer->LrcRow.MaxLrcLen = (int16_t)LrcTextLengthGet(SeekTime)) >= 0)
	{
		TEXT_ENCODE_TYPE CharSet;
		bool bCheckFlag = FALSE;
		
		if(gMediaPlayer->LrcRow.MaxLrcLen > 128)
		{
			bCheckFlag = TRUE;
			gMediaPlayer->LrcRow.MaxLrcLen = 128;
		}

		memset(gMediaPlayer->LrcRow.LrcText, 0, 128);

		LrcInfoGet(&gMediaPlayer->LrcRow, SeekTime, 0);
		if(gMediaPlayer->LrcRow.StartTime + gMediaPlayer->LrcRow.Duration == LrcStartTime)
		{
			return FALSE;
		}
		// ??? 下面的代码需要确认执行时间，如果时间较长，影响定时的精确度，需要移到外面执行
		// ??? 编码转换
		CharSet = LrcEncodeTypeGet();
		//APP_DBG("CharSet = %d\n", CharSet);
		if(gMediaPlayer->LrcRow.MaxLrcLen > 0 
		  && !(CharSet == ENCODE_UNKNOWN || CharSet == ENCODE_GBK || CharSet == ENCODE_ANSI))
		{
#ifdef  CFG_FUNC_STRING_CONVERT_EN
			uint32_t  ConvertType = UNICODE_TO_GBK;
			uint8_t* TmpStr = gMediaPlayer->TempBuf2;

			if(CharSet == ENCODE_UTF8)
			{
				ConvertType = UTF8_TO_GBK;
			}
			else if(CharSet == ENCODE_UTF16_BIG)
			{
				ConvertType = UNICODE_BIG_TO_GBK;
			}
			//APP_DBG("ConvertType = %d\n", ConvertType);
			memset(TmpStr, 0, 128);

			gMediaPlayer->LrcRow.MaxLrcLen = (uint16_t)StringConvert(TmpStr,
			                                (uint32_t)gMediaPlayer->LrcRow.MaxLrcLen,
			                                gMediaPlayer->LrcRow.LrcText,
			                                (uint32_t)gMediaPlayer->LrcRow.MaxLrcLen,
			                                ConvertType);

			memcpy(gMediaPlayer->LrcRow.LrcText, TmpStr, gMediaPlayer->LrcRow.MaxLrcLen);
			memset(TmpStr, 0, 128);

			memset((void*)(gMediaPlayer->LrcRow.LrcText + gMediaPlayer->LrcRow.MaxLrcLen),
			               0, 128 - gMediaPlayer->LrcRow.MaxLrcLen);
#endif
			//gPlayContrl->LrcRow.LrcText[gPlayContrl->LrcRow.MaxLrcLen] = '\0';
		}

		if(bCheckFlag)
		{
			uint32_t i = 0;
			while(i < (uint32_t)gMediaPlayer->LrcRow.MaxLrcLen)
			{
				if(gMediaPlayer->LrcRow.LrcText[i] > 0x80)
				{
					i += 2;
				}
				else
				{
					i++;
				}
			}

			if(i >= (uint32_t)gMediaPlayer->LrcRow.MaxLrcLen)
			{
				gMediaPlayer->LrcRow.MaxLrcLen--;
				gMediaPlayer->LrcRow.LrcText[gMediaPlayer->LrcRow.MaxLrcLen] = '\0';
			}
		}

		APP_DBG("<%s>\r\n", gMediaPlayer->LrcRow.LrcText);
		
		LrcStartTime = gMediaPlayer->LrcRow.StartTime + gMediaPlayer->LrcRow.Duration;
		return TRUE;
	}

	APP_DBG("gMediaPlayer->LrcRow.MaxLrcLen = %d\n", gMediaPlayer->LrcRow.MaxLrcLen);
	if(gMediaPlayer->LrcRow.MaxLrcLen < 0)
	{
		gMediaPlayer->LrcRow.MaxLrcLen = 0;
	}

	return FALSE;	
}

void SearchAndOpenLrcFile(void)
{
	int32_t i;
	int32_t Len = 0;
	bool ret;
	//FOLDER CurFolder;		

	memset(gMediaPlayer->TempBuf2, 0, sizeof(gMediaPlayer->TempBuf2));
	
	//if(FR_OK != f_open_by_num(current_vol, &gMediaPlayer->PlayerFile, gMediaPlayer->CurFileIndex, (char*)file_longname))
	if(FR_OK != f_opendir_by_num(current_vol, &gMediaPlayer->PlayerFolder, gMediaPlayer->PlayerFile.dir_num, NULL))
	{
		APP_DBG("folder open err!!\n");
	}
	if(gMediaPlayer->file_longname[0] != 0)
	{
		i = strlen(gMediaPlayer->file_longname);
		while(--i)
		{
			if(gMediaPlayer->file_longname[i] == '.')
			{
				memcpy(&gMediaPlayer->TempBuf2[Len], gMediaPlayer->file_longname, i + 1);
				{
					gMediaPlayer->TempBuf2[i + 1 + Len] = (uint8_t)'l';
					gMediaPlayer->TempBuf2[i + 2 + Len] = (uint8_t)'r';
					gMediaPlayer->TempBuf2[i + 3 + Len] = (uint8_t)'c';
				}
				break;
			}
		}
		//APP_DBG("lrc file long Name: %s\n", TempBuf2);
	}
	else
	{
		//memcpy(TempBuf2, (void*)gMediaPlayer->PlayerFile.ShortName, 8);
		//memcpy(&gMediaPlayer->TempBuf2[Len], f_audio_get_name(gMediaPlayer->CurFileIndex), 8);//此处有问题，后续需要修改
		memcpy(&gMediaPlayer->TempBuf2[Len], gMediaPlayer->PlayerFile.fn, 8);
		memcpy(&gMediaPlayer->TempBuf2[Len + 8], (void*)"lrc", 3);
		APP_DBG("lrc file short Name: %s\n", gMediaPlayer->TempBuf2);
	}

	//ret = f_open(&gMediaPlayer->LrcFile, (TCHAR*)gMediaPlayer->TempBuf2, FA_READ);
	ret = f_open_by_name_in_dir(&gMediaPlayer->PlayerFolder, &gMediaPlayer->LrcFile, (TCHAR*)gMediaPlayer->TempBuf2);

	if(ret == FR_OK)
	{
		APP_DBG("open lrc file ok\n");
		gMediaPlayer->LrcRow.LrcText = gMediaPlayer->TempBuf1; // 歌词内存区映射
		gMediaPlayer->IsLrcRunning = TRUE;
		LrcInit(&gMediaPlayer->LrcFile, gMediaPlayer->ReadBuffer, sizeof(gMediaPlayer->ReadBuffer), &gMediaPlayer->LrcInfo);
	}
	else
	{
		gMediaPlayer->IsLrcRunning = FALSE;
		APP_DBG("No lrc\n");
	}

}
#endif


bool MediaPlayerInitialize(DEV_ID DeviceIndex, int32_t FileIndex, uint32_t FolderIndex)
{
	uint16_t i;//首播歌曲出现故障时跳下一首，直到初始化解码器成功的歌曲

	SetMediaPlayerState(PLAYER_STATE_IDLE);
	if(!HardwareInit(DeviceIndex))
	{
		APP_DBG("Hardware initialize error\n");
		return FALSE;
	}
	if(GetSystemMode() == ModeUDiskAudioPlay || GetSystemMode() == ModeCardAudioPlay)
	{
#if defined(FUNC_MATCH_PLAYER_BP) && (defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN))
		DiskSongSearchBPInit();
		ffpresearch_init(DeviceIndex == DEV_ID_SD ? &gMediaPlayer->gFatfs_sd : &gMediaPlayer->gFatfs_u, DiskSongSearchBP, FolderExclude_callback,gMediaPlayer->AccRamBlk);
#else
		ffpresearch_init(DeviceIndex == DEV_ID_SD ? &gMediaPlayer->gFatfs_sd : &gMediaPlayer->gFatfs_u, NULL, FolderExclude_callback,gMediaPlayer->AccRamBlk);
#endif
#if FLASH_BOOT_EN
		MvaCountEn = 1;
#endif
		if(FR_OK != f_scan_vol(current_vol))
		{
			APP_DBG("f_scan_vol err!");
			return FALSE;
		}

		UAndSDDiskUpgradeJudge();
		
		APP_DBG("Hardware initialize success.\n");

		//默认使用全盘播放第一个文件
		gMediaPlayer->CurFolderIndex = 1; // 根目录
#if defined(FUNC_MATCH_PLAYER_BP) && (defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN))
		gMediaPlayer->CurFileIndex = BPDiskSongNumGet();
		APP_DBG("CurFileIndex = %d\n", gMediaPlayer->CurFileIndex);
		gMediaPlayer->CurPlayTime = BPDiskSongPlayTimeGet();
		APP_DBG("CurPlayTime = %d\n", (int)gMediaPlayer->CurPlayTime);
		if(gMediaPlayer->CurFileIndex == 0)
#endif
		{
			gMediaPlayer->CurFileIndex = 1;
			gMediaPlayer->CurPlayTime = 0;
		}

		gMediaPlayer->TotalFileSumInDisk = f_file_real_quantity();
		gMediaPlayer->ValidFolderSumInDisk = f_dir_with_song_real_quantity();//f_dir_real_quantity();
		if(!SoftFlagGet(SoftFlagUpgradeOK)&&(SoftFlagGet(SoftFlagMvaInUDisk)||SoftFlagGet(SoftFlagMvaInCard)))
		{
			APP_DBG("MediaPlayerInitialize return,because MVA upgrade \n");
			return TRUE;
		}
		if(!gMediaPlayer->TotalFileSumInDisk)
		{
			return FALSE;
		}
		// 打开播放文件夹，默认全盘播放
		// 文件夹相关操作后续补充
	}
#ifdef CFG_FUNC_RECORDER_EN
	else //playback
	{
#ifdef CFG_FUNC_RECORD_UDISK_FIRST
		if(!RecFilePlayerInit())
		{
			return FALSE;
		}
#else //flashfs 回放
#ifdef CFG_FUNC_RECORD_FLASHFS
		if((gMediaPlayer->FlashFsFile = Fopen(CFG_PARA_FLASHFS_FILE_NAME, "r")) == NULL)//flashfs无有效文件
		{
			APP_DBG("open error");
			return FALSE;
		}
		Fclose(gMediaPlayer->FlashFsFile);
		gMediaPlayer->FlashFsFile = NULL;
		gMediaPlayer->FlashFsMemHandle.addr = NULL;//不做缓冲，由解码器直接获取
		gMediaPlayer->FlashFsMemHandle.mem_capacity = 0;
		gMediaPlayer->FlashFsMemHandle.mem_len = 0;
		gMediaPlayer->FlashFsMemHandle.p = 0;
#endif
		gMediaPlayer->TotalFileSumInDisk = 1;//flashfs有效时只有单个录音文件。
#endif
		gMediaPlayer->CurFolderIndex = 1; // 不在加速器计算内
		gMediaPlayer->ValidFolderSumInDisk = 1;
		gMediaPlayer->CurFileIndex = 1;
		gMediaPlayer->CurPlayTime = 0;
		gMediaPlayer->CurPlayMode = PLAY_MODE_REPEAT_ALL;//回放录音只有循环(倒序)播放，不支持播放模式切换
	}
#endif
	for(i = 0; i < gMediaPlayer->TotalFileSumInDisk; i++)
	{
		if(MediaPlayerFileDecoderInit())
		{
			break;
		}
		gMediaPlayer->CurFileIndex = (gMediaPlayer->CurFileIndex % gMediaPlayer->TotalFileSumInDisk) + 1;
	}
	if(i == gMediaPlayer->TotalFileSumInDisk)
	{
		return FALSE;//没有有效歌曲。
	}

	if((gMediaPlayer->CurPlayMode == PLAY_MODE_REPEAT_FOLDER || gMediaPlayer->CurPlayMode == PLAY_MODE_RANDOM_FOLDER)
		&& (GetSystemMode() == ModeUDiskAudioPlay || GetSystemMode() == ModeCardAudioPlay))
	{
		f_file_count_in_dir(&gMediaPlayer->PlayerFolder);//确认此文件夹 歌曲数量
	}
	MediaPlayerSwitchPlayMode(gMediaPlayer->CurPlayMode);

	SetMediaPlayerState(PLAYER_STATE_PLAYING);
	return TRUE;
}

void MediaPlayerDeinitialize(void)
{
	if(gMediaPlayer != NULL)
	{
	    osPortFree(gMediaPlayer);
	    gMediaPlayer = NULL;
	}
}

//文件解析 歌曲信息log
static void MediaPlayerSongInfoLog(void)
{
/*
	SongInfo* PlayingSongInfo = audio_decoder_get_song_info(gAudioDecoders[DECODER_MODE_CHANNEL]);

	APP_DBG("PlayCtrl:MediaPlayerSongInfoLog\n");
	if(PlayingSongInfo == NULL)
	{
		return ;
	}

#ifdef CFG_FUNC_LRC_EN
    APP_DBG("LRC:%d\n", gMediaPlayer->IsLrcRunning);
#else
    APP_DBG("LRC:0\n");
#endif
	APP_DBG("----------TAG Info----------\n");

	APP_DBG("CharSet:");
	switch(PlayingSongInfo->char_set)
	{
		case CHAR_SET_ISO_8859_1:
			APP_DBG("CHAR_SET_ISO_8859_1\n");
			break;
		case CHAR_SET_UTF_16:
			APP_DBG("CHAR_SET_UTF_16\n");
			break;
		case CHAR_SET_UTF_8:
			APP_DBG("CHAR_SET_UTF_8\n");
			break;
		default:
			APP_DBG("CHAR_SET_UNKOWN\n");
			break;
	}
#ifdef CFG_FUNC_STRING_CONVERT_EN
    if(PlayingSongInfo->char_set == CHAR_SET_UTF_8)
    {
        StringConvert(PlayingSongInfo->title,	  MAX_TAG_LEN, PlayingSongInfo->title,	   MAX_TAG_LEN, UTF8_TO_GBK);
        StringConvert(PlayingSongInfo->artist,	  MAX_TAG_LEN, PlayingSongInfo->artist,    MAX_TAG_LEN, UTF8_TO_GBK);
        StringConvert(PlayingSongInfo->album,	  MAX_TAG_LEN, PlayingSongInfo->album,	   MAX_TAG_LEN, UTF8_TO_GBK);
        StringConvert(PlayingSongInfo->comment,   MAX_TAG_LEN, PlayingSongInfo->comment,   MAX_TAG_LEN, UTF8_TO_GBK);
        StringConvert(PlayingSongInfo->genre_str, MAX_TAG_LEN, PlayingSongInfo->genre_str, MAX_TAG_LEN, UTF8_TO_GBK);
    }
    else if(PlayingSongInfo->char_set == CHAR_SET_UTF_16)
    {
    	if(PlayingSongInfo->stream_type == STREAM_TYPE_WMA)
		{
			StringConvert(PlayingSongInfo->title,	  MAX_TAG_LEN, &PlayingSongInfo->title[0],	   MAX_TAG_LEN, UNICODE_TO_GBK);
			StringConvert(PlayingSongInfo->artist,	  MAX_TAG_LEN, &PlayingSongInfo->artist[0],    MAX_TAG_LEN, UNICODE_TO_GBK);
			StringConvert(PlayingSongInfo->album,	  MAX_TAG_LEN, &PlayingSongInfo->album[0],	   MAX_TAG_LEN, UNICODE_TO_GBK);
			StringConvert(PlayingSongInfo->comment,   MAX_TAG_LEN, &PlayingSongInfo->comment[0],   MAX_TAG_LEN, UNICODE_TO_GBK);
			StringConvert(PlayingSongInfo->genre_str, MAX_TAG_LEN, &PlayingSongInfo->genre_str[0], MAX_TAG_LEN, UNICODE_TO_GBK);
		}
    	else
    	{
			if(PlayingSongInfo->title[0] == 0xFE && PlayingSongInfo->title[1] == 0xFF)
			{
				StringConvert(PlayingSongInfo->title,	  MAX_TAG_LEN, &PlayingSongInfo->title[2],	   MAX_TAG_LEN, UNICODE_BIG_TO_GBK);
				StringConvert(PlayingSongInfo->artist,	  MAX_TAG_LEN, &PlayingSongInfo->artist[2],    MAX_TAG_LEN, UNICODE_BIG_TO_GBK);
				StringConvert(PlayingSongInfo->album,	  MAX_TAG_LEN, &PlayingSongInfo->album[2],	   MAX_TAG_LEN, UNICODE_BIG_TO_GBK);
				StringConvert(PlayingSongInfo->comment,   MAX_TAG_LEN, &PlayingSongInfo->comment[2],   MAX_TAG_LEN, UNICODE_BIG_TO_GBK);
				StringConvert(PlayingSongInfo->genre_str, MAX_TAG_LEN, &PlayingSongInfo->genre_str[2], MAX_TAG_LEN, UNICODE_BIG_TO_GBK);
			}
			else
			{
				StringConvert(PlayingSongInfo->title,	  MAX_TAG_LEN, &PlayingSongInfo->title[2],	   MAX_TAG_LEN, UNICODE_TO_GBK);
				StringConvert(PlayingSongInfo->artist,	  MAX_TAG_LEN, &PlayingSongInfo->artist[2],    MAX_TAG_LEN, UNICODE_TO_GBK);
				StringConvert(PlayingSongInfo->album,	  MAX_TAG_LEN, &PlayingSongInfo->album[2],	   MAX_TAG_LEN, UNICODE_TO_GBK);
				StringConvert(PlayingSongInfo->comment,   MAX_TAG_LEN, &PlayingSongInfo->comment[2],   MAX_TAG_LEN, UNICODE_TO_GBK);
				StringConvert(PlayingSongInfo->genre_str, MAX_TAG_LEN, &PlayingSongInfo->genre_str[2], MAX_TAG_LEN, UNICODE_TO_GBK);
			}
    	}
    }
    else if((PlayingSongInfo->char_set & 0xF0000000) == CHAR_SET_DIVERSE)
    {
        uint32_t type = PlayingSongInfo->char_set & 0xF;

        if(type == CHAR_SET_UTF_8)
        {
        	 StringConvert(PlayingSongInfo->title,	  MAX_TAG_LEN, PlayingSongInfo->title,	   MAX_TAG_LEN, UTF8_TO_GBK);
        }
        else if(type == CHAR_SET_UTF_16)
        {
        	if(PlayingSongInfo->title[0] == 0xFF && PlayingSongInfo->title[1] == 0xFE)
        	{
        		StringConvert(PlayingSongInfo->title,	  MAX_TAG_LEN, &PlayingSongInfo->title[2],	   MAX_TAG_LEN, UNICODE_TO_GBK);
        	}
        	else if(PlayingSongInfo->title[0] == 0xFE && PlayingSongInfo->title[1] == 0xFF)
        	{
        		StringConvert(PlayingSongInfo->title,	  MAX_TAG_LEN, &PlayingSongInfo->title[2],	   MAX_TAG_LEN, UNICODE_BIG_TO_GBK);
        	}
        }

        type = (gAudioDecoders[DECODER_MODE_CHANNEL]->song_info.char_set >> 4)  & 0xF;
        if(type == CHAR_SET_UTF_8)
		{
        	StringConvert(PlayingSongInfo->artist,	  MAX_TAG_LEN, PlayingSongInfo->artist,	   MAX_TAG_LEN, UTF8_TO_GBK);
		}
		else if(type == CHAR_SET_UTF_16)
		{
			if(PlayingSongInfo->artist[0] == 0xFF && PlayingSongInfo->artist[1] == 0xFE)
			{
				StringConvert(PlayingSongInfo->artist,	  MAX_TAG_LEN, &PlayingSongInfo->artist[2],	   MAX_TAG_LEN, UNICODE_TO_GBK);
			}
			else if(PlayingSongInfo->artist[0] == 0xFE && PlayingSongInfo->artist[1] == 0xFF)
			{
				StringConvert(PlayingSongInfo->artist,	  MAX_TAG_LEN, &PlayingSongInfo->artist[2],	   MAX_TAG_LEN, UNICODE_BIG_TO_GBK);
			}
		}

        type = (gAudioDecoders[DECODER_MODE_CHANNEL]->song_info.char_set >> 8)  & 0xF;
        if(type == CHAR_SET_UTF_8)
		{
			StringConvert(PlayingSongInfo->album,	  MAX_TAG_LEN, PlayingSongInfo->album,	   MAX_TAG_LEN, UTF8_TO_GBK);
		}
		else if(type == CHAR_SET_UTF_16)
		{
			if(PlayingSongInfo->album[0] == 0xFF && PlayingSongInfo->album[1] == 0xFE)
			{
				StringConvert(PlayingSongInfo->album,	  MAX_TAG_LEN, &PlayingSongInfo->album[2],	   MAX_TAG_LEN, UNICODE_TO_GBK);
			}
			else if(PlayingSongInfo->album[0] == 0xFE && PlayingSongInfo->album[1] == 0xFF)
			{
				StringConvert(PlayingSongInfo->album,	  MAX_TAG_LEN, &PlayingSongInfo->album[2],	   MAX_TAG_LEN, UNICODE_BIG_TO_GBK);
			}
		}

        type = (gAudioDecoders[DECODER_MODE_CHANNEL]->song_info.char_set >> 12) & 0xF;
        if(type == CHAR_SET_UTF_8)
		{
			StringConvert(PlayingSongInfo->comment,	  MAX_TAG_LEN, PlayingSongInfo->comment,	   MAX_TAG_LEN, UTF8_TO_GBK);
		}
		else if(type == CHAR_SET_UTF_16)
		{
			if(PlayingSongInfo->comment[0] == 0xFF && PlayingSongInfo->comment[1] == 0xFE)
			{
				StringConvert(PlayingSongInfo->comment,	  MAX_TAG_LEN, &PlayingSongInfo->comment[2],	   MAX_TAG_LEN, UNICODE_TO_GBK);
			}
			else if(PlayingSongInfo->comment[0] == 0xFE && PlayingSongInfo->comment[1] == 0xFF)
			{
				StringConvert(PlayingSongInfo->comment,	  MAX_TAG_LEN, &PlayingSongInfo->comment[2],	   MAX_TAG_LEN, UNICODE_BIG_TO_GBK);
			}
		}

        type = (gAudioDecoders[DECODER_MODE_CHANNEL]->song_info.char_set >> 16) & 0xF;
        if(type == CHAR_SET_UTF_8)
		{
			StringConvert(PlayingSongInfo->genre_str,	  MAX_TAG_LEN, PlayingSongInfo->genre_str,	   MAX_TAG_LEN, UTF8_TO_GBK);
		}
		else if(type == CHAR_SET_UTF_16)
		{
			if(PlayingSongInfo->genre_str[0] == 0xFF && PlayingSongInfo->genre_str[1] == 0xFE)
			{
				StringConvert(PlayingSongInfo->genre_str,	  MAX_TAG_LEN, &PlayingSongInfo->genre_str[2],	   MAX_TAG_LEN, UNICODE_TO_GBK);
			}
			else if(PlayingSongInfo->genre_str[0] == 0xFE && PlayingSongInfo->genre_str[1] == 0xFF)
			{
				StringConvert(PlayingSongInfo->genre_str,	  MAX_TAG_LEN, &PlayingSongInfo->genre_str[2],	   MAX_TAG_LEN, UNICODE_BIG_TO_GBK);
			}
		}
    }
#endif
    
    APP_DBG("title: %s\n", PlayingSongInfo->title);
    APP_DBG("artist: %s\n", PlayingSongInfo->artist);
    APP_DBG("Album: %s\n", PlayingSongInfo->album);
    APP_DBG("comment: %s\n", PlayingSongInfo->comment);
    APP_DBG("genre: %d %s\n", PlayingSongInfo->genre, PlayingSongInfo->genre_str);
    APP_DBG("year: %s\n", PlayingSongInfo->year);
    
    APP_DBG("\n");
    APP_DBG("----------------------------\n");
    APP_DBG("**********Song Info*********\n");
    APP_DBG("SongType:");
    switch(PlayingSongInfo->stream_type)
    {
        case STREAM_TYPE_MP2:
            APP_DBG("MP2");
            break;
        case STREAM_TYPE_MP3:
            APP_DBG("MP3");
            break;
        case STREAM_TYPE_WMA:
            APP_DBG("WMA");
            break;
        case STREAM_TYPE_SBC:
            APP_DBG("SBC");
            break;
        case STREAM_TYPE_PCM:
            APP_DBG("PCM");
            break;
        case STREAM_TYPE_ADPCM:
            APP_DBG("ADPCM");
            break;
        case STREAM_TYPE_FLAC:
            APP_DBG("FLAC");
            break;
        case STREAM_TYPE_AAC:
            APP_DBG("AAC");
            break;
        default:
            APP_DBG("UNKNOWN");
            break;
    }
    APP_DBG("\n");
    APP_DBG("Chl Num:%d\n", (int)PlayingSongInfo->num_channels);
    APP_DBG("SampleRate:%d\n", (int)PlayingSongInfo->sampling_rate);
    APP_DBG("BitRate:%d\n", (int)PlayingSongInfo->bitrate);
    APP_DBG("File Size:%d\n", (int)PlayingSongInfo->file_size);
    APP_DBG("TotalPlayTime:%dms\n", (int)PlayingSongInfo->duration);
    APP_DBG("CurPlayTime:%dms\n", (int)gMediaPlayer->CurPlayTime);
    APP_DBG("IsVBR:%d\n", (int)PlayingSongInfo->vbr_flag);
    APP_DBG("MpegVer:");

    switch(mp3_decoder_get_mpeg_version(gAudioDecoders[DECODER_MODE_CHANNEL]))
    {
        case MPEG_VER2d5:
            APP_DBG("MPEG_2_5");
            break;
        case MPEG_VER1:
            APP_DBG("MPEG_1");
            break;
        case MPEG_VER2:
            APP_DBG("MPEG_2");
            break;
        default:
            APP_DBG("MPEG_UNKNOWN");
            break;
    }
    APP_DBG("\n");
    APP_DBG("Id3Ver:%d\n", (int)mp3_decoder_get_id3_version(gAudioDecoders[DECODER_MODE_CHANNEL]));

    APP_DBG("**************************\n");
*/
	return ;
}

// 根据媒体参数，更新audiocore声道
//根据媒体文件参数，调整Dac频率和采样率
static void MediaPlayerUpdataDecoderSourceNum(void)
{
	AudioCoreSourceEnable(DecoderSourceNumGet(DECODER_MODE_CHANNEL));
#ifndef CFG_FUNC_MIXER_SRC_EN
	SongInfo* PlayingSongInfo = audio_decoder_get_song_info(gAudioDecoders[DECODER_MODE_CHANNEL]);
#ifdef CFG_RES_AUDIO_DACX_EN
	AudioDAC_SampleRateChange(ALL, PlayingSongInfo->sampling_rate);
#endif

#ifdef CFG_RES_AUDIO_DAC0_EN
	AudioDAC_SampleRateChange(DAC0, PlayingSongInfo->sampling_rate);
#endif
	APP_DBG("DAC Sample rate = %d\n", (int)AudioDAC_SampleRateGet(DAC0));
#endif
}

//仅打开文件
bool MediaPlayerOpenSongFile(void)
{
    memset(&gMediaPlayer->PlayerFile, 0, sizeof(gMediaPlayer->PlayerFile));
	if(CmdErrCnt >= 3)//临时处理U盘异常但是U盘插着的情况
	{
		if(!IsMediaPlugOut())
		{
			SendModeKeyMsg();
		}
		SoftFlagRegister(SoftFlagMediaDevicePlutOut);
		CmdErrCnt = 0;
		return FALSE;
	}
#ifdef CFG_FUNC_RECORDER_EN
#ifdef CFG_FUNC_RECORD_UDISK_FIRST
	if(GetSystemMode() == ModeUDiskPlayBack || GetSystemMode() == ModeCardPlayBack)
	{
		if(FR_OK != f_open_recfile_by_num(&gMediaPlayer->PlayerFile, gMediaPlayer->CurFileIndex))
		{
			APP_DBG(("f_open_recfile_by_num() error!\n"));
			SendModeKeyMsg();
			return FALSE;
		}
	}
	else
#elif defined(CFG_FUNC_RECORD_FLASHFS)
	if(GetSystemMode() == AppModeFlashFsPlayBack)
	{
		APP_DBG("open flashfs");
		if((gMediaPlayer->FlashFsFile = Fopen(CFG_PARA_FLASHFS_FILE_NAME, "r")) == NULL)
		{
			return FALSE;
		}
	}
	else
#endif
#endif
	{
#ifdef FUNC_BROWSER_PARALLEL_EN
		if(gMediaPlayer->CurPlayMode == PLAY_MODE_BROWSER)
		{
			gMediaPlayer->PlayerFolder.FolderIndex = 0;
		}
#endif
		APP_DBG("CurFileIndex = %d\n", gMediaPlayer->CurFileIndex);
		{
			if(FR_OK != f_open_by_num(current_vol, &gMediaPlayer->PlayerFolder, &gMediaPlayer->PlayerFile, gMediaPlayer->CurFileIndex, gMediaPlayer->file_longname))
			{
				APP_DBG(("FileOpenByNum() error!\n"));
				if(!IsMediaPlugOut())
				{
					SendModeKeyMsg();
				}
				SoftFlagRegister(SoftFlagMediaDevicePlutOut);
				return FALSE;
			}
		}

		if(gMediaPlayer->PlayerFile.fn[0]==0)// bkd add for exfat short file name
		{
			memcpy(gMediaPlayer->PlayerFile.fn, gMediaPlayer->file_longname, FF_SFN_BUF);
			gMediaPlayer->PlayerFile.fn[FF_SFN_BUF]=0;
		}

		if(GetSystemMode() == ModeUDiskAudioPlay || GetSystemMode() == ModeCardAudioPlay)
		{
			gMediaPlayer->CurFolderIndex = f_dir_with_song_real_quantity_cur();
		}

		if(gMediaPlayer->file_longname[0] != 0)
		{
			APP_DBG("Song file long Name: %s\n", gMediaPlayer->file_longname);
			SongFileType = get_audio_type((TCHAR*)(gMediaPlayer->file_longname));
		}
		else
		{
			APP_DBG("Song Name: %s\n", gMediaPlayer->PlayerFile.fn);
			SongFileType = get_audio_type((TCHAR *)gMediaPlayer->PlayerFile.fn);
		}

#ifdef CFG_FUNC_LRC_EN
		SearchAndOpenLrcFile();
#endif
		//APP_DBG("Song type: %d\n", SongFileType);
	}

	return TRUE;
}

bool MediaPlayerDecoderInit(void)
{
#ifdef CFG_FUNC_RECORDER_EN
#if defined(CFG_FUNC_RECORD_SD_UDISK)
	if(GetSystemMode() == ModeUDiskPlayBack || GetSystemMode() == ModeCardPlayBack)
	{
		if(DecoderInit(&gMediaPlayer->PlayerFile,DECODER_MODE_CHANNEL, (int32_t)IO_TYPE_FILE, (int32_t)FILE_TYPE_MP3) != RT_SUCCESS)
		{
			APP_DBG("Decoder Init Err\n");
			return FALSE;
		}
	}
	else
#elif defined(CFG_FUNC_RECORD_FLASHFS)
	if(GetSystemMode() == AppModeFlashFsPlayBack)
	{

		mv_mread_callback_set(&gMediaPlayer->FlashFsMemHandle,FlashFsReadFile);

		if(DecoderInit(&gMediaPlayer->FlashFsMemHandle, (uint32_t)IO_TYPE_MEMORY, (uint32_t)MP3_DECODER) != RT_SUCCESS)
		{
			APP_DBG("Decoder Init Err\n");
			return FALSE;
		}

	}
	else
#endif
#endif
	{
		if(DecoderInit(&gMediaPlayer->PlayerFile,DECODER_MODE_CHANNEL,(int32_t)IO_TYPE_FILE, (int32_t)SongFileType) != RT_SUCCESS)
		{
			return FALSE;
		}
	}

    APP_DBG("Open File ok! i=%d, ptr=%d, filesize=%uKB\n", (int)gMediaPlayer->CurFileIndex, (int)f_tell(&gMediaPlayer->PlayerFile), (uint16_t)(f_size(&gMediaPlayer->PlayerFile)/1024));

	return TRUE;
}

//打开文件和解码器设置
bool MediaPlayerFileDecoderInit(void)
{
	if(!MediaPlayerOpenSongFile())
	{
		return FALSE;
	}

	if(MediaPlayerDecoderInit() == FALSE)
	{
		return FALSE;
	}
#ifdef FUNC_MATCH_PLAYER_BP
	if(gMediaPlayer->CurPlayTime != 0)// && (GetSystemMode() == ModeUDiskPlayBack || GetSystemMode() == ModeCardPlayBack))
	{
		if(DecoderSeek(gMediaPlayer->CurPlayTime,DECODER_MODE_CHANNEL) != TRUE) 
		{
			return FALSE;
		}
	}
#endif

	MediaPlayerSongInfoLog();
	MediaPlayerUpdataDecoderSourceNum();
	
    return TRUE;
}


//关闭播放文件
void MediaPlayerCloseSongFile(void)
{
#ifdef CFG_FUNC_RECORD_FLASHFS
    if(GetSystemMode() == AppModeFlashFsPlayBack && gMediaPlayer->FlashFsFile)
    {
		APP_DBG("Closefs");
    	Fclose(gMediaPlayer->FlashFsFile);
		mv_mread_callback_unset(&gMediaPlayer->FlashFsMemHandle);
		gMediaPlayer->FlashFsFile = NULL;
    }
	else
#endif
	{
		if(gMediaPlayer->PlayerFile.obj.fs && gMediaPlayer)
		{
			f_close(&gMediaPlayer->PlayerFile);
		}
	#ifdef CFG_FUNC_LRC_EN
		if(gMediaPlayer->LrcFile.obj.fs && gMediaPlayer)
		{
			f_close(&gMediaPlayer->LrcFile);
		}
	#endif
	}
}

//播放器 下一首 设定
void MediaPlayerNextSong(bool IsSongPlayEnd)
{
#ifdef CFG_FUNC_DISPLAY_EN
	MessageContext	msgSend;
#endif
	APP_DBG("PlayCtrl:MediaPlayerNextSong\n");
	if(CmdErrCnt >= 3)
	{
		if(!IsMediaPlugOut())
		{
			SendModeKeyMsg();
		}
		SoftFlagRegister(SoftFlagMediaDevicePlutOut);
		CmdErrCnt = 0;
		return ;
	}

	switch(gMediaPlayer->CurPlayMode)
	{
		case PLAY_MODE_RANDOM_ALL:
			gMediaPlayer->CurFileIndex = GetRandomNum((uint16_t)GetSysTick1MsCnt(), gMediaPlayer->TotalFileSumInDisk);
			break;
		case PLAY_MODE_RANDOM_FOLDER:
			if(!gMediaPlayer->PlayerFolder.FileNumLen
					|| gMediaPlayer->CurFileIndex < gMediaPlayer->PlayerFolder.FirstFileIndex
					|| gMediaPlayer->CurFileIndex > gMediaPlayer->PlayerFolder.FirstFileIndex + gMediaPlayer->PlayerFolder.FileNumLen - 1)
			{
				APP_DBG("No file in dir or file Index out dir!\n ");
			}
			gMediaPlayer->CurFileIndex = gMediaPlayer->PlayerFolder.FirstFileIndex + GetRandomNum((uint16_t)GetSysTick1MsCnt(), gMediaPlayer->PlayerFolder.FileNumLen) - 1;
			APP_DBG("Cur File Num = %d in Folder %d\n", gMediaPlayer->CurFileIndex - gMediaPlayer->PlayerFolder.FirstFileIndex + 1 , gMediaPlayer->PlayerFolder.FolderIndex);
			break;

		case PLAY_MODE_REPEAT_FOLDER:
			if(!gMediaPlayer->PlayerFolder.FileNumLen
					|| gMediaPlayer->CurFileIndex < gMediaPlayer->PlayerFolder.FirstFileIndex
					|| gMediaPlayer->CurFileIndex > gMediaPlayer->PlayerFolder.FirstFileIndex + gMediaPlayer->PlayerFolder.FileNumLen - 1)
			{
				APP_DBG("No file in dir or file Index out dir!\n ");
			}
			gMediaPlayer->CurFileIndex++;
			if(gMediaPlayer->CurFileIndex > gMediaPlayer->PlayerFolder.FirstFileIndex + gMediaPlayer->PlayerFolder.FileNumLen - 1)
			{
				gMediaPlayer->CurFileIndex = gMediaPlayer->PlayerFolder.FirstFileIndex;
			}
			APP_DBG("Cur File Num = %d in Folder %d\n", gMediaPlayer->CurFileIndex - gMediaPlayer->PlayerFolder.FirstFileIndex + 1 , gMediaPlayer->PlayerFolder.FolderIndex);
			break;
		case PLAY_MODE_REPEAT_ONE:
			if(IsSongPlayEnd)
			{
				break;
			}
		case PLAY_MODE_REPEAT_OFF:
		case PLAY_MODE_REPEAT_ALL:
			gMediaPlayer->CurFileIndex++;
			if(gMediaPlayer->CurFileIndex > gMediaPlayer->TotalFileSumInDisk)
			{
				gMediaPlayer->CurFileIndex = 1;
			}
			APP_DBG("Cur File Num = %d\n", gMediaPlayer->CurFileIndex);
			break;
#ifdef FUNC_BROWSER_PARALLEL_EN
		case PLAY_MODE_BROWSER:
			if(gMediaPlayer->CurPlayMode == PLAY_MODE_BROWSER && GetBrowserPlay_state() == Browser_Play_Normal)
			{
				APP_DBG("browser normal play,next song\n");
				BrowserDn(Browser_File);
			}
			else if(gMediaPlayer->CurPlayMode == PLAY_MODE_BROWSER && (GetBrowserPlay_state() != Browser_Play_Normal))
			{

				if(!gMediaPlayer->PlayerFolder.FileNumLen
					|| gMediaPlayer->CurFileIndex < gMediaPlayer->PlayerFolder.FirstFileIndex
					|| gMediaPlayer->CurFileIndex > gMediaPlayer->PlayerFolder.FirstFileIndex + gMediaPlayer->PlayerFolder.FileNumLen - 1)
				{//合法性检查
					APP_DBG("No file in dir or file Index out dir!\n ");
				}
				gMediaPlayer->CurFileIndex++;
				if(gMediaPlayer->CurFileIndex > gMediaPlayer->PlayerFolder.FirstFileIndex + gMediaPlayer->PlayerFolder.FileNumLen - 1)
				{
					gMediaPlayer->CurFileIndex = gMediaPlayer->PlayerFolder.FirstFileIndex;
				}
				APP_DBG("Cur File Num = %d in Folder %d\n", gMediaPlayer->CurFileIndex - gMediaPlayer->PlayerFolder.FirstFileIndex + 1, gMediaPlayer->PlayerFolder.FolderIndex);
			}
			else
			{
				//PLAY_MODE_BROWSER mode ,but not play,so play as repeat all
				gMediaPlayer->CurFileIndex++;
				if(gMediaPlayer->CurFileIndex > gMediaPlayer->TotalFileSumInDisk)
				{
					gMediaPlayer->CurFileIndex = 1;
				}
				//APP_DBG("browser not normal play,next song=CurFileIndex TotalFileSumInDisk\n",gpMediaPlayer->CurFileIndex,gpMediaPlayer->TotalFileSumInDisk);
			}
		break;
#endif
		default:
			break;
	}
	gMediaPlayer->CurPlayTime = 0;
	gMediaPlayer->SongSwitchFlag = 0;

	MediaPlayerRepeatABClear();
	SetMediaPlayerState(PLAYER_STATE_PLAYING);
	SoftFlagDeregister(SoftFlagMediaNextOrPrev);
	MediaPlayerSong();

#ifdef CFG_FUNC_DISPLAY_EN
	msgSend.msgId = MSG_DISPLAY_SERVICE_FILE_NUM;
	MessageSend(GetSysModeMsgHandle(), &msgSend);
#endif
}

//播放器 上一首 设定
void MediaPlayerPreSong(void)
{
#ifdef CFG_FUNC_DISPLAY_EN
	MessageContext	msgSend;
#endif
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
	APP_DBG("PlayCtrl:MediaPlayerPreSong\n");
	switch(gMediaPlayer->CurPlayMode)
	{
		case PLAY_MODE_RANDOM_ALL:
			gMediaPlayer->CurFileIndex = GetRandomNum((uint16_t)GetSysTick1MsCnt(), gMediaPlayer->TotalFileSumInDisk);
			break;
		case PLAY_MODE_RANDOM_FOLDER:
			if(!gMediaPlayer->PlayerFolder.FileNumLen
					|| gMediaPlayer->CurFileIndex < gMediaPlayer->PlayerFolder.FirstFileIndex
					|| gMediaPlayer->CurFileIndex > gMediaPlayer->PlayerFolder.FirstFileIndex + gMediaPlayer->PlayerFolder.FileNumLen - 1)
			{
				APP_DBG("No file in dir or file Index out dir!\n ");
			}
			gMediaPlayer->CurFileIndex = gMediaPlayer->PlayerFolder.FirstFileIndex + GetRandomNum((uint16_t)GetSysTick1MsCnt(), gMediaPlayer->PlayerFolder.FileNumLen) - 1;
			APP_DBG("Cur File Num = %d in Folder %d\n", gMediaPlayer->CurFileIndex - gMediaPlayer->PlayerFolder.FirstFileIndex + 1, gMediaPlayer->PlayerFolder.FolderIndex);
			break;

		case PLAY_MODE_REPEAT_FOLDER:
			if(!gMediaPlayer->PlayerFolder.FileNumLen
				|| gMediaPlayer->CurFileIndex < gMediaPlayer->PlayerFolder.FirstFileIndex
				|| gMediaPlayer->CurFileIndex > gMediaPlayer->PlayerFolder.FirstFileIndex + gMediaPlayer->PlayerFolder.FileNumLen - 1)
			{
				APP_DBG("No file in dir or file Index out dir!\n ");
			}
			gMediaPlayer->CurFileIndex--;
			if(gMediaPlayer->CurFileIndex < gMediaPlayer->PlayerFolder.FirstFileIndex)
			{
				gMediaPlayer->CurFileIndex = gMediaPlayer->PlayerFolder.FirstFileIndex  + gMediaPlayer->PlayerFolder.FileNumLen - 1;
			}
			APP_DBG("Cur File Num = %d in Folder %d\n", gMediaPlayer->CurFileIndex - gMediaPlayer->PlayerFolder.FirstFileIndex + 1 , gMediaPlayer->PlayerFolder.FolderIndex);
			break;

		case PLAY_MODE_REPEAT_OFF:
		case PLAY_MODE_REPEAT_ALL:
		case PLAY_MODE_REPEAT_ONE:
			gMediaPlayer->CurFileIndex--;
			if(gMediaPlayer->CurFileIndex < 1)
			{
				gMediaPlayer->CurFileIndex = gMediaPlayer->TotalFileSumInDisk;
				APP_DBG("gMediaPlayer->CurFileIndex = %d\n", gMediaPlayer->CurFileIndex);
			}			
			break;
#ifdef FUNC_BROWSER_PARALLEL_EN
		case PLAY_MODE_BROWSER:
			if(gMediaPlayer->CurPlayMode == PLAY_MODE_BROWSER && GetBrowserPlay_state() == Browser_Play_Normal)
			{
				APP_DBG("browser normal play,prev song\n");
				BrowserUp(Browser_File);
			}
			else if(gMediaPlayer->CurPlayMode == PLAY_MODE_BROWSER && (GetBrowserPlay_state() != Browser_Play_Normal))
			{

				if(!gMediaPlayer->PlayerFolder.FileNumLen
				|| gMediaPlayer->CurFileIndex < gMediaPlayer->PlayerFolder.FirstFileIndex
				|| gMediaPlayer->CurFileIndex > gMediaPlayer->PlayerFolder.FirstFileIndex + gMediaPlayer->PlayerFolder.FileNumLen - 1)
				{//合法性检查
					APP_DBG("No file in dir or file Index out dir!\n ");
				}
				gMediaPlayer->CurFileIndex--;
				if(gMediaPlayer->CurFileIndex < gMediaPlayer->PlayerFolder.FirstFileIndex)
				{
					gMediaPlayer->CurFileIndex = gMediaPlayer->PlayerFolder.FirstFileIndex + gMediaPlayer->PlayerFolder.FileNumLen - 1;
				}
				APP_DBG("Cur File Num = %d in Folder %d\n", gMediaPlayer->CurFileIndex - gMediaPlayer->PlayerFolder.FirstFileIndex + 1, gMediaPlayer->PlayerFolder.FolderIndex);

			}
			else
			{
				//PLAY_MODE_BROWSER mode ,but not play,so play as repeat all
				gMediaPlayer->CurFileIndex--;
				if(gMediaPlayer->CurFileIndex < 1)
				{
					gMediaPlayer->CurFileIndex = gMediaPlayer->TotalFileSumInDisk;
					APP_DBG("gpMediaPlayer->CurFileIndex = %d\n", gMediaPlayer->CurFileIndex);
				}
			}
			break;
#endif

		default:
			break;
	}	
	gMediaPlayer->CurPlayTime = 0;
	gMediaPlayer->SongSwitchFlag = 1;

	MediaPlayerRepeatABClear();
	SetMediaPlayerState(PLAYER_STATE_PLAYING);
	
	SoftFlagRegister(SoftFlagMediaNextOrPrev);
	MediaPlayerSong();

#ifdef CFG_FUNC_DISPLAY_EN
	msgSend.msgId = MSG_DISPLAY_SERVICE_FILE_NUM;
	MessageSend(GetSysModeMsgHandle(), &msgSend);
#endif
}

//播放器 暂停和播放 设定
void MediaPlayerPlayPause(void)
{
	switch(GetMediaPlayerState())
	{
		case PLAYER_STATE_PLAYING:
			SetMediaPlayerState(PLAYER_STATE_PAUSE);
			DecoderPause(DECODER_MODE_CHANNEL);
			break;
		case PLAYER_STATE_STOP:
		case PLAYER_STATE_IDLE:
			SetMediaPlayerState(PLAYER_STATE_PLAYING);
			MediaPlayerSong();
			break;
		case PLAYER_STATE_FB:
			DecoderPause(DECODER_MODE_CHANNEL);
			break;
		default:
			SetMediaPlayerState(PLAYER_STATE_PLAYING);
			DecoderResume(DECODER_MODE_CHANNEL);
			break;
	}	
}

// 播放器 停止态设定
void MediaPlayerStop(void)
{
	APP_DBG("PlayCtrl:MediaPlayerStop\n");	
	gMediaPlayer->CurPlayTime = 0;
	SetMediaPlayerState(PLAYER_STATE_STOP);
	DecoderMuteAndStop(DECODER_MODE_CHANNEL);
}

//播放器状态 快进设定
void MediaPlayerFastForward(void)
{
	APP_DBG("PlayCtrl:MediaPlayerFastForward\n");

	if (gMediaPlayer->RepeatAB.RepeatFlag == REPEAT_OPENED_PAUSE) 
	{
		return;
	}
	
	if((GetMediaPlayerState() == PLAYER_STATE_IDLE)	// 停止状态下，禁止快进、快退
		|| (GetMediaPlayerState() == PLAYER_STATE_STOP)
		|| (GetMediaPlayerState() == PLAYER_STATE_PAUSE))
	{
		return;
	}
	if(GetDecoderState(DECODER_MODE_CHANNEL) == DecoderStateNone)//decoder end 应该禁止此操作
	{
		MediaPlayerCloseSongFile();
		MediaPlayerNextSong(TRUE);
		//MediaPlayerFileDecoderInit();

		return;
	}
	DecoderFF(FF_FB_STEP,DECODER_MODE_CHANNEL);
	SetMediaPlayerState(PLAYER_STATE_FF);
	
}

//播放器状态 快退
void MediaPlayerFastBackward(void)
{
	APP_DBG("PlayCtrl:MediaPlayerFastBackward\n");

	if (gMediaPlayer->RepeatAB.RepeatFlag == REPEAT_OPENED_PAUSE) {
		return;
	}
	
	if (GetMediaPlayerState() == PLAYER_STATE_FB && GetDecoderState(DECODER_MODE_CHANNEL) == DecoderStatePause) {
		APP_DBG("Doesn't Fast Backward\n");	
		return;
	}
	
	
	
	if((GetMediaPlayerState() == PLAYER_STATE_IDLE)	// 停止状态，禁止快进、快退
		|| (GetMediaPlayerState() == PLAYER_STATE_STOP)
		|| (GetMediaPlayerState() == PLAYER_STATE_PAUSE))
	{
		return;
	}
	if(GetDecoderState(DECODER_MODE_CHANNEL) == DecoderStateNone)//解码器空闲，未初始化过，播放上一个文件。应该禁止此操作
	{
		MediaPlayerCloseSongFile();
		MediaPlayerPreSong();
		return;
	}

	DecoderFB(FF_FB_STEP,DECODER_MODE_CHANNEL);//由解码器获取文件时长，判断seek和判断上下首
	SetMediaPlayerState(PLAYER_STATE_FB);
	
}

//播放器状态 快进快退停止设定
void MediaPlayerFFFBEnd(void)
{
	APP_DBG("PlayCtrl:MediaPlayerFFFBEnd\n");
	if((GetMediaPlayerState() == PLAYER_STATE_IDLE) // 停止状态，禁止快进、快退
		|| (GetMediaPlayerState() == PLAYER_STATE_STOP)
		|| (GetMediaPlayerState() == PLAYER_STATE_PAUSE))
	{
		return;
	}

	DecoderResume(DECODER_MODE_CHANNEL);
	if(gMediaPlayer->RepeatAB.RepeatFlag == 3)
	{
		gMediaPlayer->RepeatAB.RepeatFlag = 2;
		if(GetMediaPlayerState() == PLAYER_STATE_FF)
		{
			DecoderFB(gMediaPlayer->RepeatAB.Times * 1000,DECODER_MODE_CHANNEL);
		} 
	}
	SetMediaPlayerState(PLAYER_STATE_PLAYING);
	
}

//play模式切换,指定模式/或循环  不打断歌曲播放，
//此api前提是当前需要播放的文件必须已经open。否则无法刷新文件夹信息
void MediaPlayerSwitchPlayMode(uint8_t PlayMode)
{
	if(GetSystemMode() == ModeUDiskAudioPlay
	|| GetSystemMode() == ModeCardAudioPlay
#ifdef CFG_FUNC_RECORDER_EN
	|| GetSystemMode() == ModeUDiskPlayBack
	|| GetSystemMode() == ModeCardPlayBack
#endif
	)
	{
		APP_DBG("PlayCtrl:MediaPlayerSwitchPlayMode\n");

		if(PlayMode < PLAY_MODE_SUM)
		{
			gMediaPlayer->CurPlayMode = PlayMode;
		}
		else
		{
#ifdef FUNC_BROWSER_PARALLEL_EN
			if(gMediaPlayer->CurPlayMode == PLAY_MODE_RANDOM_FOLDER)
			{
				EnterBrowserPlayMode();
			}
			else if(gMediaPlayer->CurPlayMode == PLAY_MODE_BROWSER)
			{
				ExitBrowserPlayMode();
			}
			else
#endif
			{
				gMediaPlayer->CurPlayMode++;
				gMediaPlayer->CurPlayMode %= PLAY_MODE_SUM;
			}
			
			if(gMediaPlayer->CurPlayMode==PLAY_MODE_PREVIEW_PLAY)
			{
				MediaPlayerStop();
				gMediaPlayer->CurPlayTime = 0;
				gMediaPlayer->SongSwitchFlag = 0;
				SetMediaPlayerState(PLAYER_STATE_PLAYING);
				MediaPlayerSong();
			}
		}
		
		APP_DBG("media CurPlayMode = %d\n", gMediaPlayer->CurPlayMode);
#ifdef CFG_FUNC_BREAKPOINT_EN
		BackupInfoUpdata(BACKUP_PLAYER_INFO);
#ifdef BP_PART_SAVE_TO_NVM
		BackupInfoUpdata(BACKUP_PLAYER_INFO_2NVM);
#endif
#endif
	}
}


//播放器 上一文件夹 设定
void MediaPlayerPreFolder(void)
{
	uint16_t CurFileIndexTemp=0;

	if((GetSystemMode() == ModeUDiskAudioPlay || GetSystemMode() == ModeCardAudioPlay)
			&& gMediaPlayer->ValidFolderSumInDisk > 1)//
	{
	
		APP_DBG("PlayCtrl:MediaPlayerPreFolder\n");
		DecoderMuteAndStop(DECODER_MODE_CHANNEL);
		
		f_file_count_in_dir(&gMediaPlayer->PlayerFolder);

		if(!gMediaPlayer->PlayerFolder.FileNumLen) //当前文件夹内文件数量不能为0
		{
			APP_DBG("No play file in folder!\n ");
			return;
		}
		
		if(gMediaPlayer->PlayerFolder.FirstFileIndex == 1)//第一个有效文件夹，切换最后一个文件夹，先播最后一个
		{
			CurFileIndexTemp = gMediaPlayer->TotalFileSumInDisk;
		}
		else //换前一个文件夹最后一首
		{
			CurFileIndexTemp = gMediaPlayer->PlayerFolder.FirstFileIndex - 1;
		}
		gMediaPlayer->CurPlayTime = 0;
		gMediaPlayer->SongSwitchFlag = 1;
		
		//非当前folder 必须重新打开文件号，get文件夹号
		if(FR_OK != f_open_by_num(current_vol, &gMediaPlayer->PlayerFolder, &gMediaPlayer->PlayerFile, CurFileIndexTemp, gMediaPlayer->file_longname))
		{
			APP_DBG(("FileOpenByNum() error!\n"));
			if(!IsMediaPlugOut())
			{
				SendModeKeyMsg();
			}
			SoftFlagRegister(SoftFlagMediaDevicePlutOut);
			return;
		}
		
		f_close(&gMediaPlayer->PlayerFile);
		
		
		gMediaPlayer->CurFileIndex=gMediaPlayer->PlayerFolder.FirstFileIndex;//gMediaPlayer->PlayerFolder.FirstFileIndex;
		
				
		if(FR_OK != f_open_by_num(current_vol, &gMediaPlayer->PlayerFolder, &gMediaPlayer->PlayerFile, gMediaPlayer->CurFileIndex, gMediaPlayer->file_longname))
		{
			APP_DBG(("FileOpenByNum() error!\n"));
			if(!IsMediaPlugOut())
			{
				SendModeKeyMsg();
			}
			SoftFlagRegister(SoftFlagMediaDevicePlutOut);
			return;
		}

		
		MediaPlayerSong();
		
		gMediaPlayer->CurFolderIndex = f_dir_with_song_real_quantity_cur();

		f_file_count_in_dir(&gMediaPlayer->PlayerFolder);

	}
}

//播放器 下一文件夹 设定
void MediaPlayerNextFolder(void)
{
	if((GetSystemMode() == ModeUDiskAudioPlay || GetSystemMode() == ModeCardAudioPlay)
			&& gMediaPlayer->ValidFolderSumInDisk > 1)
	{
		
		APP_DBG("PlayCtrl:MediaPlayerNextFolder\n");
		
		DecoderMuteAndStop(DECODER_MODE_CHANNEL);
		f_file_count_in_dir(&gMediaPlayer->PlayerFolder);

		if(!gMediaPlayer->PlayerFolder.FileNumLen) //当前文件夹内文件数量不能为0
		{
			APP_DBG("No play file in folder!\n ");
			return;
		}
		if(gMediaPlayer->PlayerFolder.FirstFileIndex + gMediaPlayer->PlayerFolder.FileNumLen - 1 == gMediaPlayer->TotalFileSumInDisk)//最后一个有效文件夹，切换第一个文件夹，先播第一首
		{
			gMediaPlayer->CurFileIndex = 1;
		}
		else //换下一个文件夹第一
		{
			gMediaPlayer->CurFileIndex = gMediaPlayer->PlayerFolder.FirstFileIndex+ gMediaPlayer->PlayerFolder.FileNumLen;
		}
		gMediaPlayer->CurPlayTime = 0;
		gMediaPlayer->SongSwitchFlag = 0;
		if(FR_OK != f_open_by_num(current_vol, &gMediaPlayer->PlayerFolder, &gMediaPlayer->PlayerFile, gMediaPlayer->CurFileIndex, gMediaPlayer->file_longname))
		{
			APP_DBG(("FileOpenByNum() error!\n"));
			if(!IsMediaPlugOut())
			{
				SendModeKeyMsg();
			}
			SoftFlagRegister(SoftFlagMediaDevicePlutOut);
			return;
		}
		
		MediaPlayerSong();

		gMediaPlayer->CurFolderIndex = f_dir_with_song_real_quantity_cur();

		f_file_count_in_dir(&gMediaPlayer->PlayerFolder);

	}
}


void MediaPlayerRepeatABClear(void)
{
	if(gMediaPlayer->RepeatAB.RepeatFlag & 0x03)
	{
		gMediaPlayer->RepeatAB.RepeatFlag = 0;
		gMediaPlayer->RepeatAB.StartTime = 0;
		gMediaPlayer->RepeatAB.Times = 0;
	}
}

void MediaPlayerTimerCB(void)
{
	if(GetMediaPlayerState() == PLAYER_STATE_PLAYING
			&& (gMediaPlayer->CurPlayTime >= (gMediaPlayer->RepeatAB.StartTime + gMediaPlayer->RepeatAB.Times)))
	{
		DecoderFB(gMediaPlayer->RepeatAB.Times * 1000,DECODER_MODE_CHANNEL);
		APP_DBG("Repeat Mode running\n");
	}
	else if(GetMediaPlayerState() == PLAYER_STATE_FF
			&& (gMediaPlayer->CurPlayTime >= (gMediaPlayer->RepeatAB.StartTime + gMediaPlayer->RepeatAB.Times)))
	{
		gMediaPlayer->RepeatAB.RepeatFlag = 3;
		DecoderFB(gMediaPlayer->CurPlayTime * 1000 - (gMediaPlayer->RepeatAB.StartTime + gMediaPlayer->RepeatAB.Times) * 1000,DECODER_MODE_CHANNEL);
		gMediaPlayer->CurPlayTime = gMediaPlayer->RepeatAB.StartTime + gMediaPlayer->RepeatAB.Times;
		DecoderPause(DECODER_MODE_CHANNEL);
		APP_DBG("FF: Pause the AB\n");
	}
	else if(GetMediaPlayerState() == PLAYER_STATE_FB
			&& (gMediaPlayer->CurPlayTime <= gMediaPlayer->RepeatAB.StartTime))
	{
		gMediaPlayer->RepeatAB.RepeatFlag = 3;
		//AudioCoreSourceMute(APP_SOURCE_NUM, TRUE, TRUE);
		DecoderFF((gMediaPlayer->RepeatAB.StartTime - gMediaPlayer->CurPlayTime) * 1000,DECODER_MODE_CHANNEL);
		//AudioCoreSourceUnmute(APP_SOURCE_NUM, TRUE, TRUE);
		DecoderPause(DECODER_MODE_CHANNEL);
		APP_DBG("FB: Pause The AB");
	}
}


void MediaPlayerRepeatAB(void)
{
	if(GetMediaPlayerState() != PLAYER_STATE_PLAYING||
		(GetSystemMode() != ModeCardAudioPlay
		&& GetSystemMode() != ModeUDiskAudioPlay
#ifdef CFG_FUNC_RECORDER_EN
		&& GetSystemMode() != ModeUDiskPlayBack
		&& GetSystemMode() != ModeCardPlayBack
#endif
		))
	{
		APP_DBG("does't response RepeateAB mode\n");
		return;
	} 

	switch (gMediaPlayer->RepeatAB.RepeatFlag)
	{
		case REPEAT_CLOSED:
			gMediaPlayer->RepeatAB.StartTime  = gMediaPlayer->CurPlayTime;
			gMediaPlayer->RepeatAB.RepeatFlag = REPEAT_A_SETED;
			APP_DBG("Set RepeatAB StartTime = %d s\n", gMediaPlayer->RepeatAB.StartTime);
			break;
		case REPEAT_A_SETED:
			if (gMediaPlayer->CurPlayTime <= gMediaPlayer->RepeatAB.StartTime) 
			{
				gMediaPlayer->RepeatAB.RepeatFlag = REPEAT_CLOSED;
				APP_DBG("RepeatAB fail, start time <= end time\n");
				break;
			}
			gMediaPlayer->RepeatAB.Times      = (gMediaPlayer->CurPlayTime - gMediaPlayer->RepeatAB.StartTime);
			gMediaPlayer->RepeatAB.RepeatFlag = REPEAT_OPENED;
			APP_DBG("Set RepeatAB Duration = %d s\n", gMediaPlayer->RepeatAB.Times);
			break;

		case REPEAT_OPENED:
			APP_DBG("open repeat mode\n");
			gMediaPlayer->RepeatAB.RepeatFlag = REPEAT_CLOSED;
			APP_DBG("Repeat Mode Over\n");
			break;

		default:
			gMediaPlayer->RepeatAB.RepeatFlag = REPEAT_CLOSED;
			APP_DBG("Cancel RepeatAB\n");
			break;
	}
}



void MediaPlayerTimeUpdate(void)
{

	if((GetMediaPlayerState() == PLAYER_STATE_PLAYING) || (GetMediaPlayerState() == PLAYER_STATE_FF) || (GetMediaPlayerState() == PLAYER_STATE_FB))
	{
		gMediaPlayer->CurPlayTime = DecoderServicePlayTimeGet(DECODER_MODE_CHANNEL);
#if  (defined(FUNC_BROWSER_PARALLEL_EN) || defined(FUNC_BROWSER_TREE_EN))
		if(GetShowGuiTime()==0)
#endif
		{
			if(MediaPlayDevice() == DEV_ID_USB)
			{
				APP_DBG("USB ");
			}
			else if(MediaPlayDevice() == DEV_ID_SD)
			{
				APP_DBG("SD ");
			}

			if((gMediaPlayer->CurPlayMode == PLAY_MODE_RANDOM_FOLDER) || (gMediaPlayer->CurPlayMode == PLAY_MODE_REPEAT_FOLDER))
			{
				APP_DBG("F(%d/%d, %d/%d) ",(int)gMediaPlayer->CurFolderIndex,(int)gMediaPlayer->ValidFolderSumInDisk,(int)(gMediaPlayer->CurFileIndex - gMediaPlayer->PlayerFolder.FirstFileIndex + 1),(int)gMediaPlayer->PlayerFolder.FileNumLen);
			}
#ifdef FUNC_BROWSER_PARALLEL_EN
			else if(gMediaPlayer->CurPlayMode == PLAY_MODE_BROWSER && GetBrowserPlay_state() == Browser_Play_Normal)
			{
				APP_DBG("^F(%d/%d, %d/%d) ",
						//(int)gpMediaPlayer->PlayerFolder.FolderIndex,
						(int)(gStartFolderIndex+gFolderFocusing-1),
						(int)gMediaPlayer->ValidFolderSumInDisk,
						(int)(gMediaPlayer->CurFileIndex - gMediaPlayer->PlayerFolder.FirstFileIndex + 1),
						(int)gMediaPlayer->PlayerFolder.FileNumLen);
			}
			else if(gMediaPlayer->CurPlayMode == PLAY_MODE_BROWSER && GetBrowserPlay_state() != Browser_Play_Normal)
			{
				APP_DBG("^F(%d/%d, %d/%d) ",
						//(int)gpMediaPlayer->PlayerFolder.FolderIndex,
						(int)gMediaPlayer->CurFolderIndex, //(int)(f_dir_with_song_real_quantity_cur()),
						(int)gMediaPlayer->ValidFolderSumInDisk,
						(int)(gMediaPlayer->CurFileIndex - gMediaPlayer->PlayerFolder.FirstFileIndex + 1),
						(int)gMediaPlayer->PlayerFolder.FileNumLen);
			}
#endif
			else
			{
				APP_DBG("D(%d/%d, %d/%d) ",(int)gMediaPlayer->CurFolderIndex,(int)gMediaPlayer->ValidFolderSumInDisk,(int)gMediaPlayer->CurFileIndex,(int)gMediaPlayer->TotalFileSumInDisk);
			}
	#ifdef CFG_FUNC_RECORDER_EN
	#if defined(CFG_FUNC_RECORD_SD_UDISK)
			if(GetSystemMode() == ModeUDiskPlayBack || GetSystemMode() == ModeCardPlayBack)
			{
				TCHAR NameStr[FILE_PATH_LEN];
				IntToStrMP3Name(NameStr, gMediaPlayer->RecFileList[gMediaPlayer->CurFileIndex - 1]);
				APP_DBG("^%s, %02d:%02d ",
						NameStr,
						(int)(gMediaPlayer->CurPlayTime ) / 60,
						(int)(gMediaPlayer->CurPlayTime ) % 60);

			}
			else
	#elif defined(CFG_FUNC_RECORD_FLASHFS)
			if(GetSystemMode() == AppModeFlashFsPlayBack)
			{
				APP_DBG("^%s, %02d:%02d ",
						CFG_PARA_FLASHFS_FILE_NAME,
						(int)(gMediaPlayer->CurPlayTime ) / 60,
						(int)(gMediaPlayer->CurPlayTime ) % 60);
			}
			else
	#endif
	#endif
			{
				APP_DBG("%s, %02d:%02d ",gMediaPlayer->PlayerFile.fn,(int)(gMediaPlayer->CurPlayTime ) / 60,(int)(gMediaPlayer->CurPlayTime ) % 60);
			}
			switch(gMediaPlayer->CurPlayMode)
			{
				case PLAY_MODE_REPEAT_ONE:
					APP_DBG("RP_ONE ");
					break;
				case PLAY_MODE_REPEAT_ALL:
					APP_DBG("RP_ALL ");
					break;
				case PLAY_MODE_REPEAT_FOLDER:
					APP_DBG("RP_FOLDER ");
					break;
				case PLAY_MODE_RANDOM_FOLDER:
					APP_DBG("RDM_FOLDER ");
					break;
				case PLAY_MODE_RANDOM_ALL:
					APP_DBG("RDM_ALL ");
					break;
				case PLAY_MODE_REPEAT_OFF:
					APP_DBG("RP_OFF ");
					break;
#ifdef FUNC_BROWSER_PARALLEL_EN
				case PLAY_MODE_BROWSER:
					APP_DBG("BROWSER ");
					break;
#endif
				default:
					break;
			}
			APP_DBG("\n");
		}
#if  (defined(FUNC_BROWSER_PARALLEL_EN)||defined(FUNC_BROWSER_TREE_EN))
		else
			DecShowGuiTime();
#endif
#ifdef BP_PART_SAVE_TO_NVM
		if((GetSystemMode() == ModeUDiskAudioPlay || GetSystemMode() == ModeCardAudioPlay))
		{
			BackupInfoUpdata(BACKUP_PLAYER_INFO_2NVM);
		}
#endif
	}
#ifdef CFG_FUNC_LRC_EN
	if((GetSystemMode() == ModeUDiskAudioPlay || GetSystemMode() == ModeCardAudioPlay) && gMediaPlayer->IsLrcRunning)
	{
		PlayerParseLrc();
	}
#endif // FUNC_LRC_EN
}

//刷新解码配置
bool MediaPlayerDecoderRefresh(uint32_t SeekTime)
{
	//SoftFlagDeregister(SoftFlagDecoderMask & ~SoftFlagDecoderApp);//清理非App对解码器的登记
	if(!MediaPlayerDecoderInit())
	{
		return FALSE;
	}
	if(SeekTime != 0)
	{
		DecoderSeek(SeekTime,DECODER_MODE_CHANNEL);
	}
	MediaPlayerSongInfoLog();
	MediaPlayerUpdataDecoderSourceNum();
    return TRUE;
}


bool MediaPlayerSong(void)
{
	//SoftFlagDeregister(SoftFlagDecoderMask & ~SoftFlagDecoderApp);//清理非App对解码器的登记
	if(gMediaPlayer->CurPlayTime == FF_PLAY_TIME_MAX)
	{
		MediaPlayerNextSong(TRUE);
		return TRUE;
	}
	else if(gMediaPlayer->CurPlayTime == FB_PLAY_LAST)
	{
		MediaPlayerPreSong();
		return TRUE;
	}
	MediaPlayerCloseSongFile();//存在重复关闭情况，需兼容。

	if(!MediaPlayerOpenSongFile())
	{
		return FALSE;//文件错误
	}
	
#ifdef TWS_CODE_BACKUP//BT_TWS_SUPPORT
	if( GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)	
	{
#ifdef CFG_FUNC_REMIND_SOUND_EN
		if(RemindSoundIsPlay() <= 1)
#endif
		{
			if(IsAudioPlayerMute() == FALSE)
			{
				HardWareMuteOrUnMute();
			}	
		}
		g_tws_need_init = 1;
	}
#endif	

	//解码初始化失败(无效文件)，保持依据历史操作，切换上下首
	if(!MediaPlayerDecoderRefresh(gMediaPlayer->CurPlayTime))
	{
	
		MessageContext		msgSend;
		if((gMediaPlayer->CurPlayMode == PLAY_MODE_REPEAT_FOLDER) || (gMediaPlayer->CurPlayMode == PLAY_MODE_RANDOM_FOLDER))
		{
			if(gMediaPlayer->ErrFileCount)
			{
				gMediaPlayer->ErrFileCount --;
				if(!gMediaPlayer->ErrFileCount)
				{
					msgSend.msgId		= MSG_FOLDER_NEXT;
					MessageSend(GetSysModeMsgHandle(), &msgSend);
					return FALSE;
				}
			}		
		}
		
		APP_DBG("play song fail ,jump song\n");
		
		msgSend.msgId = MSG_SOFT_NEXT;
		if(SoftFlagGet(SoftFlagMediaNextOrPrev))
		{
			msgSend.msgId = MSG_SOFT_PRE;
		}
		SoftFlagDeregister(SoftFlagMediaNextOrPrev);
		
		MessageSend(GetSysModeMsgHandle(), &msgSend);

		return FALSE;
	}
    else
	{
		if((gMediaPlayer->CurPlayMode == PLAY_MODE_REPEAT_FOLDER) || (gMediaPlayer->CurPlayMode == PLAY_MODE_RANDOM_FOLDER))
		{
			gMediaPlayer->ErrFileCount = gMediaPlayer->PlayerFolder.FileNumLen;
		}
	}
	switch(GetMediaPlayerState())
	{
		case PLAYER_STATE_PLAYING:
		case PLAYER_STATE_FF:
		case PLAYER_STATE_FB:
			SetMediaPlayerState(PLAYER_STATE_PLAYING);//
			DecoderPlay(DECODER_MODE_CHANNEL);
			break;
		case PLAYER_STATE_IDLE:
		case PLAYER_STATE_PAUSE:
			APP_DBG("Pause\n");
			SetMediaPlayerState(PLAYER_STATE_PAUSE);
			DecoderPause(DECODER_MODE_CHANNEL);
			break;
		case PLAYER_STATE_STOP:
			APP_DBG("STATE_STOP\n");
			break;
	}
	//此处是否判断快进超过歌曲长度，改为下一首，当前是解码器自动。
#ifdef CFG_FUNC_BREAKPOINT_EN
	if(GetSystemMode() == ModeCardAudioPlay || GetSystemMode() == ModeUDiskAudioPlay)
	{
		BackupInfoUpdata(BACKUP_SYS_INFO);
		BackupInfoUpdata(BACKUP_PLAYER_INFO);
#ifdef BP_PART_SAVE_TO_NVM
		BackupInfoUpdata(BACKUP_PLAYER_INFO_2NVM);
#endif
	}
#endif
	return TRUE;
}


//播放器 获取其状态
uint8_t GetMediaPlayerState(void)
{
	if(gMediaPlayer != NULL)
	{
		return gMediaPlayer->CurPlayState;
	}

	return PLAYER_STATE_IDLE;
}

//播放器 状态设置
void SetMediaPlayerState(uint8_t state)
{
	if(gMediaPlayer != NULL)
	{
		if(gMediaPlayer->CurPlayState != state)
		{
			APP_DBG("PlayCtrl:SetMediaPlayerState %d\n", state);
			gMediaPlayer->CurPlayState = state;
		}
	}
}
#ifdef CFG_FUNC_RECORDER_EN
#ifdef CFG_FUNC_RECORD_SD_UDISK
//录音文件夹 检索排序
bool RecFilePlayerInit(void)
{
	FRESULT res;
	TCHAR PathStr[16];
	uint16_t j, k, i = 0;
	uint16_t	Backup;

	strcpy(PathStr, current_vol);
	strcat(PathStr, CFG_PARA_RECORDS_FOLDER);
	f_chdrive(current_vol);
	res = f_opendir(&gMediaPlayer->Dir, PathStr);
	if(res != FR_OK)
	{
		APP_DBG("f_opendir failed: %s ret:%d\n", PathStr, res);
		return FALSE;
	}

	if(gMediaPlayer->RecFileList == NULL)
	{
		gMediaPlayer->RecFileList = (uint16_t*)osPortMalloc(FILE_INDEX_MAX * FILE_NAME_VALUE_SIZE);//录音文件的最大数量。
		if(gMediaPlayer->RecFileList == NULL)
		{
			return FALSE;
		}
	}
    memset(gMediaPlayer->RecFileList, 0, FILE_INDEX_MAX * FILE_NAME_VALUE_SIZE);
	while(((res = f_readdir(&gMediaPlayer->Dir, &gMediaPlayer->FileInfo)) == FR_OK) && gMediaPlayer->FileInfo.fname[0])// && i != FILE_INDEX_MAX)
	{
		if(gMediaPlayer->FileInfo.fattrib & AM_ARC && gMediaPlayer->FileInfo.fsize)//跳过非存档文件、长度为0的文件。
		{
			k = RecFileIndex(gMediaPlayer->FileInfo.fname);//
			if(k)
			{
				for(j = 0; j <= i && j < FILE_INDEX_MAX; j++)//排序，检查,正常录音时顺序无需排序。
				{
					if(gMediaPlayer->RecFileList[j] < k)
					{
						Backup = gMediaPlayer->RecFileList[j];
						gMediaPlayer->RecFileList[j] = k;
						k = Backup;
					}
				}
				i++;
			}
		}
	}
	gMediaPlayer->TotalFileSumInDisk = i < FILE_INDEX_MAX ? i : FILE_INDEX_MAX;
	if(gMediaPlayer->TotalFileSumInDisk == 0)
	{
		APP_DBG("No Rec File\n");
		return FALSE;
	}
	APP_DBG("RecList:%d\n", gMediaPlayer->TotalFileSumInDisk);
	f_closedir(&gMediaPlayer->Dir);
	return TRUE;
}

//依据编号打开 录音文件
FRESULT f_open_recfile_by_num(FIL *filehandle, UINT Index)
{
	FRESULT ret;
	TCHAR PathStr[25];

	if(Index > 255 || Index > gMediaPlayer->TotalFileSumInDisk || Index == 0)
	{
		return FR_NO_FILE;
	}
	Index--;
	strcpy(PathStr, current_vol);
	strcat(PathStr, CFG_PARA_RECORDS_FOLDER);
	strcat(PathStr,"/");
	IntToStrMP3Name(PathStr + strlen(PathStr), gMediaPlayer->RecFileList[Index]);
	APP_DBG("%s", PathStr);

	ret = f_open(filehandle, PathStr, FA_READ);
	return ret;
}
#endif
#ifdef CFG_FUNC_RECORD_FLASHFS
uint32_t FlashFsReadFile(void *buffer, uint32_t length) // 作为flashfs 回放callback
{
	uint8_t ret = 0;
	if(length == 0)
	{
		return 0;
	}
    if(gMediaPlayer->FlashFsFile)
    {
		 ret = Fread(buffer, length, 1, gMediaPlayer->FlashFsFile);
    }
    if(ret)
    {
		//APP_DBG("R:%d\n",length);
    	return length;
    }
    else
    {
    	DecoderMuteAndStop();
    	APP_DBG("Read fail\n");
    	return 0;
    }
}

#endif

#ifdef DEL_REC_FILE_EN
void DelRecFile(void)// bkd add
{
#if defined(CFG_FUNC_RECORD_SD_UDISK)
	char FilePath[FILE_PATH_LEN];
	uint32_t i_count = 0;

	strcpy(FilePath, current_vol);
	strcat(FilePath, CFG_PARA_RECORDS_FOLDER);
	strcat(FilePath,"/");
	IntToStrMP3Name(FilePath + strlen(FilePath), gMediaPlayer->RecFileList[gMediaPlayer->CurFileIndex - 1]);
	f_unlink(FilePath);

	if(gMediaPlayer->CurFileIndex == gMediaPlayer->TotalFileSumInDisk)
	{
		gMediaPlayer->CurFileIndex=1;
	}
	else
	{

	for(i_count = gMediaPlayer->CurFileIndex-1; i_count <= (gMediaPlayer->TotalFileSumInDisk - 2); i_count++)
		gMediaPlayer->RecFileList[i_count] = gMediaPlayer->RecFileList[i_count + 1];

	}

	gMediaPlayer->TotalFileSumInDisk--;

	APP_DBG("Del %s.mp3\n",FilePath);
#elif defined(CFG_FUNC_RECORD_FLASHFS)
	Remove(CFG_PARA_FLASHFS_FILE_NAME);
#endif
	APP_DBG("MSG_REC_FILE_DEL\n");

}
#endif
#endif
#if defined(FUNC_BROWSER_TREE_EN)||defined(FUNC_SPECIFY_FOLDER_PLAY_EN)
void MediaPlayerBrowserEnter(void)
{
	MediaPlayerStop();
	//SoftFlagRegister(SoftFlagDecoderSwitch);
	//vTaskDelay(1);
	gMediaPlayer->CurPlayTime = 0;
	gMediaPlayer->SongSwitchFlag = 0;
	SetMediaPlayerState(PLAYER_STATE_PLAYING);

	MediaPlayerRepeatABClear();
	SoftFlagRegister(SoftFlagMediaNextOrPrev);
	MediaPlayerSong();
}

void SetMediaPlayMode(uint8_t playmode)
{
	gMediaPlayer->CurPlayMode = playmode;//PLAY_MODE_BROWSER;
}

#ifdef FUNC_SPECIFY_FOLDER_PLAY_EN
void MediaPlayerStoryEnter(void)
{
	MediaPlayerStop();
	gMediaPlayer->CurPlayTime = 0;
	gMediaPlayer->SongSwitchFlag = 0;
	SetMediaPlayerState(PLAYER_STATE_PLAYING);
	gMediaPlayer->StorySelectPlayFlag=STORY_PLAY_CURRENT;
}
#endif

#endif

#ifdef FUNC_BROWSER_PARALLEL_EN
void SetMediaPlayMode(uint8_t playmode)
{
	gMediaPlayer->CurPlayMode = playmode;//PLAY_MODE_BROWSER;
}

uint8_t GetMediaPlayMode(void)
{
	return gMediaPlayer->CurPlayMode;//PLAY_MODE_BROWSER;
}
void MediaPlayerBrowserEnter(void)
{
	if(GetBrowserPlay_state() == Browser_Play_Normal
		&& (gMediaPlayer->CurFileIndex == gBrowserDirWin[gFolderFocusing - 1].first_file_index + (gStartFileIndex+gFileFocusing - 1) - 1))
	{
		MediaPlayerPlayPause();
	}
	else
	{
		MediaPlayerStop();
		//SoftFlagRegister(SoftFlagDecoderSwitch);
		//vTaskDelay(1);
		gMediaPlayer->CurPlayTime = 0;
		gMediaPlayer->SongSwitchFlag = 0;
		SetMediaPlayerState(PLAYER_STATE_PLAYING);
		APP_DBG("browser song refresh\n");
		MediaPlayerSongBrowserRefresh();
	}
}

void MediaPlayerSongBrowserRefresh(void)
{

//	SoftFlagDeregister(SoftFlagDecoderMask & ~SoftFlagDecoderApp);//清理非App对解码器的登记
	MediaPlayerCloseSongFile();//存在重复关闭情况，需兼容。

	/*
		memset(&gMediaPlayer->PlayerFile, 0x00, sizeof(FIL));
		memset(&gMediaPlayer->PlayerFolder, 0x00, sizeof(ff_dir));
		memcpy(&gMediaPlayer->PlayerFolder, &gBrowserDirWin[gFolderFocusing- 1], sizeof(ff_dir));
		memcpy(&gMediaPlayer->PlayerFile, &gBrowserFileWin[gFileFocusing - 1], sizeof(FIL));
		gMediaPlayer->CurFileIndex = gBrowserDirWin[gFolderFocusing - 1].first_file_index + (gStartFileIndex+gFileFocusing - 1) - 1;
		gMediaPlayer->PlayerFolder.FirstFileIndex = gBrowserDirWin[gFolderFocusing - 1].first_file_index;
		gMediaPlayer->PlayerFolder.FileNumLen = gBrowserDirWin[gFolderFocusing - 1].valid_file_num;
		APP_DBG("file_index_in_disk=%d file_index_in_folder=%ld\n", gMediaPlayer->CurFileIndex, (gStartFileIndex+gFileFocusing - 1));
		SongFileType = get_audio_type((TCHAR *)gMediaPlayer->PlayerFile.fn);
	*/


		gMediaPlayer->CurFileIndex = gBrowserDirWin[gFolderFocusing - 1].first_file_index + (gStartFileIndex+gFileFocusing - 1) - 1;

		if(!MediaPlayerOpenSongFile())
		{
			return FALSE;//文件错误
		}

	//解码初始化失败(无效文件)，保持依据历史操作，切换上下首
	if(!MediaPlayerDecoderRefresh(gMediaPlayer->CurPlayTime))
	{
		APP_DBG("Refresh fail\n");
		{
			if(gMediaPlayer->ErrFileCount )
			{
				gMediaPlayer->ErrFileCount --;
				if(!gMediaPlayer->ErrFileCount )
				{
					MessageContext		msgSend;

					msgSend.msgId		= MSG_MEDIA_PLAY_BROWER_RETURN;
					MessageSend(GetSysModeMsgHandle(), &msgSend);
					APP_DBG("Can't play ,please select again");
					vTaskDelay(5);
					return;
				}
			}

		}
		if(gMediaPlayer->SongSwitchFlag == TRUE)
		{
			MessageContext		msgSend;

			msgSend.msgId		= MSG_PRE;
			MessageSend(GetSysModeMsgHandle(), &msgSend);

		}
		else
		{
			MessageContext		msgSend;

			msgSend.msgId		= MSG_NEXT;
			MessageSend(GetSysModeMsgHandle(), &msgSend);
		}
		vTaskDelay(5);

		return ;
	}

	SetMediaPlayerState(PLAYER_STATE_PLAYING);
	DecoderPlay(DECODER_MODE_CHANNEL);

	//此处是否判断快进超过歌曲长度，改为下一首，当前是解码器自动。
#ifdef CFG_FUNC_BREAKPOINT_EN
	if(GetSystemMode() == ModeCardAudioPlay || GetSystemMode() == ModeUDiskAudioPlay)
	{
		BackupInfoUpdata(BACKUP_SYS_INFO);
		BackupInfoUpdata(BACKUP_PLAYER_INFO);
#ifdef BP_PART_SAVE_TO_NVM
		BackupInfoUpdata(BACKUP_PLAYER_INFO_2NVM);
#endif
	}
#endif
}
#endif
#endif


