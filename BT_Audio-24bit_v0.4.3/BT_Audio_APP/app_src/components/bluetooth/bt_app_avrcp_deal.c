/**
 **************************************************************************************
 * @file    bluetooth_avrcp_deal.c
 * @brief   
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2021-4-18 18:00:00$
 *
 * @Copyright (C) Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include "debug.h"
#include "app_config.h"
#include "bt_config.h"
#include "bt_app_avrcp_deal.h"
#include "bt_manager.h"
#include "bt_interface.h"
#include "audio_vol.h"
#include "bt_app_common.h"
#include "app_message.h"
#include "bt_stack_service.h"
#include "mode_task.h"

#if (BT_AVRCP_SONG_TRACK_INFOR == ENABLE)
#define StringMaxLen 60
#include "string_convert.h"
#endif

extern uint32_t AvrcpStateSuspendCount;
extern uint32_t gSpecificDevice;
#if (BT_LINK_DEV_NUM == 2)
static AvrcpAdvMediaStatus sPlayStatus[BT_LINK_DEV_NUM]={AVRCP_ADV_MEDIA_STOPPED,AVRCP_ADV_MEDIA_STOPPED};
static AvrcpAdvMediaStatus sPlayStatusBk[BT_LINK_DEV_NUM]={AVRCP_ADV_MEDIA_STOPPED,AVRCP_ADV_MEDIA_STOPPED};
#else //(BT_LINK_DEV_NUM == 1)
static AvrcpAdvMediaStatus sPlayStatus[BT_LINK_DEV_NUM]={AVRCP_ADV_MEDIA_STOPPED};
static AvrcpAdvMediaStatus sPlayStatusBk[BT_LINK_DEV_NUM]={AVRCP_ADV_MEDIA_STOPPED};
#endif
/*****************************************************************************************
* AVRCP连接成功
****************************************************************************************/
void BtAvrcpConnectedDev(BT_AVRCP_CALLBACK_PARAMS * param)
{
	uint8_t ConnectIndex = 0;
	APP_DBG("param->index:%d, Avrcp Connected : bt address = %02x:%02x:%02x:%02x:%02x:%02x\n", param->index,
			(param->params.bd_addr)[0],
			(param->params.bd_addr)[1],
			(param->params.bd_addr)[2],
			(param->params.bd_addr)[3],
			(param->params.bd_addr)[4],
			(param->params.bd_addr)[5]);
	
#if (BT_LINK_DEV_NUM == 2)
	for(ConnectIndex = 0; ConnectIndex < BT_LINK_DEV_NUM; ConnectIndex++)
	{
		if(memcmp(param->params.bd_addr,btManager.btLinked_env[ConnectIndex].remoteAddr,BT_ADDR_SIZE) == 0)
		{
			break;
		}
	}
	if(ConnectIndex >= BT_LINK_DEV_NUM)
	{
		for(ConnectIndex = 0; ConnectIndex < BT_LINK_DEV_NUM; ConnectIndex++)
		{
			if(!(btManager.btLinked_env[ConnectIndex].btLinkedProfile & BT_CONNECTED_A2DP_FLAG)
					&& !(btManager.btLinked_env[ConnectIndex].btLinkedProfile & BT_CONNECTED_AVRCP_FLAG)
			#if(BT_HFP_SUPPORT == ENABLE)
					&& !(btManager.btLinked_env[ConnectIndex].btLinkedProfile & BT_CONNECTED_HFP_FLAG)
			#endif
				)
			{
				break;
			}
		}
	}
			
	if(ConnectIndex >= BT_LINK_DEV_NUM)
	{
		APP_DBG("AVRCP link full\n");
		BTHciDisconnectCmd(param->params.bd_addr);//连接出现异常直接断开蓝牙
		if(btManager.btLinked_env[0].btLinkState)
		{
			btManager.btLinked_env[1].avrcpState = BT_AVRCP_STATE_NONE;
			btManager.btLinked_env[1].a2dpState = BT_A2DP_STATE_NONE;
			btManager.btLinked_env[1].hfpState = BT_HFP_STATE_NONE;
			btManager.btLinked_env[1].btLinkState = 0;
			APP_DBG("BtLink[1] DisConnect\n");
		}
		else
		{
			btManager.btLinked_env[0].avrcpState = BT_AVRCP_STATE_NONE;
			btManager.btLinked_env[0].a2dpState = BT_A2DP_STATE_NONE;
			btManager.btLinked_env[0].hfpState = BT_HFP_STATE_NONE;
			btManager.btLinked_env[1].btLinkState = 1;
			APP_DBG("BtLink[0] DisConnect\n");
		}
		return;
	}
#endif

	btManager.btLinked_env[ConnectIndex].avrcp_index = param->index;
	SetAvrcpState(ConnectIndex, BT_AVRCP_STATE_CONNECTED);

	if((param->params.bd_addr)[0] || (param->params.bd_addr)[1] || (param->params.bd_addr)[2]
		|| (param->params.bd_addr)[3] || (param->params.bd_addr)[4] || (param->params.bd_addr)[5])
	{
		memcpy(GetBtManager()->remoteAddr, param->params.bd_addr, 6);
		memcpy(GetBtManager()->btLinked_env[ConnectIndex].remoteAddr, param->params.bd_addr, 6);
	}


	SetBtConnectedProfile(ConnectIndex, BT_CONNECTED_AVRCP_FLAG);
	BtLinkStateConnect(0, ConnectIndex);

#ifdef BT_TWS_SUPPORT				
	if((btManager.twsState == BT_TWS_STATE_CONNECTED)&&(btManager.twsRole == BT_TWS_SLAVE))
	{
		//从机已经组网成功,此时被手机连接上,则断开手机
		BTHciDisconnectCmd(param->params.bd_addr);
		return;
	}
#endif

#if (BT_AUTO_PLAY_MUSIC == ENABLE)
	BtAutoPlayMusic();
#endif

#if (BT_AVRCP_VOLUME_SYNC == ENABLE)
	btManager.avrcpSyncEnable = 0;
	btManager.avrcpSyncVol = AudioMusicVolGet(); //初始值
	AvrcpAdvTargetSyncVolumeConfig(ConnectIndex,BtLocalVolLevel2AbsVolme(AudioMusicVolGet()));//蓝牙AVRCP连接成功后，将当前的音量值同步到AVRCP adv.volume默认值
#endif

	BtStackServiceMsgSend(MSG_BTSTACK_RECONNECT_REMOTE_PROFILE);
	
	BtEventFlagDeregister(BT_EVENT_FLAG_AVRCP_CONNECT);

	if(btManager.linkedNumber == 0)
	{
		//btManager.cur_index = param->index;
		BtCurIndex_Set(ConnectIndex);
	}
}

