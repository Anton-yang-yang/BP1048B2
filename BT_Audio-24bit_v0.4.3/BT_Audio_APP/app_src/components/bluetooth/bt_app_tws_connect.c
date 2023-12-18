/**
 **************************************************************************************
 * @file    bluetooth_tws_connect.c
 * @brief   tws 组网连接相关函数功能接口
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2021-4-18 18:00:00$
 *
 * @Copyright (C) Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include <string.h>
#include "type.h"
#include "app_config.h"
#include "bt_config.h"
//driver
#include "chip_info.h"
#include "debug.h"
//middleware
#include "main_task.h"
#include "bt_manager.h"
#include "bt_tws_api.h"
//application
#include "bt_app_tws_connect.h"
#include "bt_app_ddb_info.h"
#include "bt_stack_service.h"
#include "bt_app_connect.h"
#include "tws_mode.h"


#ifdef BT_TWS_SUPPORT

BtTwsAppCt *gBtTwsAppCt = NULL;
static uint32_t gBtTwsCount = 0;

extern uint32_t gBtTwsDelayConnectCnt;

extern int8_t ME_CancelInquiry(void);


/***********************************************************************************
 * TWS内存申请
 **********************************************************************************/
void BtTwsAppInit(void)
{
	if(gBtTwsAppCt == NULL)
	{
		gBtTwsAppCt = (BtTwsAppCt *)osPortMalloc(sizeof(BtTwsAppCt));
	}

	if(gBtTwsAppCt)
	{
		memset(gBtTwsAppCt, 0, sizeof(BtTwsAppCt));
	}
}

void BtTwsAppDeinit(void)
{
	if(gBtTwsAppCt)
	{
		osPortFree(gBtTwsAppCt);
	}
	gBtTwsAppCt = NULL;
}
/***********************************************************************************
 * tws 主动的连接断开消息
 **********************************************************************************/
void BtTwsConnectApi(void)
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_BTSTACK_TWS_CONNECT;
	MessageSend(GetBtStackServiceMsgHandle(), &msgSend);
}

void BtTwsDisconnectApi(void)
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_BTSTACK_TWS_DISCONNECT;
	MessageSend(GetBtStackServiceMsgHandle(), &msgSend);
}

/***********************************************************************************
 * 
 **********************************************************************************/
void BtTwsDeviceReconnect(void)
{
	if(btManager.twsState == BT_TWS_STATE_NONE)
	{
		if(btManager.twsFlag) 
		{
			APP_DBG("tws device connect\n");
			BtTwsConnectApi();
		}
		else
		{
			APP_DBG("no tws paired list\n");
		}
	}
	else
	{
		APP_DBG("tws is connected\n");
	}
}

/**************************************************************************
 *  断开TWS连接的函数
 *************************************************************************/
void BtTwsDeviceDisconnectExt(void)
{
	if(btManager.twsState == BT_TWS_STATE_CONNECTED)
	{
		APP_DBG("tws disconnect\n");
		tws_link_disconnect();
	}
}

/**************************************************************************
 *  也是断开TWS连接的函数。。。和上面函数的区别就是断开后会自己清除本地的TWS信息，还会发信息让另一台机器也清除TWS信息
 *************************************************************************/
void BtTwsDeviceDisconnect(void)
{
	if(btManager.twsState == BT_TWS_STATE_CONNECTED)
	{
		APP_DBG("tws disconnect\n");
		//tws_link_disconnect();
		BtTwsDisconnectApi();
		if(sys_parameter.bt_TwsConnectedWhenActiveDisconSupport)
		{
			tws_active_disconnection();
			BtDdb_ClrTwsDevInfor();
		}
	}
}

/**************************************************************************
 *  
 *************************************************************************/
