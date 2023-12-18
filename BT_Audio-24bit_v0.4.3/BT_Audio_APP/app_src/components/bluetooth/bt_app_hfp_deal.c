/**
 **************************************************************************************
 * @file    bluetooth_hfp_deal.c
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
#include "bt_manager.h"
#include "bt_app_hfp_deal.h"
#include "bt_interface.h"
#include "mode_task.h"
#include "bt_app_common.h"
#include "bt_hf_mode.h"
#include "bt_stack_service.h"
#include "bt_interface.h"
#include "bt_app_connect.h"

#if (BT_HFP_SUPPORT == ENABLE)
#ifdef BT_HFP_MODE_DISABLE
uint32_t gHfpCallNoneWaiting;
#endif
#ifdef CFG_FUNC_POWER_MONITOR_EN
#include "power_monitor.h"
#ifdef BT_HFP_BATTERY_SYNC
static PWR_LEVEL powerLevelBak = PWR_LEVEL_0;
#endif
#endif

/*****************************************************************************************
* HFP get current call state
* 在手机端有2个电话正在通话时,获取通话状态,能连续的进行反馈
* 通过通话状态的更新,能解决三方通话状态切换的问题
****************************************************************************************/
static uint32_t Caller1Count = 0;
static uint32_t Caller2Count = 0;
static uint32_t gBtHfp3WayCallsCnt = 0;
static uint16_t testRecvLen = 0;

extern uint32_t gSpecificDevice;
#ifdef CFG_FUNC_POWER_MONITOR_EN
void SetBtHfpBatteryLevel(PWR_LEVEL level, uint8_t flag);
#endif
extern uint32_t Get_Change_To_SCO_ID();
extern void Set_Current_SCO_ID(uint32_t ID);

/**************************************************************************************
 * 准备进入通话模式
 * 更新相关的变量
 *************************************************************************************/
void EnterBtHfModeReady(uint8_t index)
{
	printf("EnterBtHfModeReady\n");
#if (BT_LINK_DEV_NUM == 2)
	uint8_t otherIndex;
	otherIndex = (index == 0)? 1 : 0;
	//暂停另外一组index的音乐播放
	if(GetA2dpState(otherIndex)==BT_A2DP_STATE_STREAMING)
		AvrcpCtrlPause(otherIndex);
#endif

	//此操作涉及到活跃链路的切换
	btManager.cur_index = btManager.HfpCurIndex;
	extern void BtHfModeEnter_Index(uint8_t index);
	BtHfModeEnter_Index(index);
}

/*****************************************************************************************
* hfp 3way calling 
****************************************************************************************/
//挂断当前通话,接听另一个电话
void BtHfp_Hangup_Answer_Call(void)
{
	BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_HF_CALL_HANGUP_ANSWER_CALL);
}

//挂断 另外通话, 保持当前通话
void BtHfp_Hangup_Another_Call(void)
{
	BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_HF_CALL_HANGUP_ANOTHER_CALL);
}

//挂起当前通话, 接听另一个电话
void BtHfp_HoldCur_Answer_Call(void)
{
	BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_HF_CALL_HOLDCUR_ANSWER_CALL);
}

/*****************************************************************************************
* hfp profile connect state
****************************************************************************************/
void BtHfpConnectedDev(BT_HFP_CALLBACK_PARAMS * param)
{
	uint8_t ConnectIndex = 0;
	APP_DBG("index:%d, Hfp Connected : [%02x:%02x:%02x:%02x:%02x:%02x]\n", param->index,
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
	if(ConnectIndex >=BT_LINK_DEV_NUM)
	{
		APP_DBG("hfp link full, disconnect hfp.\n");
		BTHciDisconnectCmd(param->params.bd_addr);//连接出现异常直接断开蓝牙
		return;
	}

	if(ConnectIndex == 0)
	{
		if((btManager.btLinkState == 0)&&(BtReconnectDevIsExcute()))
		{
			btManager.cur_index = param->index;
		}
	}
#endif

	btManager.btLinked_env[ConnectIndex].hf_index = param->index;
	if(GetHfpState(ConnectIndex) > BT_HFP_STATE_CONNECTED)
	{
		#if !defined(BT_HFP_MODE_DISABLE) && defined(CFG_APP_CONFIG)
		EnterBtHfModeReady(ConnectIndex);
		#endif
	}
	else
	{
		if(btManager.hfp_CallFalg)
		{
			btManager.hfp_CallFalg = 0;
			SetHfpState(ConnectIndex, BT_HFP_STATE_ACTIVE);
		}
		else
		{
			SetHfpState(ConnectIndex, BT_HFP_STATE_CONNECTED);
		}
	}

	if((param->params.bd_addr)[0] || (param->params.bd_addr)[1] || (param->params.bd_addr)[2]
		|| (param->params.bd_addr)[3] || (param->params.bd_addr)[4] || (param->params.bd_addr)[5])
	{
		memcpy(GetBtManager()->remoteAddr, param->params.bd_addr, 6);
		memcpy(GetBtManager()->btLinked_env[ConnectIndex].remoteAddr, param->params.bd_addr, 6);
	}

	SetBtConnectedProfile(ConnectIndex, BT_CONNECTED_HFP_FLAG);
	
	#if (defined(CFG_FUNC_POWER_MONITOR_EN)&&defined(BT_HFP_BATTERY_SYNC))
	SetBtHfpBatteryLevel(PowerLevelGet(), 1);
	#endif
	
	btManager.hfpVoiceState = 0;

	BtLinkStateConnect(0, ConnectIndex);

	BtMidMessageSend(MSG_BT_MID_HFP_CONNECTED, 0);

	if(BtAddrIsValid(param->params.bd_addr) == 0)
	{
		memcpy(btManager.remoteAddr, param->params.bd_addr, 6);
	}

	BtStackServiceMsgSend(MSG_BTSTACK_RECONNECT_REMOTE_PROFILE);

#ifdef BT_REMOTE_AEC_DISABLE
	BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_HF_DISABLE_NREC);
#endif

	BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_HF_GET_CUR_CALLNUMBER);
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtHfpDisconnectedDev(BT_HFP_CALLBACK_PARAMS * param)
{
	APP_DBG("Hfp disconnect,index %d\n",param->index);

	btManager.appleDeviceFlag = 0;
	
	uint8_t index = GetBtManagerHfpIndex(param->index);

	if(index < BT_LINK_DEV_NUM)
	{
		SetHfpState(index, BT_HFP_STATE_NONE);
		SetBtDisconnectProfile(index,BT_CONNECTED_HFP_FLAG);
		BtLinkStateDisconnect(index);
	}
	
	if((btManager.btLinked_env[0].hfpState <= BT_HFP_STATE_CONNECTED)
		#if (BT_LINK_DEV_NUM == 2)
		&&(btManager.btLinked_env[1].hfpState <= BT_HFP_STATE_CONNECTED)
		#endif
		)
	{
		btManager.HfpCurIndex = 0xff;
		BtHfModeExit();//断开HFP，同步退出通话模式
	}
	
	//回连流程,HFP被拒绝,先发起A2DP/AVRCP连接
	if((param->errorCode == 0x13)&&(BtReconnectDevIsExcute()))//remote terminated disconnect
	{
		uint8_t ConnectIndex = 0;
		for(ConnectIndex = 0; ConnectIndex < BT_LINK_DEV_NUM; ConnectIndex++)
		{
			if(memcmp(btManager.btReconPhoneSt.RemoteDevAddr,btManager.btLinked_env[ConnectIndex].remoteAddr,BT_ADDR_SIZE) == 0)
				break;
		}

		if(ConnectIndex<BT_LINK_DEV_NUM)
		{
			if((GetA2dpState(ConnectIndex) >= BT_A2DP_STATE_CONNECTED)&&(GetAvrcpState(ConnectIndex) >= BT_AVRCP_STATE_CONNECTED))
			{
				BtReconnectDevStop();
				printf("============ hfp cannot connect, stop..\n");
				return;
			}
		}
			
		BtReconProfilePrioRegister(BT_PROFILE_SUPPORTED_A2DP|BT_PROFILE_SUPPORTED_AVRCP);
		BtReconnectDevAgain(1000);
		printf("============ delay 1000ms, reconnect a2dp+avrcp\n");
	}
	#ifdef RECON_ADD
	btManager.btReconnectunusual = TRUE;
	#endif
}

