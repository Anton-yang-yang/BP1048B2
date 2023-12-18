/**
 **************************************************************************************
 * @file    bt_manager.c
 * @brief   management of all bluetooth event and apis
 *
 * @author  Halley
 * @version V1.1.0
 *
 * $Created: 2016-07-18 16:24:11$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
 

#include "type.h"
#include "bt_manager.h"
#include "bt_app_ddb_info.h"
#include "bt_common_api.h"

#ifdef CFG_APP_CONFIG
#include "app_config.h"
#include "main_task.h"
#endif
#include "debug.h"
#include "rtos_api.h"

#include "bt_app_init.h"
#include "app_message.h"
#include "bt_interface.h"
#include "bt_app_connect.h"
#include "bt_app_common.h"

#ifdef BT_TWS_SUPPORT
#include "bt_app_tws_connect.h"
#endif//#ifdef BT_TWS_SUPPORT
#include "bt_stack_service.h"

extern uint32_t IsBtHfMode(void);

BT_MANAGER_ST		btManager;

//BT_CHECK_EVENT_LIST btCheckEventList = BT_EVENT_NONE;
//uint32_t btEventListB1Count = 0; //BT_EVENT_L2CAP_LINK_DISCONNECT
//uint32_t btEventListB1State = 0;
//uint32_t btEventListCount = 0;
uint32_t gBtEnterDeepSleepFlag = 0;

extern BT_CONFIGURATION_PARAMS		*btStackConfigParams;


#ifdef BT_TWS_SUPPORT
extern uint32_t gBtTwsSniffLinkLoss;
#endif

extern uint32_t GetCurTotaBtRecNum(void);


#ifndef BT_TWS_SUPPORT
extern uint8_t BtNumericalDispEnable;
/***********************************************************************************
 * 配置蓝牙security功能，1是打开，0是关闭(系统默认关闭)，
 * 这个函数调用需要在bt stack初始化之前
 **********************************************************************************/
void BtNumericalDisplayFuncEnable(uint8_t val)
{
	BtNumericalDispEnable = val;
}
/***********************************************************************************
 * 校验手机发过来的key，客户自定义，
 * 校验完成后调用BtNumericalAccecptOrReject来决定是否连接
 **********************************************************************************/
void BtNumericalVerify(uint32_t val)
{
	APP_DBG("SEC Key: %ld\n", val);
}
#endif

/***********************************************************************************
 * 
 **********************************************************************************/
void load_tws_filter_infor(uint8_t *infor)
{
#ifdef BT_TWS_SUPPORT
	memcpy(infor, btManager.TwsFilterInfor, 6);
#endif
}

uint8_t *get_cur_tws_addr(void)
{
#ifdef BT_TWS_SUPPORT
	return &btManager.btTwsDeviceAddr[0];
#else
	return NULL;
#endif
}
/***********************************************************************************
 * 配置可见性函数接口
 **********************************************************************************/
static void BtSetAccessModeApi(BtAccessMode accessMode)
{
	//printf("BtSetAccessModeApi:%x,  bk:%x\n", accessMode, GetBtDeviceConnState());
	//if(GetBtDeviceConnState() != accessMode)
	{
		#ifdef BT_USER_VISIBILITY_STATE
		if(GetBtUserVsibilityLock())
		{
			APP_DBG("user hold set access mode!\n");
		}else
		#endif
		{
			APP_DBG("set access mode: %d\n", accessMode);
			BTSetAccessMode(accessMode);
		}	
	}
}

void BtSetAccessMode_NoDisc_NoCon(void)
{
	BtSetAccessModeApi(BtAccessModeNotAccessible);
}

void BtSetAccessMode_NoDisc_Con(void)
{
	BtSetAccessModeApi(BtAccessModeConnectableOnly);
}

void BtSetAccessMode_Disc_Con(void)
{
	if (GetBtManager()->btAccessModeEnable)
		BtSetAccessModeApi(BtAccessModeGeneralAccessible);
	else
		BtSetAccessModeApi(BtAccessModeConnectableOnly);
}