void BtTwsLinkLoss(void)
{
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
	if(!btManager.twsSbSlaveDisable)
		tws_slave_simple_pairing_ready();
#elif ((TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM))
	{
		//delay reconnect master device
		gBtTwsAppCt->btTwsEvent |= BT_TWS_EVENT_LINKLOSS;
		gBtTwsAppCt->btTwsLinkLossTimeoutCnt = 25;
	}
#endif
}

/**************************************************************************
 *  
 *************************************************************************/
void CheckBtTwsPairing(void)
{
	if((gBtTwsAppCt->btTwsPairingTimeroutCnt)&&(gBtTwsAppCt->btTwsPairingStart))
	{
		APP_DBG("again...\n");
		gBtTwsAppCt->btTwsEvent |= BT_TWS_EVENT_PAIRING;
		
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
		BtSetAccessMode_Disc_Con();
#endif
	}
	else
	{
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
		BtTwsExitPairingMode();
#endif
	}
}

/**************************************************************************
 *  
 *************************************************************************/
bool BtTwsPairingState(void)
{
	if(gBtTwsAppCt)
		return gBtTwsAppCt->btTwsPairingStart;
	else
		return 0;
}

/**************************************************************************
 *  
 *************************************************************************/
void BtTwsEnterPairingMode(void)
{
	//1.退出回连状态(TWS回连和手机回连)
	BtReconnectDevStop();
	BtReconnectTwsStop();
	
	//2.断开当前已连接的设备
	if(btManager.twsState > BT_TWS_STATE_NONE)
	{
		BtTwsDisconnectApi();
	}
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
	if(btManager.btLinkState)//连接手机后，tws组网只广播
	{
		printf("doublekey pair start!!!!!!!\n");
		extern void tws_start_pairing_doubleKey(void);
		extern uint32_t doubleKeyCnt;
		tws_start_pairing_doubleKey();

		doubleKeyCnt = 30000;//30s 超时
		BTSetAccessMode(BtAccessModeGeneralAccessible);
	}
	else
#endif
	{
		gBtTwsAppCt->btTwsEvent |= BT_TWS_EVENT_PAIRING;
		gBtTwsAppCt->btTwsPairingStart = 1;
		gBtTwsAppCt->btTwsPairingTimeroutCnt = BT_TWS_RECONNECT_TIMEOUT;

		tws_start_pairing_radom();
	}
}

void BtTwsExitPairingMode(void)
{
	tws_stop_pairing_radom();
	if(gBtTwsAppCt->btTwsPairingStart == 0)
		return;
	
	gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_PAIRING;
	gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_REPAIR_CLEAR;
	gBtTwsAppCt->btTwsPairingStart = 0;
	gBtTwsAppCt->btTwsPairingTimeroutCnt = 0;
}

/**************************************************************************
 *  
 *************************************************************************/
void BtTwsPeerEnterPairingMode(void)
{
	//BtDdb_ClearTwsDeviceAddrList();
	btManager.twsRole = BT_TWS_UNKNOW;
}

/**************************************************************************
 *  
 *************************************************************************/
void BtTwsRepairDiscon(void)
{
	if(gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_REPAIR_CLEAR)
		gBtTwsAppCt->btTwsRepairTimerout = 1;
}

/**************************************************************************
 *  peer
 *************************************************************************/
void BtTwsEnterPeerPairingMode(void)
{
	APP_DBG("BtTwsEnterPeerPairingMode\n");
	gBtTwsAppCt->btTwsEvent |= BT_TWS_EVENT_PAIRING;
	gBtTwsAppCt->btTwsPairingStart = 1;
	gBtTwsAppCt->btTwsPairingTimeroutCnt = BT_TWS_RECONNECT_TIMEOUT;

	BtReconnectDevStop();
}

/**************************************************************************
 *  
 *************************************************************************/