/*****************************************************************************************
* sco link connect state
****************************************************************************************/
void BtHfpScoLinkConnected(BT_HFP_CALLBACK_PARAMS * param)
{
	uint8_t type;
	uint8_t index = GetBtManagerHfpIndex(param->index);

	APP_DBG("Hfp sco connect %d\n", param->index);
	if(index < BT_LINK_DEV_NUM && GetHfpState(index) < BT_HFP_STATE_CONNECTED)
		return;
	
#ifdef BT_HFP_MODE_DISABLE
	btManager.hfpScoDiscIndex = index;
	btManager.hfpScoDiscDelay = 200;	//disconnect sco link delay 200ms
#else
	#if (BT_LINK_DEV_NUM == 2)
	if(btManager.HfpCurIndex == 0xff)
	{
		btManager.HfpCurIndex = param->index;
	}
	else if((btManager.HfpCurIndex != param->index)&&(GetScoConnectFlag() == 0))
	{
		//当前的index非活跃的index,并且活跃index已经切换到私密接听,则能动态切换index
		btManager.HfpCurIndex = param->index;
		printf("HfpCurIndex update -> %d\n", btManager.HfpCurIndex);
		SetHfpState(btManager.HfpCurIndex, BT_HFP_STATE_ACTIVE);
		FirstTalkingPhoneIndexSet(btManager.HfpCurIndex);
	
		//此操作涉及到活跃链路的切换
		btManager.cur_index = btManager.HfpCurIndex;
	}
	else if(btManager.HfpCurIndex != param->index)
	{
		FirstTalkingPhoneIndexSet(btManager.HfpCurIndex);
		SetHfpState(btManager.HfpCurIndex, BT_HFP_STATE_ACTIVE);
		return;
	}
	Set_Current_SCO_ID(0);
	#endif

	SetScoConnectFlag(TRUE);
	DelayExitBtHfModeCancel();
	
	type = GetHfpScoAudioCodecType(index);
	APP_DBG("&&&sco audio type:%d\n", type);
	if(type == 0)//避免通话音频数据类型不同导致异常
	{
		btManager.hfpScoCodecType[btManager.cur_index] = HFP_AUDIO_DATA_PCM;
	}
	else if(type == 1)
	{
		btManager.hfpScoCodecType[btManager.cur_index] = HFP_AUDIO_DATA_mSBC;
	}
	BtMidMessageSend(MSG_BT_MID_HFP_CODEC_TYPE_UPDATE, btManager.hfpScoCodecType[btManager.cur_index]);

#ifdef BT_RECORD_FUNC_ENABLE
	BtMidMessageSend(MSG_BT_MID_HFP_RECORD_MODE_ENTER, 0);
#else
	EnterBtHfModeReady(param->index);
#endif

	BtMidMessageSend(MSG_BT_MID_HFP_TASK_RESUME, 0);
#endif
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtHfpScoLinkDisconnected(BT_HFP_CALLBACK_PARAMS * param)
{
	uint8_t index = GetBtManagerHfpIndex(param->index);

#ifndef BT_HFP_MODE_DISABLE
	APP_DBG("HFP: ScoDisconnect %d\n", param->index);
	if((gBtHfCt == NULL)||(index < BT_LINK_DEV_NUM && GetHfpState(index) < BT_HFP_STATE_CONNECTED))
		return;

#if (BT_LINK_DEV_NUM == 2)
	extern uint32_t AvrcpStateSuspendCount;
	AvrcpStateSuspendCount = 900;////通话模式中 其他手机正在播放 挂断后跳转到正在播放的手机

//	第一路通话断开时，判断第二路通话是否存在，如果存在则将蓝牙切换到第二路
//	if(SecondTalkingPhoneIndexGet()!=0xff && GetSystemMode() == ModeBtHfPlay)//目前代码 Secondindex暂未使用
//	{
//		SetHfpState(BtCurIndex_Get(), BT_HFP_STATE_CONNECTED);
//		BtLinkStateDisconnect(BtCurIndex_Get());
//		AvrcpCtrlPause(btManager.cur_index);
//
//		FirstTalkingPhoneIndexSet(SecondTalkingPhoneIndexGet());
//		SecondTalkingPhoneIndexSet(0xff);
//		BtCurIndex_Set(FirstTalkingPhoneIndexGet());
//		SetHfpState(BtCurIndex_Get(), BT_HFP_STATE_ACTIVE);
//
//		uint8_t type = GetHfpScoAudioCodecType(BtCurIndex_Get());
//		if(type == 0)
//		{
//			btManager.hfpScoCodecType[btManager.cur_index] = HFP_AUDIO_DATA_PCM;
//		}
//		else if(type == 1)
//		{
//			btManager.hfpScoCodecType[btManager.cur_index] = HFP_AUDIO_DATA_mSBC;
//		}
//
//		BtMidMessageSend(MSG_BT_MID_HFP_CODEC_TYPE_UPDATE, btManager.hfpScoCodecType[btManager.cur_index]);
//		//BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_HF_SCO_CONNECT);
//		gBtHfCt->btHfResumeCnt = 1;
//		APP_DBG("BtHfpScoLinkDisconnected return\n");
//		return;
//	}
#endif

	SetScoConnectFlag(FALSE);

	DBG("GetHfpState(%d) = %d\n", param->index, GetHfpState(param->index));
	switch(GetHfpState(param->index))
	{
		case BT_HFP_STATE_CONNECTED:
			BtHfModeExit();
			break;
			
		case BT_HFP_STATE_INCOMING:
		case BT_HFP_STATE_OUTGOING:
			break;	
			
		case BT_HFP_STATE_ACTIVE:
			if(GetBtHfpVoiceRecognition())
			{
				SetHfpState(param->index,BT_HFP_STATE_CONNECTED);
				DelayExitBtHfModeSet();
			}
			break;
		case BT_HFP_STATE_3WAY_INCOMING_CALL:	
			//SetHfpState(param->index, BT_HFP_STATE_INCOMING);
			break;
		case BT_HFP_STATE_3WAY_OUTGOING_CALL:	
			//SetHfpState(param->index, BT_HFP_STATE_OUTGOING);
			break;					
		case BT_HFP_STATE_3WAY_ATCTIVE_CALL:	
			//SetHfpState(param->index, BT_HFP_STATE_ACTIVE);
			break;
		default:
			break;
	}

	#ifdef BT_RECORD_FUNC_ENABLE
	BtMidMessageSend(MSG_BT_MID_HFP_RECORD_MODE_EXIT, 0);
	#endif
#endif
}

/*****************************************************************************************
* call connect state
****************************************************************************************/
void BtHfpCallConnected(BT_HFP_CALLBACK_PARAMS * param)
{
	APP_DBG("Hfp call connected %d\n", param->index);

	uint8_t index = GetBtManagerHfpIndex(param->index);
	
	if(btManager.HfpCurIndex == 0xff)
	{
		btManager.HfpCurIndex = param->index;
	}
	else if(btManager.HfpCurIndex != param->index)
	{
		//仅更新另外通话通路的状态
		SetHfpState(param->index, BT_HFP_STATE_ACTIVE);
		return;
	}

#ifndef BT_HFP_MODE_DISABLE
	gBtHfp3WayCallsCnt = 0; //清除计数器
	if(GetHfpState(param->index) > BT_HFP_STATE_ACTIVE)
	{
		SetHfpState(param->index, BT_HFP_STATE_3WAY_ATCTIVE_CALL);
	}
	else if(GetHfpState(param->index) == BT_HFP_STATE_NONE || (GetHfpState(param->index) != BT_HFP_STATE_NONE && index >= BT_LINK_DEV_NUM) )
	{
		btManager.hfp_CallFalg = 1;
	}
	else
	{
		SetHfpState(param->index, BT_HFP_STATE_ACTIVE);
	}

	DelayExitBtHfModeCancel();
	EnterBtHfModeReady(param->index);

	if(GetBtConnectedProfile()&BT_CONNECTED_HFP_FLAG)
	{
		#ifdef CFG_FUNC_REMIND_SOUND_EN
		BtMidMessageSend(MSG_BT_MID_HFP_PLAY_REMIND_END, 0);
		#endif
		
		#if defined(CFG_APP_CONFIG) && defined(BT_REMOTE_AEC_DISABLE)
		if(GetSystemMode() == ModeBtHfPlay)
		{
			//BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_HF_DISABLE_NREC);
		}
		#endif
	}
#endif
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtHfpCallDisconnected(BT_HFP_CALLBACK_PARAMS * param)
{
#ifndef BT_HFP_MODE_DISABLE
	APP_DBG("Hfp call disconnect %d,fpCurIndex %d,hfstate %d\n",param->index,btManager.HfpCurIndex,GetHfpState(param->index));
	
	#if (BT_LINK_DEV_NUM == 2)	
	//index 不匹配,则只是更新hf的通话状态
	//if(btManager.cur_index != param->index)
	if(btManager.HfpCurIndex != param->index)
	{
		BT_HFP_STATE hfpState = GetHfpState(param->index);
		switch(hfpState)
		{
			case BT_HFP_STATE_ACTIVE:
				SetHfpState(param->index, BT_HFP_STATE_CONNECTED);
				//SecondTalkingPhoneIndexSet(0xff);//目前代码secondindex暂未使用
				break;
			case BT_HFP_STATE_3WAY_INCOMING_CALL:	//1CALL ACTIVE, 1CALL INCOMING
				SetHfpState(param->index, BT_HFP_STATE_INCOMING);
				break;
			case BT_HFP_STATE_3WAY_OUTGOING_CALL:	//1CALL ACTIVE, 1CALL OUTGOING
				SetHfpState(param->index, BT_HFP_STATE_OUTGOING);
				break;					
			case BT_HFP_STATE_3WAY_ATCTIVE_CALL:	//2CALL ACTIVE
				SetHfpState(param->index, BT_HFP_STATE_ACTIVE);
	
			default:
				break;
		}
		return;
	}
	#endif
	
	if(GetHfpState(param->index) < BT_HFP_STATE_CONNECTED)
		return;

	switch(GetHfpState(param->index))
	{
		case BT_HFP_STATE_OUTGOING:
		case BT_HFP_STATE_INCOMING:
		case BT_HFP_STATE_ACTIVE:
			SetHfpState(param->index, BT_HFP_STATE_CONNECTED);
#if (BT_LINK_DEV_NUM == 2)
			{
				uint8_t 		cfgIndex;
				cfgIndex = (param->index == 0)? 1 : 0;
				if(GetHfpState(cfgIndex) <= BT_HFP_STATE_CONNECTED)
				{
					BtHfModeExit();
					if(btManager.HfpCurIndex == param->index)
						btManager.HfpCurIndex = 0xff;
				}
				else
				{
					btManager.HfpCurIndex = cfgIndex;
					FirstTalkingPhoneIndexSet(btManager.HfpCurIndex);
					//SecondTalkingPhoneIndexSet(0xff);//目前代码secondindex暂未使用
					//此操作涉及到活跃链路的切换
					btManager.cur_index = btManager.HfpCurIndex;
				}
			}
#else
			BtHfModeExit();
			btManager.HfpCurIndex = 0xff;
#endif
			break;
			
		case BT_HFP_STATE_3WAY_INCOMING_CALL:	//1CALL ACTIVE, 1CALL INCOMING
			SetHfpState(param->index,BT_HFP_STATE_INCOMING);
			break;
		case BT_HFP_STATE_3WAY_OUTGOING_CALL:	//1CALL ACTIVE, 1CALL OUTGOING
			SetHfpState(param->index,BT_HFP_STATE_OUTGOING);
			break;					
		case BT_HFP_STATE_3WAY_ATCTIVE_CALL:	//2CALL ACTIVE
			SetHfpState(param->index,BT_HFP_STATE_CONNECTED);
			DelayExitBtHfModeSet();
			break;	
		
		default:
			break;
	}
#endif
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtHfpCallSetupNone(BT_HFP_CALLBACK_PARAMS * param)
{
	APP_DBG("Hfp call setup none  %d,hfstate %d\n", param->index,GetHfpState(param->index));
	
#ifdef BT_HFP_MODE_DISABLE
	//在呼入电话时,对方挂断电话,避免残留的铃声通过A2DP传过来,进行标识
	gHfpCallNoneWaiting = 1;
#else
	if(GetHfpState(param->index) < BT_HFP_STATE_CONNECTED)
		return;
	
	#if (BT_LINK_DEV_NUM == 2)
	if(GetHfpState((param->index == 0) ? 1 : 0) <= BT_HFP_STATE_CONNECTED && GetHfpState(param->index) < 6 && !GetScoConnectFlag() )// && SecondTalkingPhoneIndexGet() == 0xff)//不在三方通话的状态中  目前代码secondindex暂未使用
	{
		SetHfpState(param->index, BT_HFP_STATE_CONNECTED);
		FirstTalkingPhoneIndexSet(0xff);
		BtHfModeExit();
		btManager.HfpCurIndex = 0xff;
	}
	#endif

	SetBtHfpVoiceRecognition(0);

	switch(GetHfpState(param->index))
	{
		case BT_HFP_STATE_3WAY_INCOMING_CALL:
		case BT_HFP_STATE_3WAY_OUTGOING_CALL:
			APP_DBG("3way calling,\n");
			SetHfpState(param->index, BT_HFP_STATE_3WAY_ATCTIVE_CALL);
			break;
		
		case BT_HFP_STATE_INCOMING:
		case BT_HFP_STATE_OUTGOING:
			SetHfpState(param->index, BT_HFP_STATE_CONNECTED);
			BtHfModeExit();
			btManager.HfpCurIndex = 0xff;				
			break;
		case BT_HFP_STATE_ACTIVE:
			if((!GetScoConnectFlag()) && (GetSystemMode() == ModeBtHfPlay))
			{
				SetHfpState(param->index, BT_HFP_STATE_CONNECTED);
				BtHfModeExit();
			}
			break;
		case BT_HFP_STATE_3WAY_ATCTIVE_CALL:
			break;
			
		default:
			break;
	}
#endif
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtHfpCallSetupIncoming(BT_HFP_CALLBACK_PARAMS * param)
{
	APP_DBG("Hfp call setup incoming %d\n", param->index );
	if(GetHfpState(param->index) < BT_HFP_STATE_CONNECTED)
		return;

#if(BT_LINK_DEV_NUM == 2)
	FirstTalkingPhoneIndexSet(param->index);
	APP_DBG("FirstTalkingPhoneIndex %d\n",FirstTalkingPhoneIndexGet());
#endif
	
	if(GetHfpState(param->index) >= BT_HFP_STATE_ACTIVE)
	{
		APP_DBG("3 way incoming\n");
		SetHfpState(param->index, BT_HFP_STATE_3WAY_INCOMING_CALL);
	}
	else
	{
		APP_DBG("Hfp call setup incoming\n");
		SetHfpState(param->index, BT_HFP_STATE_INCOMING);
	}
	
	if(btManager.HfpCurIndex == 0xff){
		btManager.HfpCurIndex = param->index;
	}else if(btManager.HfpCurIndex != param->index){
		return;
	}

	DelayExitBtHfModeCancel();

	if((btManager.cur_index != param->index))
	{
		//将之前的活跃手机暂停播放
		APP_DBG("incoming now,arvcp pause index %d\n",btManager.cur_index);
		AvrcpCtrlPause(btManager.cur_index);

		if(GetHfpState(btManager.cur_index) != BT_HFP_STATE_ACTIVE){
			BtCurIndex_Set(param->index);
		}else{
			return;
		}
	}

#ifdef BT_RECORD_FUNC_ENABLE
	if(GetSystemMode() == AppModeBtRecordPlay)
		gSysRecordMode2HfMode = 1;
	
	BtMidMessageSend(MSG_BT_MID_HFP_RECORD_MODE_DEREGISTER, 0);
#endif

	EnterBtHfModeReady(param->index);
	btManager.hfpVoiceState = 0;
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtHfpCallSetupOutgoing(BT_HFP_CALLBACK_PARAMS * param)
{
	APP_DBG("Hfp call setup outgoing %d\n", param->index );
	
	if(GetHfpState(param->index) < BT_HFP_STATE_CONNECTED)
		return;
	
	if(btManager.HfpCurIndex == 0xff)
	{
		btManager.HfpCurIndex = param->index;
	}else if(btManager.HfpCurIndex != param->index){
		return;
	}

#if(BT_LINK_DEV_NUM == 2)
	FirstTalkingPhoneIndexSet(param->index);
	APP_DBG("FirstTalkingPhoneIndex %d\n",FirstTalkingPhoneIndexGet());
#endif

	if(GetHfpState(param->index) >= BT_HFP_STATE_ACTIVE)
	{
		APP_DBG("3 way outgoing\n");
		SetHfpState(param->index, BT_HFP_STATE_3WAY_OUTGOING_CALL);
	}
	else
	{
		APP_DBG("Hfp call setup outgoing index:%d\n",param->index);
		SetHfpState(param->index, BT_HFP_STATE_OUTGOING);
	}

	if((btManager.cur_index != param->index))
	{
		//将之前的活跃手机暂停播放
		APP_DBG("outgoing now,arvcp pause index %d\n",btManager.cur_index);
		AvrcpCtrlPause(btManager.cur_index);

		if(GetHfpState(btManager.cur_index) != BT_HFP_STATE_ACTIVE)
		{
			BtCurIndex_Set(param->index);
		}
		else
		{
			return;
		}
	}

#ifdef BT_RECORD_FUNC_ENABLE
	BtMidMessageSend(MSG_BT_MID_HFP_RECORD_MODE_ENTER, 0);
#else
	EnterBtHfModeReady(param->index);
#endif
	btManager.hfpVoiceState = 1;
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtHfpCallSetupAlert(BT_HFP_CALLBACK_PARAMS * param)
{
	APP_DBG("Hfp call setup alert %d\n", param->index );
	//if(GetHfpState(param->index) < BT_HFP_STATE_CONNECTED)
	//	return;

	if( GetHfpState(param->index) >= BT_HFP_STATE_ACTIVE)
	{
		APP_DBG("SetupAlert: 3 way outgoing! %d\n",param->index);
		SetHfpState(param->index ,BT_HFP_STATE_3WAY_OUTGOING_CALL);
	}else{
		SetHfpState(param->index ,BT_HFP_STATE_OUTGOING);
	}
	
	if( param->index != BtCurIndex_Get()) return;//不是当前活跃只更新状态

	if(btManager.HfpCurIndex == 0xff)
	{
		btManager.HfpCurIndex = param->index;
	}else if(btManager.HfpCurIndex != param->index){
		return;
	}

#ifdef BT_RECORD_FUNC_ENABLE
	BtMidMessageSend(MSG_BT_MID_HFP_RECORD_MODE_DEREGISTER, 0);
#endif
	EnterBtHfModeReady(param->index);
	
	btManager.hfpVoiceState = 0;
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtHfpCallRing(BT_HFP_CALLBACK_PARAMS * param)
{
	APP_DBG("Hfp RING %d...\n", param->index);

	if((param->index != btManager.cur_index) || (GetHfpState(param->index) < BT_HFP_STATE_CONNECTED))
		return;

	if(GetHfpState(param->index) <= BT_HFP_STATE_ACTIVE)
	{
		SetHfpState(param->index, BT_HFP_STATE_INCOMING);
		#if(BT_LINK_DEV_NUM == 2)
		if((btManager.HfpCurIndex != 0xff) && (btManager.HfpCurIndex == param->index))
		#endif
		{
			EnterBtHfModeReady(param->index);  // 来电铃声没建立SCO的这类手机，如果重启后有这个信号则自动进入HFP Mode
		}
	}
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtHfpCallerIdNotify(BT_HFP_CALLBACK_PARAMS * param)
{
	APP_DBG("Hfp caller id %d : %s\n", param->index, param->params.hfpPhoneNumber);

	SetBtCallInPhoneNumber(param->index, (uint8_t *)param->params.hfpPhoneNumber, param->paramsLen);

	if(param->index != btManager.cur_index)
		return;

#ifdef CFG_FUNC_REMIND_SOUND_EN
	BtMidMessageSend(MSG_BT_MID_HFP_PLAY_REMIND, 0);
#endif
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtHfpCallWaitNotify(BT_HFP_CALLBACK_PARAMS * param)
{
	APP_DBG("Hfp call wait %d : %s\n", param->index, param->params.hfpPhoneNumber);
	
	SetBtCallWaitingNotify(TRUE);

	SetBtCallInPhoneNumber(param->index, (uint8_t *)param->params.hfpPhoneNumber, param->paramsLen);
}

/*****************************************************************************************
 * 手机电池电量等级
 ****************************************************************************************/
void BtHfpBatteryLevel(BT_HFP_CALLBACK_PARAMS * param)
{
	APP_DBG("Hfp battery level %d : [%d]\n", param->index, param->params.hfpBattery);
	
	SetBtBatteryLevel(param->params.hfpBattery);
}

/*****************************************************************************************
 * 手机信号强度
 ****************************************************************************************/
void BtHfpSignalLevel(BT_HFP_CALLBACK_PARAMS * param)
{
	APP_DBG("Hfp signal level %d : [%d]\n", param->index, param->params.hfpSignal);

	SetBtHfpSignalLevel(param->params.hfpSignal);
}

//index: 1 or 2
void BtHfpCallListUnusedSet(uint8_t callId)
{
	if((callId > 2)||(callId == 0))
	{
		APP_DBG("Hfp caller id %d error\n", callId);
		return;
	}
	
	btManager.hfpCallListParams[callId-1].used = 1;
}

//index: 1 or 2
void BtHfpCurCallListSet(uint8_t callId, cbCallListParms params)
{
	if((callId > 2)||(callId == 0))
	{
		APP_DBG("Hfp caller id %d error\n", callId);
		return;
	}
	
	btManager.hfpCallListParams[callId-1].used = 1;
	btManager.hfpCallListParams[callId-1].dir = params.dir;
	btManager.hfpCallListParams[callId-1].state = params.state;
}

//index: 1 or 2
uint8_t BtHfpCurCallListGet(uint8_t callId)
{
	if((callId > 2)||(callId == 0))
	{
		APP_DBG("Hfp caller id %d error\n", callId);
		return 0xff;
	}
	
	return btManager.hfpCallListParams[callId-1].state;
}

void BtHfpSingleCallingStateSet(uint8_t curIndex, uint8_t state)
{
	if((state == 2) || (state == 3))
	{
		if(GetHfpState(curIndex) != BT_HFP_STATE_OUTGOING)
		{
			printf("-----> %x -> signal outgoing\n", GetHfpState(curIndex));
			SetHfpState(curIndex, BT_HFP_STATE_OUTGOING);
		}
	}
	else if(state == 4)
	{
		if(GetHfpState(curIndex) != BT_HFP_STATE_INCOMING)
		{
			printf("-----> %x -> signal incoming\n", GetHfpState(curIndex));
			SetHfpState(curIndex, BT_HFP_STATE_INCOMING);
			BtHfModeEnter_Index(curIndex);
		}
	}
	else //other
	{
		if(GetHfpState(curIndex) != BT_HFP_STATE_ACTIVE)
		{
			printf("-----> %x -> signal active\n", GetHfpState(curIndex));
			SetHfpState(curIndex, BT_HFP_STATE_ACTIVE);
		}
	}
}

void BtHfp3WayCallingStateSet(uint8_t curIndex)
{
	if((btManager.hfpCallListParams[0].used)&&(btManager.hfpCallListParams[1].used))
	{
		if((btManager.hfpCallListParams[0].state == 2) || (btManager.hfpCallListParams[0].state == 3)
			||(btManager.hfpCallListParams[1].state == 2) || (btManager.hfpCallListParams[1].state == 3))
		{
			if(GetHfpState(curIndex) != BT_HFP_STATE_3WAY_OUTGOING_CALL)
			{
				printf("-----> signal active -> 3way outgoing\n");
				SetHfpState(curIndex, BT_HFP_STATE_3WAY_OUTGOING_CALL);
			}
		}
		else if((btManager.hfpCallListParams[0].state == 4) || (btManager.hfpCallListParams[0].state == 5)
			||(btManager.hfpCallListParams[1].state == 4) || (btManager.hfpCallListParams[1].state == 5))
		{
			if(GetHfpState(curIndex) != BT_HFP_STATE_3WAY_INCOMING_CALL)
			{
				printf("-----> signal active -> 3way incoming\n");
				SetHfpState(curIndex, BT_HFP_STATE_3WAY_INCOMING_CALL);
			}
		}
		else //other
		{
			if(GetHfpState(curIndex) != BT_HFP_STATE_3WAY_ATCTIVE_CALL)
			{
				printf("-----> signal active -> 3way active\n");
				SetHfpState(curIndex, BT_HFP_STATE_3WAY_ATCTIVE_CALL);
			}
		}
	}
}

void BtHfpCurCallState(BT_HFP_CALLBACK_PARAMS * param)
{
	APP_DBG("Hfp caller :%d,  state :%d\n", param->params.callListParms.index,  param->params.callListParms.state);

	if(param->index != btManager.cur_index)
		return;

	if(GetHfpState(param->index) < BT_HFP_STATE_ACTIVE)
	{
		BtHfpSingleCallingStateSet(btManager.cur_index, param->params.callListParms.state);
	}

	//将手机端的通话链路状态进行数据保存
	BtHfpCurCallListSet(param->params.callListParms.index, param->params.callListParms);

	if(GetSystemMode() != ModeBtHfPlay)
		return;
	
	//此处获取手机的通话状态,以此来更新本地的三方通话的状态
	if(param->params.callListParms.index == 1)
	{
		Caller1Count++;
		
		//三方通话,获取call1,如call1和call2都获取,则保留三方状态
		if(Caller1Count==Caller2Count)
		{
			gBtHfp3WayCallsCnt = 0;//清除计数器

			BtHfp3WayCallingStateSet(btManager.cur_index);
		}
		//三方通话,先获取call2,然后再call1获取到
		else
		{
			if(GetHfpState(param->index) > BT_HFP_STATE_ACTIVE)
			{
				gBtHfp3WayCallsCnt = 300;//delay 300ms 内获取call2状态
			}else{
				gBtHfp3WayCallsCnt = 0;//delay 10ms 更新通话状态
			}
		}
	}
	else if(param->params.callListParms.index == 2)
	{
		Caller2Count++;
	
		//三方通话,获取call2,如call1和call2都获取,则保留三方状态
		if(Caller1Count==Caller2Count)
		{
			gBtHfp3WayCallsCnt = 0;//清除计数器

			BtHfp3WayCallingStateSet(btManager.cur_index);
		}
		//三方通话,先获取call2,然后再call1获取到
		else
		{
			if(GetHfpState(param->index) > BT_HFP_STATE_ACTIVE)
			{
				gBtHfp3WayCallsCnt = 300;//delay 300ms 内获取call2状态
			}else{
				gBtHfp3WayCallsCnt = 0;//delay 10ms 更新通话状态
			}
		}
	}
}

/*****************************************************************************************
* HFP voice recognition
****************************************************************************************/
void BtHfpVoiceRecognition(BT_HFP_CALLBACK_PARAMS * param)
{
	APP_DBG("Hfp vocie recognition %d : %d\n", param->index, param->params.hfpVoiceRec);
	
	if(GetHfpState(param->index) < BT_HFP_STATE_CONNECTED)
		return;
	
	SetBtHfpVoiceRecognition(param->params.hfpVoiceRec);
	
	if(param->params.hfpVoiceRec)
	{
		APP_DBG("Hfp vocie recognition TRUE\n");
		SetHfpState(param->index, BT_HFP_STATE_ACTIVE);
		#if(BT_HFP_SUPPORT == ENABLE)
		DelayExitBtHfModeCancel();
		#endif
	}
	else
	{
		APP_DBG("Hfp vocie recognition FALSE\n");
		if(GetHfpState(param->index) == BT_HFP_STATE_ACTIVE)
		{
			SetHfpState(param->index, BT_HFP_STATE_CONNECTED);
			
			#if(BT_LINK_DEV_NUM == 2)
			if((GetHfpState(0) <= BT_HFP_STATE_CONNECTED)&&(GetHfpState(1) <= BT_HFP_STATE_CONNECTED))
			#endif
			{
				//解决双手机连接时,先操作siri,退出siri未清理index,导致另外个手机打电话index不对而进入不了通话的问题
				btManager.HfpCurIndex = 0xff;
				#if(BT_HFP_SUPPORT == ENABLE)
				DelayExitBtHfModeSet();
				#endif
			}
		}
	}
}

/*****************************************************************************************
* HFP speaker volume
****************************************************************************************/
void BtHfpSpeakerVolume(BT_HFP_CALLBACK_PARAMS * param)
{
	APP_DBG("Hfp speaker vol %d : [%d]\n", param->index, param->params.hfpVolGain);

	if(param->index == btManager.cur_index)
	{
		SetBtHfpSpeakerVolume(param->params.hfpVolGain);
	}
}

/*****************************************************************************************
* HFP codec type
****************************************************************************************/
void BtHfpCodecType(BT_HFP_CALLBACK_PARAMS * param)
{
	//type: 1=CVSD, 2=MSBC
	uint8_t index = GetBtManagerHfpIndex(param->index);
	if(index >= BT_LINK_DEV_NUM)
		return;

	if(param->params.scoCodecType == HFP_AUDIO_DATA_mSBC)
	{
		APP_DBG("btManager.hfpScoCodecType[%d] --->>+++mSBC+++\n",index);
		btManager.hfpScoCodecType[index] = HFP_AUDIO_DATA_mSBC;
	}
	else if(param->params.scoCodecType == HFP_AUDIO_DATA_PCM)
	{
		APP_DBG("btManager.hfpScoCodecType[%d] --->>+++CVSD+++\n",index);
		btManager.hfpScoCodecType[index] = HFP_AUDIO_DATA_PCM;
	}
	else
	{
		APP_DBG("!!!ERROR: param->params.scoCodecType = %d\n",param->params.scoCodecType);
		btManager.hfpScoCodecType[index] = HFP_AUDIO_DATA_mSBC;//默认mSBC
	}
	if(btManager.cur_index == index)//不是当前通话不更新数据格式
	BtMidMessageSend(MSG_BT_MID_HFP_CODEC_TYPE_UPDATE, btManager.hfpScoCodecType[index]);
}

/*****************************************************************************************
* HFP manufactory info
****************************************************************************************/
void BtHfpManufactoryInfo(BT_HFP_CALLBACK_PARAMS * param)
{
	APP_DBG("%s\n", param->params.hfpRemoteManufactory);
	
	if(strstr(param->params.hfpRemoteManufactory,"Apple Inc."))
		btManager.btLinked_env[param->index].appleDeviceFlag = 1;
	else
		btManager.btLinked_env[param->index].appleDeviceFlag = 0;
		
	btManager.appleDeviceFlag = btManager.btLinked_env[param->index].appleDeviceFlag;
}

/*****************************************************************************************
* 针对笔记本传输msbac数据传输数据进行修复
****************************************************************************************/
const unsigned char Esco_mute_pack[] =
{0x1 ,0x8 ,0xad,0x0 ,0x0 ,0xc5,0x0 ,0x0 ,0x0 ,0x0 ,
 0x77,0x6d,0xb6,0xdd,0xdb,0x6d,0xb7,0x76,0xdb,0x6d,
 0xdd,0xb6,0xdb,0x77,0x6d,0xb6,0xdd,0xdb,0x6d,0xb7,
 0x76,0xdb,0x6d,0xdd,0xb6,0xdb,0x77,0x6d,0xb6,0xdd,
 0xdb,0x6d,0xb7,0x76,0xdb,0x6d,0xdd,0xb6,0xdb,0x77,
 0x6d,0xb6,0xdd,0xdb,0x6d,0xb7,0x76,0xdb,0x6c,0x0};

void MSBC_PackFix(uint8_t* data,uint16_t len)
{
	char Esco_header[] = {0xad,0,0};
	uint16_t SBC_len = len;
	static unsigned  char syncheader_per = 2;

	if( memcmp(Esco_header,&data[2],3)!=0 )
	{
		int i = 0;

		if( memcmp(Esco_header,&data[syncheader_per],3)==0 )
		{
			i = syncheader_per-2;
		}
		else
			i = 4;
		for(;i<SBC_len;i++)
		{
			if( memcmp(Esco_header,&data[i],3)==0 )
			{
				syncheader_per = i;
				if(btManager.Esco_RePackLen != 0)
				{
					memcpy(&btManager.Esco_RePack[btManager.Esco_RePackLen],&data[0],i-2);
					if(SaveHfpScoDataToBuffer)
						SaveHfpScoDataToBuffer(btManager.Esco_RePack,len);
					btManager.Esco_RePackLen = 0;
				}
				else//没有完整payload
				{
					//这里是没找到SBC包头，直接往APP发静音包。
					if(SaveHfpScoDataToBuffer)
						SaveHfpScoDataToBuffer((uint8_t *)Esco_mute_pack,len);
				}
				memcpy(btManager.Esco_RePack,&data[i-2],SBC_len+2-i);
				btManager.Esco_RePackLen = SBC_len+2-i;
				break;
			}
		}
		if(i>=SBC_len)//扫描后发现没有包头。
		{
			//这里是没找到SBC包头，直接往APP发静音包。
			if(SaveHfpScoDataToBuffer)
				SaveHfpScoDataToBuffer((uint8_t *)Esco_mute_pack,len);
			btManager.Esco_RePackLen = 0;
		}
	}
	else
	{
		if(SaveHfpScoDataToBuffer)
			SaveHfpScoDataToBuffer(data,len);
	}
}

/*****************************************************************************************
* HFP sco data
****************************************************************************************/
void BtHfpScoDataReceived(BT_HFP_CALLBACK_PARAMS * param)
{
	if(param->index != btManager.cur_index)
		return;

	if((GetHfpState(param->index) == BT_HFP_STATE_INCOMING) 
		&& (sys_parameter.bt_CallinRingType >= USE_LOCAL_AND_PHONE_RING)
		&& !GetScoConnectFlag())
	{
		BtMidMessageSend(MSG_BT_MID_HFP_PLAY_REMIND, 0);
	}
	else
	{
		//将数据缓存到bt sco fifo
		if(btManager.hfpScoCodecType[btManager.cur_index] != HFP_AUDIO_DATA_mSBC)
		{
			if(testRecvLen != param->paramsLen)
			{
				testRecvLen = param->paramsLen;
				APP_DBG("CVSD len:%d\n", testRecvLen);
			}
		}
		else
		{
			if(testRecvLen != param->paramsLen)
			{
				testRecvLen = param->paramsLen;
				APP_DBG("MSBC len:%d\n", testRecvLen);
			}
		}

        if(SaveHfpScoDataToBuffer)
			SaveHfpScoDataToBuffer(param->params.scoReceivedData,param->paramsLen);
	}
}

/*****************************************************************************************
* HFP battery sync
****************************************************************************************/
#ifdef CFG_FUNC_POWER_MONITOR_EN
void SetBtHfpBatteryLevel(PWR_LEVEL level, uint8_t flag)
{
#ifdef BT_HFP_BATTERY_SYNC
	if(GetHfpState(BtCurIndex_Get()) < BT_HFP_STATE_CONNECTED)
		return;

	if((flag == 0)&&(powerLevelBak == level))
		return;

	powerLevelBak = level;
	btManager.HfpBatLevel = level;
	
	BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_HF_SET_BATTERY);
#endif
}
#endif

/*****************************************************************************************
* HFP 获取手机的通话状态 runloop
****************************************************************************************/
static uint32_t gBtHfpGetCurCallStateCnt = 0;
void BtHfpGetCurCallStateRunloop(void)
{
	if(btManager.cur_index >= BT_LINK_DEV_NUM)
		return;

	if(++gBtHfpGetCurCallStateCnt >= 1000)
	{
		gBtHfpGetCurCallStateCnt = 0;
		BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_HF_GET_CUR_CALLNUMBER);
	}
	
	if(gBtHfp3WayCallsCnt)
	{
		gBtHfp3WayCallsCnt--;
		if(gBtHfp3WayCallsCnt==0)
		{
			if(Caller1Count>Caller2Count)
			{
				BtHfpCallListUnusedSet(2);
				BtHfpSingleCallingStateSet(btManager.cur_index, BtHfpCurCallListGet(1));
			}
			else if(Caller1Count<Caller2Count)
			{
				BtHfpCallListUnusedSet(1);
				BtHfpSingleCallingStateSet(btManager.cur_index, BtHfpCurCallListGet(2));
			}

			Caller1Count = 0;
			Caller2Count = 0;
			gBtHfp3WayCallsCnt = 0;
		}
	}
}

/*****************************************************************************************
* 进入通话模式,注册通话执行的回调函数
****************************************************************************************/
void BtHfpRunloopRegister(void)
{
	gBtHfpGetCurCallStateCnt = 0;
	BtAppiFunc_BtHfpGetCurCallStateProcess(BtHfpGetCurCallStateRunloop);
}

/*****************************************************************************************
* 退出通话模式,注销通话执行的回调函数
****************************************************************************************/
void BtHfpRunloopDeregister(void)
{
	gBtHfpGetCurCallStateCnt = 0;
	BtAppiFunc_BtHfpGetCurCallStateProcess(NULL);
}

#endif