/***********************************************************************************
 * 蓝牙连接状态 -- 应用层管理
 * BT_DEVICE_CONNECTION_MODE_NONE -- cann't be discovered and connected
 * BT_DEVICE_CONNECTION_MODE_DISCOVERIBLE -- can be discovered
 * BT_DEVICE_CONNECTION_MODE_CONNECTABLE -- can be connected
 * BT_DEVICE_CONNECTION_MODE_ALL -- can be discovered and connected
 **********************************************************************************/
bool SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE mode)
{
	btManager.deviceConMode = mode;
	return TRUE;
}

BT_DEVICE_CONNECTION_MODE GetBtDeviceConnState(void)
{
	return btManager.deviceConMode;
}

uint8_t GetBtManagerA2dpIndex(uint8_t index)
{
	uint8_t i;

	for(i = 0; i < BT_LINK_DEV_NUM; i++)
	{
		if(index == btManager.btLinked_env[i].a2dp_index)
		{
			break;
		}
	}

	return i;
}

uint8_t GetBtManagerAvrcpIndex(uint8_t index)
{
	uint8_t i;

	for(i = 0; i < BT_LINK_DEV_NUM; i++)
	{
		if(index == btManager.btLinked_env[i].avrcp_index)
		{
			break;
		}
	}

	return i;
}

uint8_t GetBtManagerHfpIndex(uint8_t index)
{
	uint8_t i;

	for(i = 0; i < BT_LINK_DEV_NUM; i++)
	{
		if(index == btManager.btLinked_env[i].hf_index)
		{
			break;
		}
	}

	return i;
}

/***********************************************************************************
 * 实际无效
 **********************************************************************************/
uint8_t* app_bt_page_state(void)
{
	return NULL;
}

void BtManageInit(void)
{
	APP_DBG("BtManageInit\n");
	uint8_t index = 0;
	for(index = 0; index<BT_LINK_DEV_NUM; index++)
	{
		btManager.btLinked_env[index].btLinkState = 0;
		memset(btManager.btLinked_env[index].remoteAddr, 0, 6);
		btManager.btLinked_env[index].hf_index = 0xff;
		btManager.btLinked_env[index].a2dp_index = 0xff;
		btManager.btLinked_env[index].avrcp_index = 0xff;
		btManager.btLinked_env[index].a2dpState = BT_A2DP_STATE_NONE;
		btManager.btLinked_env[index].avrcpState = BT_AVRCP_STATE_NONE;
		btManager.btLinked_env[index].hfpState = BT_HFP_STATE_NONE;
	}
	btManager.cur_index = 0;
	btManager.back_index = 0;
	btManager.linkedNumber = 0;
}

/*************************************************************************************
 * 蓝牙连接成功
 * input: flag  -- 0:需要a2dp和avrcp协议连接成功,才能置btLinkState标志
 *                 1:不在乎当前连上几个协议,系统认为蓝牙已经连成功,将走连接成功流程
 * input: index -- 蓝牙链路索引
 *************************************************************************************/