void BtTwsExitPeerPairingMode(void)
{
	if(gBtTwsAppCt->btTwsPairingStart == 0)
		return;

	APP_DBG("BtTwsExitPeerPairingMode\n");
	gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_PAIRING;
	gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_REPAIR_CLEAR;
	gBtTwsAppCt->btTwsPairingStart = 0;
	gBtTwsAppCt->btTwsPairingTimeroutCnt = 0;

	BTInquiryCancel();
}

/**************************************************************************
 *  soundbar
 *************************************************************************/
void BtTwsEnterSimplePairingMode(void)
{
	APP_DBG("enter tws pairing mode...\n");
	//清除TWS组网配对记录
	//BtDdb_ClearTwsDeviceAddrList();

	//断开已经连接的TWS组网
	if(btManager.twsState > BT_TWS_STATE_NONE)
	{
		gBtTwsAppCt->btTwsEvent |= BT_TWS_EVENT_PAIRING_READY;
		//tws_link_disconnect();
		BtTwsDisconnectApi();
	}

	gBtTwsAppCt->btTwsEvent |= BT_TWS_EVENT_SIMPLE_PAIRING;
	gBtTwsAppCt->btTwsPairingStart = 1;
	gBtTwsAppCt->btTwsPairingTimeroutCnt = BT_TWS_RECONNECT_TIMEOUT;

	btManager.twsRole = BT_TWS_UNKNOW;
	btManager.twsEnterPairingFlag = 1;
	
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
	if(((gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_PAIRING_READY)==0)&&(!btManager.twsSbSlaveDisable))
		tws_slave_start_simple_pairing();
#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
	if((gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_PAIRING_READY)==0)
		ble_advertisement_data_update();
#endif
}

/**************************************************************************
 *  
 *************************************************************************/
void BtTwsExitSimplePairingMode(void)
{
	//停止广播
	gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_SIMPLE_PAIRING;
	gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_PAIRING_READY;

	gBtTwsAppCt->btTwsPairingStart = 0;
	gBtTwsAppCt->btTwsPairingTimeroutCnt = 0;
	btManager.twsEnterPairingFlag = 0;
	
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
	if((btManager.twsState == BT_TWS_STATE_NONE)&&(btManager.btLinkState == 0))
		tws_slave_stop_simple_pairing();
#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
	if((btManager.twsState == BT_TWS_STATE_NONE))
		ble_advertisement_data_update();
#endif
}

/**************************************************************************
 *  
 *************************************************************************/