/*****************************************************************************************
* AVRCP断开连接
****************************************************************************************/
void BtAvrcpDisconnectedDev(BT_AVRCP_CALLBACK_PARAMS * param)
{
	uint8_t index = GetBtManagerAvrcpIndex(param->index);
	APP_DBG("Avrcp disconnect,index %d\n",param->index);
	//SetAvrcpState(param->index, BT_AVRCP_STATE_NONE);

	if(index < BT_LINK_DEV_NUM)
	{
	//if((param->params.bd_addr!=NULL)&&(memcmp(param->params.bd_addr,btManager.btLinked_env[i].remoteAddr,BT_ADDR_SIZE) == 0))
		SetAvrcpState(index, BT_A2DP_STATE_NONE);
		SetBtDisconnectProfile(index,BT_CONNECTED_AVRCP_FLAG);
		BtLinkStateDisconnect(index);
	}
	#ifdef RECON_ADD
	btManager.btReconnectunusual = TRUE;
	#endif
	//AVRCP断开机制清除
	//BtEventFlagDeregister(BT_EVENT_FLAG_AVRCP_DISCONNECT);

	//AVRCP断开后,主动断开A2DP,会导致误触发行为,先关闭
	//BtEventFlagRegister(BT_EVENT_FLAG_A2DP_DISCONNECT, 1000);
}