void BtLinkStateConnect(uint8_t flag, uint8_t index)
{
	//printf("btManager.btLinked_env[%d].btLinkState %d\n",index,btManager.btLinked_env[index].btLinkState);
	//printf("btManager.btLinked_env[%d].a2dpState %d index:%d\n",index,btManager.btLinked_env[index].a2dpState,btManager.btLinked_env[index].a2dp_index);
	//printf("btManager.btLinked_env[%d].avrcpState %d index:%d\n",index,btManager.btLinked_env[index].avrcpState,btManager.btLinked_env[index].avrcp_index);
	//printf("btManager.btLinked_env[%d].hfpState %d index:%d\n",index,btManager.btLinked_env[index].hfpState,btManager.btLinked_env[index].hf_index);
	if(btManager.btLinked_env[index].btLinkState)
	{
		APP_DBG("profile Upgrade\n");
		BtDdb_UpLastPorfile(btManager.btLinked_env[index].btLinkedProfile);
		return ;
	}

	if(((!btManager.btLinked_env[index].btLinkState)
		&&(btManager.btLinked_env[index].a2dpState >= BT_A2DP_STATE_CONNECTED)
		&& (btManager.btLinked_env[index].avrcpState >= BT_AVRCP_STATE_CONNECTED))
#if(BT_HFP_SUPPORT == ENABLE)
		/*&& (btManager.btLinked_env[index].hfpState >= BT_HFP_STATE_CONNECTED)*/
#endif
		|| (flag)
		)
	{
		btManager.btLinkState = 1;
		btManager.btLinked_env[index].btLinkState = 1;
		APP_DBG("set btManager.btLinked_env[%d].btLinkState %d\n",index,btManager.btLinked_env[index].btLinkState);
		BtMidMessageSend(MSG_BT_MID_STATE_CONNECTED, 0);

		btManager.linkedNumber++;
#ifdef BT_TWS_SUPPORT
		#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
		extern void app_bt_device_connect(uint8_t connected);
		app_bt_device_connect(1);
		#endif

		if(btManager.linkedNumber >= 2)
			BtReconnectTwsStop();
#endif

		APP_DBG("linkedNumber %d,index %d\n",btManager.linkedNumber,index);
#if (BT_LINK_DEV_NUM == 2)
		if((btManager.btLinked_env[0].a2dpState != BT_A2DP_STATE_STREAMING)&&(btManager.btLinked_env[1].a2dpState != BT_A2DP_STATE_STREAMING))
#endif
		{
			btManager.btDdbLastProfile = btManager.btLinked_env[index].btLinkedProfile;
			//FlashErase(BTDB_ALIVE_RECORD_ADDR,BTDB_ALIVE_RECORD_MEM_SIZE);
			BtDdb_UpgradeLastBtAddr(GetBtManager()->btLinked_env[index].remoteAddr, btManager.btDdbLastProfile);
			//btManager.btDdbLastProfile = btManager.btLinked_env[index].btLinkedProfile;
			//BtDdb_UpgradeLastBtProfile(GetBtManager()->btLinked_env[index].remoteAddr, btManager.btDdbLastProfile);
		}

		if(btManager.ddbUpdate)
		{
			btManager.ddbUpdate = 0;
			if(!btManager.btLastAddrUpgradeIgnored)
			{
				//save total device info to flash
#ifdef CFG_FUNC_OPEN_SLOW_DEVICE_TASK
				{
					extern void SlowDevice_MsgSend(uint16_t msgId);
					SlowDevice_MsgSend(MSG_DEVICE_BT_LINKED_INFOR_WRITE);
				}
#else
				SaveTotalDevRec2Flash(1 + BT_REC_INFO_LEN * MAX_BT_DEVICE_NUM/*one total rec block size*/,
							 	GetCurTotaBtRecNum());
#endif
			}
		}

#ifdef BT_TWS_SUPPORT
		BtAccessModeUpdate(GetBtDeviceConnState());
	#if ((TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM))
		if((btManager.twsFlag)&&(btManager.twsRole != BT_TWS_MASTER))
		{
			//BtDdb_ClearTwsDeviceAddrList();
		}

		if((btManager.twsState == BT_TWS_STATE_CONNECTED)&&(btManager.twsRole == BT_TWS_SLAVE))
		{
			//BtStopReconnectTwsReg();
		}
	#endif
#endif
	}

#if (BT_AVRCP_VOLUME_SYNC == ENABLE)
//	extern MessageHandle GetSysModeMsgHandle(void);
	if((GetSystemMode() == ModeBtAudioPlay) && (btManager.btLinked_env[index].avrcpState >= BT_AVRCP_STATE_CONNECTED))
	{
		MessageContext		msgSend;

		SetBtSyncVolume(mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM]);

		msgSend.msgId		= MSG_BT_PLAY_VOLUME_SET;
		MessageSend(GetSysModeMsgHandle(), &msgSend);
	}
#endif

	//btManager.btLinkState = 1;
}