void BtTwsRunLoop(void)
{
	//20ms
	gBtTwsCount++;
	if(gBtTwsCount<20)
		return;

	if(gBtTwsDelayConnectCnt)
	{
		gBtTwsDelayConnectCnt--;
		if(gBtTwsDelayConnectCnt==0)
		{
			if((gBtTwsAppCt->btTwsPairingStart)&&(gBtTwsAppCt->btTwsPairingTimeroutCnt))
			{
				gBtTwsAppCt->btTwsEvent |= BT_TWS_EVENT_PAIRING;
				printf("pairing again...\n");
			}
		}
	}

	gBtTwsCount=0;
	//配对连接相关处理
	if(gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_PAIRING)
	{
		if(gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_REPAIR_CLEAR)
		{
			if(btManager.twsState != BT_TWS_STATE_NONE)
			{
				gBtTwsAppCt->btTwsRepairTimerout++;
				if(gBtTwsAppCt->btTwsRepairTimerout >= 20)
				{
					gBtTwsAppCt->btTwsRepairTimerout = 0;
					gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_REPAIR_CLEAR;
					//tws_link_disconnect();
					BtTwsDisconnectApi();
				}
			}
			else
			{
				gBtTwsAppCt->btTwsRepairTimerout = 0;
				gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_REPAIR_CLEAR;
			}
		}
		else
		{
			if(btManager.twsState == BT_TWS_STATE_NONE)
			{
			#if (TWS_PAIRING_MODE != CFG_TWS_PEER_SLAVE)
				//BtDdb_ClearTwsDeviceAddrList();
			#endif
				APP_DBG("tws pairing start\n");
				btManager.twsRole = BT_TWS_UNKNOW;
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
				tws_start_pairing(TWS_ROLE_RANDOM);
				if(btManager.btLinkState == 1)
				{
					APP_DBG("NoDisc_NoCon\n");
					BtSetAccessMode_NoDisc_NoCon();
				}
				else
				{
					APP_DBG("Disc_Con\n");
					BtSetAccessMode_Disc_Con();
				}
#elif (TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)
				tws_start_pairing(TWS_ROLE_MASTER);
				if(btManager.btLinkState == 1)
				{
					APP_DBG("NoDisc_NoCon\n");
					BtSetAccessMode_NoDisc_NoCon();
				}
				else
				{
					APP_DBG("Disc_Con\n");
					BtSetAccessMode_Disc_Con();
				}
#elif (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
				tws_start_pairing(TWS_ROLE_SLAVE);
				BtSetAccessMode_NoDisc_Con();
#endif
				gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_PAIRING;
			}
		}
	}
	if(gBtTwsAppCt->btTwsPairingTimeroutCnt)
	{
		gBtTwsAppCt->btTwsPairingTimeroutCnt--;

#if (TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)
		if(gBtTwsAppCt->btTwsPairingTimeroutCnt == 0)
			BtTwsExitPeerPairingMode();
#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
		if(gBtTwsAppCt->btTwsPairingTimeroutCnt == 0)
			BtTwsExitPairingMode();
#endif
	}

//only master and slave
	if(gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_PAIRING_READY)
	{
		if((btManager.twsState == BT_TWS_STATE_NONE)&&(btManager.btLinkState == 0))
		{
			gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_PAIRING_READY;
	#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
			{
				ble_advertisement_data_update();
			}
	#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
			{
				if(!btManager.twsSbSlaveDisable)
					tws_slave_start_simple_pairing();
			}
	#endif
		}
	}
	
	if(gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_SIMPLE_PAIRING)
	{
		if((gBtTwsAppCt->btTwsPairingTimeroutCnt == 0)||((btManager.twsState == BT_TWS_STATE_CONNECTED)&&((gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_PAIRING_READY)==0)))
		{
			APP_DBG("auto exit tws simple pairing\n");
			//gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_SIMPLE_PAIRING;

			BtTwsExitSimplePairingMode();
		}
	}

	if(sys_parameter.bt_TwsBBLostReconnectionEnable)
	{
		//linkloss处理
		if(gBtTwsAppCt->btTwsLinkLossTimeoutCnt)
			gBtTwsAppCt->btTwsLinkLossTimeoutCnt--;
		if((gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_LINKLOSS)&&(gBtTwsAppCt->btTwsLinkLossTimeoutCnt == 0))
		{
			gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_LINKLOSS;
			BtStartReconnectTws(sys_parameter.bt_TwsBBLostTryCounts, sys_parameter.bt_TwsBBLostInternalTime);
		}
	}

	//slave开机回连超时后进入可被搜索可被连接状态
	if(gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_SLAVE_TIMEOUT)
	{
		//30s timeout
		if(gBtTwsAppCt->btTwsSlaveTimeoutCount)
			gBtTwsAppCt->btTwsSlaveTimeoutCount--;
		
		if(gBtTwsAppCt->btTwsSlaveTimeoutCount == 0)
		{
			gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_SLAVE_TIMEOUT;
			//gBtTwsAppCt->btTwsSlaveTimeoutCount = 0;

			if((btManager.twsState == BT_TWS_STATE_NONE)&&(btManager.btLinkState == 0))
			{
				//TWS回连超时,进入可被搜索可被连接状态
				BtSetAccessMode_Disc_Con();
			}
		}
	}
}

/**************************************************************************
 *  
 *************************************************************************/
