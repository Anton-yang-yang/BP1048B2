/**
 *******************************************************************************
 * @file    bt_avrcp_app.c
 * @author  Halley
 * @version V1.0.1
 * @date    27-Apr-2016
 * @brief   Avrcp callback events and actions
 *******************************************************************************
 * @attention
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, MVSILICON SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */

#include "type.h"
#include "debug.h"
#include "bt_manager.h"
#include "timeout.h"
#ifdef	FUNC_OS_EN
#include "rtos_api.h"
#endif
#include "bt_interface.h"
#ifdef CFG_APP_CONFIG
#include "bt_play_api.h"
#include "app_config.h"
#endif
#include "bt_config.h"
#include "string_convert.h"
#include "bt_app_avrcp_deal.h"


uint32_t AvrcpStateSuspendCount = 0;

uint32_t ChangePlayerStateGet(void)
{
	if( AvrcpStateSuspendCount && ( (btManager.cur_index == 0 && GetA2dpState(1) == BT_A2DP_STATE_STREAMING) || (btManager.cur_index == 1 && GetA2dpState(0) == BT_A2DP_STATE_STREAMING) ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*****************************************************************************************
* avrcp callback
****************************************************************************************/
void BtAvrcpCallback(BT_AVRCP_CALLBACK_EVENT event, BT_AVRCP_CALLBACK_PARAMS * param)
{
	switch(event)
	{

		case BT_STACK_EVENT_AVRCP_CONNECTED:
		{
			BtAvrcpConnectedDev(param);
			break;
		}

		case BT_STACK_EVENT_AVRCP_DISCONNECTED:
		{
			BtAvrcpDisconnectedDev(param);
			break;
		}
		case BT_STACK_EVENT_AVRCP_ADV_PLAY_STATUS_CHANGED:
		{
			BtAvrcpPlayStatusChanged(param);
			break;
		}
		case BT_STACK_EVENT_AVRCP_ADV_PLAY_STATUS:
#if (BT_AVRCP_SONG_PLAY_STATE == ENABLE)
			BtAvrcpPlayStatus(param);
#endif
			break;

		case BT_STACK_EVENT_AVRCP_ADV_TRACK_INFO:
			//APP_DBG("TRACK_INFO\n");
			/*{
				APP_DBG("Playing > %02d:%02d / %02d:%02d \n",
            		(param->params.avrcpAdv.avrcpAdvTrack.ms/1000)/60,
            		(param->params.avrcpAdv.avrcpAdvTrack.ms/1000)%60,
            		(param->params.avrcpAdv.avrcpAdvTrack.ls/1000)/60,
            		(param->params.avrcpAdv.avrcpAdvTrack.ls/1000)%60);
			}*/
			break;

		case BT_STACK_EVENT_AVRCP_ADV_ADDRESSED_PLAYERS:
			//APP_DBG("AVRCP Event: BT_STACK_EVENT_AVRCP_ADV_ADDRESSED_PLAYERS\n");
			//APP_DBG("PlayId[%x],UidCount[%x]\n", param->params.avrcpAdv.avrcpAdvAddressedPlayer.playerId, param->params.avrcpAdv.avrcpAdvAddressedPlayer.uidCounter);
			break;

		case BT_STACK_EVENT_AVRCP_ADV_CAPABILITY_COMPANY_ID:

			break;
			
		case BT_STACK_EVENT_AVRCP_ADV_CAPABILITY_EVENTS_SUPPORTED:
			//APP_DBG("AVRCP SUPPORTED FEATRUE = [0x%04x]\n", param->params.avrcpAdv.avrcpAdvEventMask);
			break;

		case BT_STACK_EVENT_AVRCP_ADV_GET_PLAYER_SETTING_VALUE:
#if (BT_AVRCP_PLAYER_SETTING == ENABLE)
			APP_DBG("attrId:%x, ", param->params.avrcpAdv.avrcpAdvPlayerettingValue.attrMask);
			if(param->params.avrcpAdv.avrcpAdvPlayerettingValue.attrMask == AVRCP_ENABLE_PLAYER_EQ_STATUS)
				APP_DBG("EQ:%x\n", param->params.avrcpAdv.avrcpAdvPlayerettingValue.eq);
			else if(param->params.avrcpAdv.avrcpAdvPlayerettingValue.attrMask == AVRCP_ENABLE_PLAYER_REPEAT_STATUS)
				APP_DBG("repeat:%x\n", param->params.avrcpAdv.avrcpAdvPlayerettingValue.repeat);
			else if(param->params.avrcpAdv.avrcpAdvPlayerettingValue.attrMask == AVRCP_ENABLE_PLAYER_SHUFFLE_STATUS)
				APP_DBG("shuffle:%x\n", param->params.avrcpAdv.avrcpAdvPlayerettingValue.shuffle);
			else if(param->params.avrcpAdv.avrcpAdvPlayerettingValue.attrMask == AVRCP_ENABLE_PLAYER_SCAN_STATUS)
				APP_DBG("scan:%x\n", param->params.avrcpAdv.avrcpAdvPlayerettingValue.scan);
#endif
			break;

#if (BT_AVRCP_VOLUME_SYNC == ENABLE)
		case BT_STACK_EVENT_AVRCP_ADV_VOLUME_CHANGE:
			//APP_DBG("BTVOL_CHANGE = [%d]\n", param->params.avrcpAdv.avrcpAdvVolumePercent);
#if (BT_LINK_DEV_NUM == 2)
			if(BtCurIndex_Get() == param->index)
#endif
				BtAvrcpVolumeChanged(param);
			break;
#endif

#if (BT_AVRCP_SONG_TRACK_INFOR == ENABLE)
		case BT_STACK_EVENT_AVRCP_ADV_MEDIA_INFO:
			APP_DBG("AVRCP Event: BT_STACK_EVENT_AVRCP_ADV_MEDIA_INFO\n");
			if(GetMediaInfo)
				GetMediaInfo((void*)param->params.avrcpAdv.avrcpAdvMediaInfo);
			break;
#endif

#if BT_AVRCP_VOLUME_SYNC
			case BT_STACK_EVENT_AVRCP_PANEL_RELEASE:
				BtMidMessageSend(MSG_BT_MID_AVRCP_PANEL_KEY, (uint8_t)param->params.panelKey);
			break;
#endif		

///////////////////////////////////////////////////////////////////////////////////////////
		default:
			break;
	}
}
void ChangePlayer(void)
{
	/*if((PreChangeFlag == TRUE) && IsTimeOut(&tChangePlayer))
	{
		BtCurIndex_Set(btManager.back_index);
		BT_DBG("channel[%d]:start playing\n", btManager.cur_index);
		//if(RefreshSbcDecoder)
		{
			//RefreshSbcDecoder();
			BtMidMessageSend(MSG_BT_MID_PLAY_STATE_CHANGE, AVRCP_ADV_MEDIA_PLAYING);
		}
		PreChangeFlag = FALSE;
	}*/
#if (BT_LINK_DEV_NUM == 2)
	if(AvrcpStateSuspendCount)
	{
		AvrcpStateSuspendCount++;
		if(AvrcpStateSuspendCount >= 1000)
		{
			uint8_t switch_index = (btManager.cur_index == 0) ? 1 : 0;

			if(GetA2dpState(switch_index) == BT_A2DP_STATE_STREAMING)
			{
#ifdef LAST_PLAY_PRIORITY
				APP_DBG("ChangePlayer btManager.cur_index Pause %d %d\n",btManager.btLinked_env[btManager.cur_index].avrcp_index,GetA2dpState(btManager.cur_index));
				AvrcpCtrlPause(btManager.btLinked_env[btManager.cur_index].avrcp_index);
				SetA2dpState(btManager.cur_index,BT_A2DP_STATE_CONNECTED);
#endif
				BtCurIndex_Set(switch_index);
				a2dp_sbc_decoer_init();
			}
			AvrcpStateSuspendCount = 0;
		}
	}
#endif

}

void SetAvrcpState(uint8_t index, BT_AVRCP_STATE state)
{
	GetBtManager()->btLinked_env[index].avrcpState = state;
}

BT_AVRCP_STATE GetAvrcpState(uint8_t index)
{
	return GetBtManager()->btLinked_env[index].avrcpState;
}


bool IsAvrcpConnected(uint8_t index)
{
	return (GetAvrcpState(index) == BT_AVRCP_STATE_CONNECTED);
}

void BtAvrcpConnect(uint8_t index, uint8_t *addr)
{
#if (BT_LINK_DEV_NUM == 2)
	uint8_t avrcp_index = GetBtManagerAvrcpIndex(index);

	if( avrcp_index < BT_LINK_DEV_NUM
		&&(GetBtManager()->btLinked_env[avrcp_index].avrcpState > BT_AVRCP_STATE_NONE))
	{
		APP_DBG("BtAvrcpConnect:avrcpState index[%d] is %d\n",avrcp_index,GetBtManager()->btLinked_env[avrcp_index].avrcpState);
		return;
	}
#endif
	//if(!IsAvrcpConnected(index))
	{
		APP_DBG("AvrcpConnect index = %d,addr:%x:%x:%x:%x:%x:%x\n", index,addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
		AvrcpConnect(index, addr);
	}
}

//针对部分手机不能主动连接AVRCP协议,在A2DP连接成功2S后主动发起AVRCP连接
/*void BtAvrcpConnectStart(void)
{
	if((!btManager.avrcpConnectStart)&&(!IsAvrcpConnected()))
	{
		btManager.avrcpConnectStart = 1;
		TimeOutSet(&AvrcpConnectTimer, 2000);
	}
}
*/