void BtLinkStateDisconnect(uint8_t index)
{
	uint8_t i = 0;
	//printf("btManager.btLinked_env[%d].btLinkState %d\n",index,btManager.btLinked_env[index].btLinkState);
	//printf("btManager.btLinked_env[%d].a2dpState %d index:%d\n",index,btManager.btLinked_env[index].a2dpState,btManager.btLinked_env[index].a2dp_index);
	//printf("btManager.btLinked_env[%d].avrcpState %d index:%d\n",index,btManager.btLinked_env[index].avrcpState,btManager.btLinked_env[index].avrcp_index);
	//printf("btManager.btLinked_env[%d].hfpState %d index:%d\n",index,btManager.btLinked_env[index].hfpState,btManager.btLinked_env[index].hf_index);

	if( btManager.btLinked_env[index].btLinkState
		&& (btManager.btLinked_env[index].avrcpState == BT_AVRCP_STATE_NONE)
#if(BT_HFP_SUPPORT == ENABLE)
		&& (btManager.btLinked_env[index].hfpState == BT_HFP_STATE_NONE)
#endif
		&& (btManager.btLinked_env[index].a2dpState == BT_A2DP_STATE_NONE))
	{
		//btManager.btLinkState = 0;
		btManager.btLinked_env[index].btLinkState = 0;
		memset(btManager.btLinked_env[index].remoteAddr, 0, 6);
		GetBtManager()->btLinked_env[index].hf_index = 0xff;
		GetBtManager()->btLinked_env[index].a2dp_index = 0xff;
		GetBtManager()->btLinked_env[index].avrcp_index = 0xff;
		SetBtDisconnectProfile(index,BT_CONNECTED_HFP_FLAG | BT_CONNECTED_A2DP_FLAG | BT_CONNECTED_AVRCP_FLAG);
		APP_DBG("set btManager.btLinked_env[%d].btLinkState %d\n",index,btManager.btLinked_env[index].btLinkState);
		//BtStopReconnect();

		if((btManager.cur_index != index)&&(IsBtHfMode()))
		{
			//当前不是活跃的index,并且当前处于通话模式下,则不播放提示音
		}
		else
		{
			BtMidMessageSend(MSG_BT_MID_STATE_DISCONNECT, 0);
		}

		btManager.linkedNumber--;
		APP_DBG("linkedNumber %d,index %d\n",btManager.linkedNumber,index);
		
		if(btManager.linkedNumber == 0)
		{
			btManager.btDisConnectingFlag = 0;
			//BTSetAccessMode(BtAccessModeGeneralAccessible);
		}
		else if(btManager.btDisConnectingFlag == 0)
		{
			for(i=0;i<BT_LINK_DEV_NUM;i++)
			{
				if(index != i)
				{
					BtCurIndex_Set(i);
				}
			}
		}
		
		if(btManager.linkedNumber == 0)
		{
			btManager.btLastAddrUpgradeIgnored = 0;
			btManager.btLinkState = 0;

			#if defined(BT_TWS_SUPPORT) && (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
			extern void app_bt_device_connect(uint8_t connected);
			app_bt_device_connect(0);
			#endif
		}
	}
}