extern uint8_t BB_link_state_get(uint8_t type);
extern uint8_t tws_connect_state_get(void);
bool TwsPeerSlave(void)
{
	if(GetSystemMode() == ModeTwsSlavePlay)
	{					
		if(tws_connect_state_get() == TWS_PAIR_PAIRING)
		{
			return TRUE;
		}
		
		if(gBtTwsAppCt->btTwsPairingStart == 1)
		{
			APP_DBG("CFG_TWS_PEER_SLAVE: Exit pairing loop\n");
			extern unsigned char gLmpCmdStart;
			BtTwsExitPeerPairingMode();
			if(gLmpCmdStart == 0)
			{
				gLmpCmdStart = 1;
			}
			if(tws_connect_state_get() == TWS_PAIR_INQUIRY)
			{
				ME_CancelInquiry();
			}
		}
		TwsSlaveModeExit();
		return TRUE;
	}
	else
	{	
		if(!btManager.btLinkState)
		{
			if(btManager.twsState == BT_TWS_STATE_NONE)
			{
				BtSetAccessMode_Disc_Con();
				TwsSlaveModeEnter();
				BtTwsEnterPeerPairingMode();
				return TRUE;
			}
		}
		
		if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		{
			tws_link_disconnect();
		}
	}

	return FALSE;
}

bool TwsRoleSlave(void)
{
	if(!btManager.twsSbSlaveDisable)
	{
		//1.断开TWS
		if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		{
			tws_link_disconnect();
		}
		//2.关闭BLE SCAN
		tws_slave_stop_simple_pairing();
		tws_slave_simple_pairing_end();
		btManager.twsSbSlaveDisable = 1;
	}
	else
	{
		btManager.twsSbSlaveDisable = 0;
		//1.开启BLE SCAN
		tws_slave_start_simple_pairing();
	}

	return FALSE;
}

void BtTwsPairingStart(void)
{
	//取消回连手机和TWS的流程
	BtReconnectTwsStop();
	BtReconnectDevStop();

#if (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
	//对箱从模式
	if (TRUE == TwsPeerSlave())
	{
		return;
	}
#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
	//soundbar从机
	TwsRoleSlave();
#else  // 对箱主机   soundbar主机     双键组网

	if(gBtTwsAppCt->btTwsEvent || gBtTwsAppCt->btTwsPairingStart)
		return;
	
	if(BB_link_state_get(1)||btManager.btTwsPairingStartDelayCnt)
	{
		btManager.btTwsPairingStartDelayCnt = 1;
		return;
	}

	//当前连接则断开组网
	if(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)
	{
		BtTwsDeviceDisconnect();
		return;
	}

	//非蓝牙模式不可按键组网
	if(GetSystemMode() != ModeBtAudioPlay)
		return;
	
	//蓝牙连接不可组网
	if(!sys_parameter.bt_TwsPairingWhenPhoneConnectedSupport && btManager.btLinkState)
	{
		return;
	}

/*#if (TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)
    //用于取消正在发起的组网行为；  用户有需求可打开
	if(gBtTwsAppCt->btTwsPairingStart == 1)
	{
		APP_DBG("CFG_TWS_PEER_MASTER: Exit pairing loop\n");
		GetBtManager()->twsStopConnect = 1;
		extern unsigned char gLmpCmdStart;
		BtTwsExitPeerPairingMode();
		if(gLmpCmdStart == 0)
		{
			gLmpCmdStart = 1;
		}
		else
		{
			ME_CancelInquiry();
		}
		return;
	}
#endif*/

	//打开蓝牙可见性
	BtSetAccessMode_Disc_Con();

#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
	BtTwsEnterPairingMode();
#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
	BtTwsEnterSimplePairingMode();
#elif (TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)
	BtTwsEnterPeerPairingMode();
#endif

#endif
}

/**************************************************************************
 *  Note: 此函数是在main_task中进行处理;
 *        在操作蓝牙时,需要注意,不能直接调用控制函数,避免重入问题
 *************************************************************************/