/*****************************************************************************************
* AVRCP 播放状态改变处理流程
****************************************************************************************/
void BtAvrcpPlayStatusChanged(BT_AVRCP_CALLBACK_PARAMS * param)
{
//	extern uint32_t a2dp_avrcp_connect_flag;
	uint8_t index = GetBtManagerAvrcpIndex(param->index);
	APP_DBG("Remote AVRCP Play State [%d],index = %d\n", param->params.avrcpAdv.avrcpAdvMediaStatus,param->index);

	if(GetSystemMode() != ModeBtAudioPlay)
		return;

	if(index >= BT_LINK_DEV_NUM)
		return;

#if 0//(BT_HFP_SUPPORT == ENABLE)
	if((param->params.avrcpAdv.avrcpAdvMediaStatus == 1)&&(gSpecificDevice))
	{
		extern void SpecialDeviceFunc(uint8_t index);
		SpecialDeviceFunc(param->index);
	}
#endif

	sPlayStatus[index] = param->params.avrcpAdv.avrcpAdvMediaStatus;

	if(btManager.cur_index == index)
	{
		BtMidMessageSend(MSG_BT_MID_PLAY_STATE_CHANGE, param->params.avrcpAdv.avrcpAdvMediaStatus);

		if((sPlayStatusBk[index] == AVRCP_ADV_MEDIA_PLAYING)&&(sPlayStatus[index] != AVRCP_ADV_MEDIA_PLAYING))
		{
			AvrcpStateSuspendCount++;
		}
		else if(sPlayStatus[index] == AVRCP_ADV_MEDIA_PLAYING)
		{
			AvrcpStateSuspendCount = 0;
		}
	}
#ifdef LAST_PLAY_PRIORITY
	else if(param->params.avrcpAdv.avrcpAdvMediaStatus == AVRCP_ADV_MEDIA_PLAYING)
	{
		//AvrcpCtrlPause(btManager.btLinked_env[btManager.cur_index].avrcp_index);
		//BtCurIndex_Set(i);
		SetA2dpState(index, BT_A2DP_STATE_STREAMING);//防止抢占失败
		AvrcpStateSuspendCount++;
	}
	else if(param->params.avrcpAdv.avrcpAdvMediaStatus == AVRCP_ADV_MEDIA_PAUSED)
	{
		AvrcpStateSuspendCount = 0;
	}
#endif
	sPlayStatusBk[index] = sPlayStatus[index];


#if (BT_AUTO_PLAY_MUSIC == ENABLE)
	void BtAutoPlaySetAvrcpPlayStatus(uint8_t play_state);
	BtAutoPlaySetAvrcpPlayStatus(param->params.avrcpAdv.avrcpAdvMediaStatus);
#endif

	
#if (BT_HFP_SUPPORT == ENABLE)
	if((param->params.avrcpAdv.avrcpAdvMediaStatus == 1)&&(gSpecificDevice))
	{
		extern void SpecialDeviceFunc(void);
		SpecialDeviceFunc();
	}
#endif
	//bkd del for press key to play,but STREAMING command not received 
	{
		if(btManager.cur_index == index)//防止播放暂停状态错乱
			BtMidMessageSend(MSG_BT_MID_PLAY_STATE_CHANGE, param->params.avrcpAdv.avrcpAdvMediaStatus);
		
		if(CheckTimerStart_BtPlayStatus() == 0)
		{
			if(param->params.avrcpAdv.avrcpAdvMediaStatus == AVRCP_ADV_MEDIA_PLAYING)
			{
				BTCtrlGetPlayStatus(param->index);
				TimerStart_BtPlayStatus();
			}
		}
	}
}

/*****************************************************************************************
* AVRCP 播放状态反馈
****************************************************************************************/
void BtAvrcpPlayStatus(BT_AVRCP_CALLBACK_PARAMS * param)
{
	uint8_t curPlayStatus = param->params.avrcpAdv.avrcpAdvPlayStatus.CurPlayStatus;
	uint32_t curPlayTimes = param->params.avrcpAdv.avrcpAdvPlayStatus.CurPosInMs;
	uint32_t tatolPlayTimes = param->params.avrcpAdv.avrcpAdvPlayStatus.TotalLengthInMs;
	
	switch(curPlayStatus)
	{
		case AVRCP_ADV_MEDIA_STOPPED:
			APP_DBG("Stopped \n");
			break;
			
		case AVRCP_ADV_MEDIA_PLAYING:
			APP_DBG("Playing > %02d:%02d / %02d:%02d \n",
					((int)curPlayTimes/1000)/60,
					((int)curPlayTimes/1000)%60,
					((int)tatolPlayTimes/1000)/60,
					((int)tatolPlayTimes/1000)%60);

			if((curPlayTimes/1000%5) == 1)
				BTCtrlGetMediaInfor(param->index);
			
			break;

		case AVRCP_ADV_MEDIA_PAUSED:
			APP_DBG("Paused || %02d:%02d / %02d:%02d  \n",
					((int)curPlayTimes/1000)/60,
					((int)curPlayTimes/1000)%60,
					((int)tatolPlayTimes/1000)/60,
					((int)tatolPlayTimes/1000)%60);
			break;

		case AVRCP_ADV_MEDIA_FWD_SEEK:
			APP_DBG("FF >> %02d:%02d / %02d:%02d  \n",
					((int)curPlayTimes/1000)/60,
					((int)curPlayTimes/1000)%60,
					((int)tatolPlayTimes/1000)/60,
					((int)tatolPlayTimes/1000)%60);
			break;

		case AVRCP_ADV_MEDIA_REV_SEEK:
			APP_DBG("FB << %02d:%02d / %02d:%02d  \n",
					((int)curPlayTimes/1000)/60,
					((int)curPlayTimes/1000)%60,
					((int)tatolPlayTimes/1000)/60,
					((int)tatolPlayTimes/1000)%60);
			break;

		default:
			break;
	}
}