bool GetBtLinkState(uint8_t index)
{
	return btManager.btLinked_env[index].btLinkState;
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void SetBtCurConnectFlag(uint8_t state)
{
	btManager.btCurConnectFlag = state;
}

uint8_t GetBtCurConnectFlag(void)
{
	return btManager.btCurConnectFlag;
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtStackCallback(BT_STACK_CALLBACK_EVENT event, BT_STACK_CALLBACK_PARAMS * param)
{
	switch(event)
	{
		case BT_STACK_EVENT_COMMON_STACK_INITIALIZED:
		{
			BtStackInitialized();
		}
		break;

		case BT_STACK_EVENT_COMMON_STACK_FREE_MEM_SIZE:
		{
			//APP_DBG("stack mem:%d, %d\n", param->params.stackMemParams.stackMallocMemSize, param->params.stackMemParams.stackFreeMemSize);
		}
		break;

		case BT_STACK_EVENT_COMMON_STACK_OUT_OF_MEM_ERR:
		{
			APP_DBG("!!!ERR: BT_STACK_EVENT_COMMON_STACK_FREE_MEM_SIZE\n");
		}
		break;

		case BT_STACK_EVENT_COMMON_INQUIRY_RESULT:
		{
			//APP_DBG("Inquriy Result:\n");
			//APP_DBG("\t%02x:%02x:%02x:%02x:%02x:%02x",
			//		(param->params.inquiryResult.addr)[0],
			//		(param->params.inquiryResult.addr)[1],
			//		(param->params.inquiryResult.addr)[2],
			//		(param->params.inquiryResult.addr)[3],
			//		(param->params.inquiryResult.addr)[4],
			//		(param->params.inquiryResult.addr)[5]);
			//APP_DBG("\t %d dB", param->params.inquiryResult.rssi);
			//APP_DBG("\t extLen = %d , ext = %s \n",param->params.inquiryResult.extRespLen, param->params.inquiryResult.extResp);
		}
		break;

		case BT_STACK_EVENT_COMMON_INQUIRY_COMPLETE:
		{
			APP_DBG("BT_STACK_EVENT_COMMON_INQUIRY_COMPLETE\n");
		}
		break;

		case BT_STACK_EVENT_COMMON_INQUIRY_CANCELED:
		{
			APP_DBG("BT_STACK_EVENT_COMMON_INQUIRY_CANCELED\n");
		}
		break;


		case BT_STACK_EVENT_COMMON_MODE_CHANGE:
		{
			APP_DBG("mode - %d, addrs -  %02x:%02x:%02x:%02x:%02x:%02x\n",
					param->params.modeChange.mode,
					(param->params.modeChange.addr)[0],
					(param->params.modeChange.addr)[1],
					(param->params.modeChange.addr)[2],
					(param->params.modeChange.addr)[3],
					(param->params.modeChange.addr)[4],
					(param->params.modeChange.addr)[5]);
			BtLinkModeChanged(param);
		}
		break;

		case BT_STACK_EVENT_COMMON_GET_REMDEV_NAME:
		{
			if(param->params.remDevName.name == NULL)
				break;
			
			APP_DBG("Remote Device:\n");
			APP_DBG("\t%02x:%02x:%02x:%02x:%02x:%02x",
					(param->params.remDevName.addr)[0],
					(param->params.remDevName.addr)[1],
					(param->params.remDevName.addr)[2],
					(param->params.remDevName.addr)[3],
					(param->params.remDevName.addr)[4],
					(param->params.remDevName.addr)[5]);
			BtGetRemoteName(param);
		}
		break;

		case BT_STACK_EVENT_SETTING_ACCESS_MODE:
			BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);
			break;
			
		case BT_STACK_EVENT_COMMON_ACCESS_MODE:
			BtAccessModeUpdate(param->params.accessMode);
			break;

		case BT_STACK_EVENT_COMMON_CONNECTION_ABORTED:
			APP_DBG("BT_STACK_EVENT_COMMON_CONNECTION_ABORTED\n");
			//BtDevConnectionAborted();
			break;

		case BT_STACK_EVENT_COMMON_PAGE_TIME_OUT:
			APP_DBG("BT_STACK_EVENT_COMMON_PAGE_TIME_OUT\n");
			BtLinkPageTimeout(param);
			break;

		case BT_STACK_EVENT_TRUST_LIST_ADD_NEW_RECORD:
		//	APP_DBG("A new device has been added into db : bt address = %02x:%02x:%02x:%02x:%02x:%02x\n",
		//			(param->params.bd_addr)[0],
		//			(param->params.bd_addr)[1],
		//			(param->params.bd_addr)[2],
		//			(param->params.bd_addr)[3],
		//			(param->params.bd_addr)[4],
		//			(param->params.bd_addr)[5]);
			break;

		case BT_STACK_EVENT_TRUST_LIST_DELETE_RECORD:
			break;

		case BT_STACK_EVENT_COMMON_BB_LOST_CONNECTION:
			BtDevConnectionLinkLoss(param);
			break;
        
        case BT_STACK_EVENT_COMMON_LINK_DISCON_LOCAL:
			APP_DBG("BT_STACK_EVENT_COMMON_LINK_DISCON_LOCAL\n");
            if(BtReconnectDevIsExcute())
			    BtStackServiceMsgSend(MSG_BTSTACK_RECONNECT_REMOTE_PAGE_TIMEOUT);
            break;

#ifdef BT_TWS_SUPPORT
		case BT_STACK_EVENT_COMMON_TWS_LOST_CONNECTION:
			if(sys_parameter.bt_TwsBBLostReconnectionEnable)
				BtTwsConnectionLinkLoss();
			break;
		
		case BT_STACK_EVENT_COMMON_TWS_IS_FOUND:
			btManager.remoteNameLen = param->params.inquiryResult.extResp[0] - 1;
			if(btManager.remoteNameLen < BT_NAME_SIZE)
				memcpy(btManager.remoteName, &param->params.inquiryResult.extResp[2], btManager.remoteNameLen);
			else
				memcpy(btManager.remoteName, &param->params.inquiryResult.extResp[2], BT_NAME_SIZE);
			
			APP_DBG("[TWS]tws device is found, connecting: %x:%x:%x:%x:%x:%x		nameLen: %d		name: %s\n", 
			param->params.inquiryResult.addr[0], param->params.inquiryResult.addr[1], param->params.inquiryResult.addr[2], 
			param->params.inquiryResult.addr[3], param->params.inquiryResult.addr[4], param->params.inquiryResult.addr[5],
			btManager.remoteNameLen, &btManager.remoteName[0]);

			BtTwsPairingConnect(param->params.inquiryResult.addr);
			break;

		case BT_STACK_EVENT_COMMON_TWS_PAIRING_STOP:
			if(btManager.twsState != BT_TWS_STATE_CONNECTED)
				BTSetAccessMode(BtAccessModeGeneralAccessible);
			break;
			
		case BT_STACK_EVENT_COMMON_TWS_PAIRING_TIMEOUT:
			APP_DBG("[TWS]Pairing timeout.\n");
			BtTwsPairingTimeout();
			break;
#endif //#ifdef BT_TWS_SUPPORT

		default:
			break;
	}
}

/**************************************************************************************
 **************************************************************************************/
void ClearBtManagerReg(void)
{
	memset(&btManager, 0, sizeof(BT_MANAGER_ST));
}

BT_MANAGER_ST * GetBtManager(void)
{
	return &btManager;
}

/***********************************************************************************
 * 蓝牙协议栈初始化状态
 **********************************************************************************/
void SetBtStackState(BT_STACK_STATE state)
{
	btManager.stackState = state;
}

BT_STACK_STATE GetBtStackState(void)
{
	return btManager.stackState;
}

/***********************************************************************************
 * 蓝牙被连接过滤条件配置
 *
 * Bt device connected profile manage.
 */
void SetBtConnectedProfile(uint8_t index, uint16_t connectedProfile)
{
	btManager.btConnectedProfile |= connectedProfile;

	SetBtCurConnectFlag(1);

	btManager.btLinked_env[index].btLinkedProfile |= connectedProfile;
#if 0
	//只有在1个设备连接上的时候,才会去匹配和更新最后一次配对记录
	if(btManager.linkedNumber>1)
		return;

	//连接到一个蓝牙设备的一个profile时,就更新最后1次连接的蓝牙记录信息
	if(GetNumOfBtConnectedProfile() == 1)
	{
		//配对地址一致,不需要重复保存
		if(memcmp(btManager.btDdbLastAddr, btManager.remoteAddr, 6) == 0)
		{
			if(btManager.btDdbLastProfile & connectedProfile)
				return ;
		}

		btManager.btDdbLastProfile = (uint8_t)btManager.btConnectedProfile;
		//BP10主动回连上次连接的设备，被新的手机连接，发现MAC地址不一致，则停止回连
		if(sys_parameter.bt_ReconnectionEnable && btManager.btReconnectTimer.timerFlag)
		{
			BtStopReconnect();
		}

		memcpy(btManager.btDdbLastAddr, btManager.remoteAddr, 6);
		BtDdb_UpgradeLastBtAddr(btManager.remoteAddr, btManager.btDdbLastProfile);
	}
	else
	{
		if((btManager.btDdbLastProfile & connectedProfile) == 0 )
		{
			btManager.btDdbLastProfile |= connectedProfile;
			BtDdb_UpgradeLastBtProfile(btManager.remoteAddr, btManager.btDdbLastProfile);
		}
	}
#endif
}

void SetBtDisconnectProfile(uint8_t index,uint16_t disconnectProfile)
{
	btManager.btConnectedProfile &= ~disconnectProfile;

	if(!btManager.btConnectedProfile)
	{
		SetBtCurConnectFlag(0);
	}
	btManager.btLinked_env[index].btLinkedProfile &= ~disconnectProfile;
}

 /* param: Addr is the address of remote device
 * return: 0->success, BT host allow connect, other->the reson of fail
 **********************************************************************************/
uint8_t BtConnectDecide(uint8_t *Addr)
{
#if 0	// 客户根据自己的需求实现过滤代码
	uint8_t HPHONE[6] = {0xA6, 0x48, 0x80, 0xFA, 0xA1, 0x30};

	if(memcmp(Addr, HPHONE, 6) == 0)
	{
		APP_DBG("reject the connect request\n");
		return 1;
	}
#endif
	return 0;
}

/***********************************************************************************
 * 蓝牙被连接过滤条件配置
 * param: NULL
 * return: 0->success, allow Master/Slave connect, other->the reson of fail
 **********************************************************************************/
uint8_t BtConnectConfirm(void)
{
#if 0
	if(GetSystemMode() != ModeBtAudioPlay)
	{
		// 假设主机刚发起了回连操作就立马插入SD/U盘，就会出现切换到SLAVE后主副机依然回连组网成功
		// 这个和CFG_TWS_ONLY_IN_BT_MODE的设计理念不相符，所以加上这个判断
		return 1;
	}
#endif
	return 0;
}

/***********************************************************************************
 * @ return : 0=valid; -1=invalid
 **********************************************************************************/
int32_t BtAddrIsValid(uint8_t *addr)
{
	if(((addr[0]==0)
		&&(addr[1]==0)
		&&(addr[2]==0)
		&&(addr[3]==0)
		&&(addr[4]==0)
		&&(addr[5]==0))
		||
		((addr[0]==0xff)
		&&(addr[1]==0xff)
		&&(addr[2]==0xff)
		&&(addr[3]==0xff)
		&&(addr[4]==0xff)
		&&(addr[5]==0xff)))
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

////////////////////////////////////////////////////////////////////
//多链路管理
////////////////////////////////////////////////////////////////////
//获取index
uint8_t bt_link_index_get(uint8_t *addr)
{
	uint8_t ret = 0xff;
	uint8_t i;
	for(i=0; i<BT_LINK_DEV_NUM; i++)
	{
		if(memcmp(addr, btManager.btLinked_env[i].remoteAddr, BT_ADDR_SIZE) == 0)
		{
			return i;
		}
	}
	return ret;
}

uint8_t bt_linked_num_get(void)
{
	return btManager.linkedNumber;
}

/*****************************************************************************************
 *
 ****************************************************************************************/
void BtCurIndex_Set(uint8_t index)
{
#if(BT_LINK_DEV_NUM == 2)
	uint8_t unused_index;
	btManager.cur_index = index;
	btManager.back_index = (btManager.cur_index == 0)? 1 : 0;
	APP_DBG("cur index = %d,back_index = %d\n", btManager.cur_index,btManager.back_index);

	//保存当前的活跃链路信息
	if((btManager.btLinked_env[btManager.cur_index].btLinkState == 1)&&(btManager.btDisConnectingFlag == 0)&&(GetSystemMode() == ModeBtAudioPlay))
	{
		btManager.btDdbLastProfile = btManager.btLinked_env[btManager.cur_index].btLinkedProfile;
		BtDdb_UpgradeLastBtAddr(GetBtManager()->btLinked_env[btManager.cur_index].remoteAddr, btManager.btDdbLastProfile);
		
		memcpy(btManager.btDdbLastAddr, btManager.btLinked_env[btManager.cur_index].remoteAddr, 6);
	}		


	extern void Acl_duration_cancel(uint8_t* addr);
	if(index == 0)
		unused_index = 1;
	else
		unused_index = 0;
	if(btManager.btLinked_env[unused_index].btLinkState)
		Acl_duration_cancel(btManager.btLinked_env[unused_index].remoteAddr);
	if(GetSystemMode() != ModeBtHfPlay)
	{
		extern void Acl_duartion_update(uint8_t* addr,uint32_t duration_plus);
		Acl_duartion_update(btManager.btLinked_env[btManager.cur_index].remoteAddr,2*625);//4*625);//8*625);
	}
	else
	{
		Acl_duration_cancel(btManager.btLinked_env[btManager.cur_index].remoteAddr);
	}
#else
	btManager.cur_index = 0;
#endif
}

uint8_t BtCurIndex_Get(void)
{
#if(BT_LINK_DEV_NUM != 2)
	btManager.cur_index = 0;
#endif
	return btManager.cur_index;
}

uint8_t* BtCurretAddr_Get(void)
{
	return GetBtManager()->btLinked_env[btManager.cur_index].remoteAddr;
}