void tws_msg_process(uint16_t msg)
{
	switch(msg)
	{
#ifdef BT_TWS_SUPPORT
		case MSG_BT_TWS_MASTER_CONNECTED:
		case MSG_BT_TWS_SLAVE_CONNECTED:
			break;

		//发起TWS组网
		case MSG_BT_TWS_PAIRING:
			BtStackServiceMsgSend(MSG_BT_STACK_TWS_PAIRING_START);//bkd change
			break;

		case MSG_BT_TWS_RECONNECT:
			BtTwsDeviceReconnect();
			break;
		
		case MSG_BT_TWS_DISCONNECT:
			BtTwsDeviceDisconnect();
			break;

		case MSG_BT_TWS_CLEAR_PAIRED_LIST:
			BtDdb_ClearTwsDeviceAddrList();
			break;

		case MSG_BT_CLEAR_PAIRED_LIST:
			memset(btManager.btLinkDeviceInfo,0,sizeof(btManager.btLinkDeviceInfo));
			BtDdb_EraseBtLinkInforMsg();
			break;

#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
		//Soundbar Slave进入可以校频偏状态
		//校准完频偏后,需要重启系统
		case MSG_BT_SOUNDBAR_SLAVE_TEST_MODE:
			if(btManager.twsState == BT_TWS_STATE_NONE)
			{
				if(!btManager.twsSoundbarSlaveTestFlag)
				{
					btManager.twsSoundbarSlaveTestFlag = 1;
					APP_DBG("Soundbar Slave Enter Test State\n");
					BtSetAccessMode_Disc_Con();
				}
				else
				{
					btManager.twsSoundbarSlaveTestFlag = 0;
					APP_DBG("Soundbar Slave Exit Test State\n");
					BtSetAccessMode_NoDisc_Con();
				}
			}
			break;
#endif
#endif
#ifdef BT_TWS_SUPPORT
		case MSG_BT_SNIFF:
	#if((TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE))
			if(GetBtManager()->twsState > BT_TWS_STATE_NONE)
			{
				if(tws_audio_state_get() == TWS_DISCONNECT)
					break;
			}
			if(GetBtManager()->twsState == BT_TWS_STATE_NONE)
			{
				sniff_lmpsend_set(0);
				MessageContext		msgSend;
				msgSend.msgId		= MSG_DEEPSLEEP;
				MessageSend(GetMainMessageHandle(), &msgSend);
				break;
			}
				
			if(sniff_lmpsend_get() == 0)
			{
				sniff_lmpsend_set(1);
				if(GetBtManager()->twsState > BT_TWS_STATE_NONE)
				{
					if(GetBtManager()->twsRole == BT_TWS_MASTER)
					{
						//if(sniff_lmpsend_get() == 0)
						{
						//发起sniff请求，主从停止TWS传输，主从都会进入 tws_stop_callback().
							//sniff_lmpsend_set(1);
							tws_stop_transfer();
						}
					}
					else
					{
						tws_slave_send_cmd_sniff();
					}

				}
				else
				{
					sniff_lmpsend_set(0);
					MessageContext		msgSend;
					msgSend.msgId		= MSG_DEEPSLEEP;
					MessageSend(GetMainMessageHandle(), &msgSend);
				}
			}
		break;
	#else
//		BtStackServiceMsgSend(MSG_BT_STACK_TWS_PAIRING_STOP);

//		if(btManager.twsState == BT_TWS_STATE_CONNECTED)
//			BtStackServiceMsgSend(MSG_BT_STACK_TWS_SYNC_POWERDOWN);
		
		{
			MessageContext		msgSend;
			msgSend.msgId		= MSG_DEEPSLEEP;
			MessageSend(GetMainMessageHandle(), &msgSend);
		}
		break;
	#endif
#endif

	}
}
#else
bool BtTwsPairingState(void)
{
	return 0;
}
#endif