/*****************************************************************************************
* 定时主动获取播放时间
****************************************************************************************/
void TimerStart_BtPlayStatus(void)
{
	TimeOutSet(&btManager.avrcpPlayStatusTimer.timerHandle, 900);
	btManager.avrcpPlayStatusTimer.timerFlag = 1;
}

void TimerStop_BtPlayStatus(void)
{
	btManager.avrcpPlayStatusTimer.timerFlag = 0;
}

uint8_t CheckTimerStart_BtPlayStatus(void)
{
	return btManager.avrcpPlayStatusTimer.timerFlag;
}

#if (BT_AVRCP_VOLUME_SYNC == ENABLE)
/*****************************************************************************************
* AVRCP 同步音量改变
****************************************************************************************/
void BtAvrcpVolumeChanged(BT_AVRCP_CALLBACK_PARAMS * param)
{
	uint16_t VolumePercent = 0;
	btManager.avrcpSyncEnable = 1;
	VolumePercent = BtAbsVolume2VolLevel(param->params.avrcpAdv.avrcpAdvVolumePercent);
	APP_DBG("BTVOL_CHANGE = [%d]\n", VolumePercent);
	
	BtMidMessageSend(MSG_BT_MID_VOLUME_CHANGE, (uint8_t)VolumePercent);
}

/*****************************************************************************************
* 设置/获取 音量同步值
****************************************************************************************/
void SetBtSyncVolume(uint8_t volume)
{
	btManager.avrcpSyncVol = volume;
}

uint8_t GetBtSyncVolume(void)
{
	return btManager.avrcpSyncVol;
}
#endif

/***********************************************************************************
 * 获取歌曲信息函数
 **********************************************************************************/
#if (BT_AVRCP_SONG_TRACK_INFOR == ENABLE)
void GetBtMediaInfo(void *params)
{
	AvrcpAdvMediaInfo	*CurMediaInfo;
	uint8_t i;
	uint8_t StringData[StringMaxLen];
	uint8_t ConvertStringData[StringMaxLen];
	CurMediaInfo = (AvrcpAdvMediaInfo*)params;

	if((CurMediaInfo)&&(CurMediaInfo->numIds))
	{
		for(i=0;i<CurMediaInfo->numIds;i++)
		{
			memset(StringData, 0, StringMaxLen);
			memset(ConvertStringData, 0, StringMaxLen);
			
			if(CurMediaInfo->property[i].length)
			{
				if(CurMediaInfo->property[i].charSet == 0x006a)
				{
					//APP_DBG("Character Set Id: UTF-8\n");
					
					if(CurMediaInfo->property[i].length > StringMaxLen)
					{
						memcpy(StringData, CurMediaInfo->property[i].string, StringMaxLen);
					#ifdef CFG_FUNC_STRING_CONVERT_EN
						StringConvert(ConvertStringData, 60, StringData, StringMaxLen ,UTF8_TO_GBK);
					#endif
					}
					else
					{
						memcpy(StringData, CurMediaInfo->property[i].string, CurMediaInfo->property[i].length);
					#ifdef CFG_FUNC_STRING_CONVERT_EN
						StringConvert(ConvertStringData, 60, StringData, CurMediaInfo->property[i].length ,UTF8_TO_GBK);
					#endif
					}

			//Attribute ID
					if(ConvertStringData[0])// no character ,not dispaly ID3
					{
						switch(CurMediaInfo->property[i].attrId)
						{
							case 1:
								APP_DBG("Title of the media\n");
								break;
			
							case 2:
								APP_DBG("Name of the artist\n");
								break;
			
							case 3:
								APP_DBG("Name of the Album\n");
								break;
			
							//当前曲目数:只有在自带播放器才能获取到
							case 4:
								APP_DBG("Number of the media\n");
								break;
			
							//总共曲目数:只有在自带播放器才能获取到
							case 5:
								APP_DBG("Totle number of the media\n");
								break;
			
							case 6:
								APP_DBG("Genre\n");
								break;
			
							case 7:
								APP_DBG("Playing time in millisecond\n");
								break;
							
							case 8:
								APP_DBG("Default cover art\n");
								break;
			
							default:
								break;
						}
					}

					#ifdef CFG_FUNC_STRING_CONVERT_EN
					APP_DBG("%s\n", ConvertStringData);
					#endif
					
					}
				else
				{
					;//APP_DBG("Other Character Set Id: 0x%x\n", CurMediaInfo->property[i].charSet);
				}
			}
		}
	}
}
#endif

