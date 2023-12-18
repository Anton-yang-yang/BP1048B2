/**
 **************************************************************************************
 * @file    bt_stack_service.c
 * @brief   
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2018-2-9 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
//#include "soft_watch_dog.h"
#include <string.h>
#include "type.h"
#include "app_config.h"
#include "gpio.h" //for BOARD
#include "debug.h"
#include "rtos_api.h"
#include "app_message.h"
#include "uarts.h"
#include "uarts_interface.h"
#include "dma.h"
#include "timeout.h"
#include "irqn.h"
#include "bt_config.h"
#include "bt_app_init.h"
#include "bt_stack_service.h"
#include "bt_stack_memory.h"
#include "bt_play_mode.h"
#include "bt_play_api.h"
#include "bt_hf_mode.h"
#include "bt_common_api.h"
#include "bt_avrcp_api.h"
#include "bt_manager.h"
#include "main_task.h"
#include "bt_interface.h"
#include "audio_core_service.h"
#include "bb_api.h"
#include "clk.h"
#include "reset.h"
#include "remind_sound.h"
#include "bt_app_ddb_info.h"
#include "bt_app_connect.h"
#include "bt_app_common.h"
#include "bt_app_tws_connect.h"
#include "bt_app_tws.h"
#include "bt_app_avrcp_deal.h"
#include "backup.h"
#include "ble_app_func.h"
#include "bt_em_config.h"

#ifdef CFG_APP_BT_MODE_EN

static uint8_t gBtHostStackMemHeap[BT_STACK_MEM_SIZE];

//BR/EDR STACK SERVICE
#ifdef BT_TWS_SUPPORT

#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
const char gBtNameNull[] = {' '};
void TwsDoubleKeyPairTimeOut(void);
#endif

#define BT_STACK_SERVICE_STACK_SIZE		1024
#define BT_STACK_SERVICE_PRIO			4
#else
#define BT_STACK_SERVICE_STACK_SIZE		640//768
#define BT_STACK_SERVICE_PRIO			4
#endif
#define BT_STACK_NUM_MESSAGE_QUEUE		20

typedef struct _BtStackServiceContext
{
	xTaskHandle			taskHandle;
	MessageHandle		msgHandle;
//	TaskState			serviceState;

	uint8_t				serviceWaitResume;	//1:蓝牙不在后台运行时,开启通话,退出播放模式,不能kill蓝牙协议栈

	uint8_t				bbErrorMode;
	uint32_t			bbErrorType;

//	uint32_t			btEnterSniffStep;
//	uint32_t			btExitSniffReconPhone;
}BtStackServiceContext;

//蓝牙服务sniff 服务task。
#ifdef BT_SNIFF_ENABLE

typedef struct _BtUserServiceContext
{
	xTaskHandle			taskHandle;
	MessageHandle		msgHandle;
	TaskState			serviceState;

}BtUserServiceContext;

#endif	//BT_SNIFF_ENABLE

static BtStackServiceContext	*btStackServiceCt = NULL;
BT_CONFIGURATION_PARAMS		*btStackConfigParams = NULL;

#ifdef TWS_CODE_BACKUP
uint32_t g_tws_need_init;
#endif
extern uint8_t tws_slave_cap;

static void BtRstStateCheck(void);
extern int8_t ME_CancelInquiry(void);
#ifdef CFG_FUNC_OPEN_SLOW_DEVICE_TASK
extern void SlowDevice_MsgSend(uint16_t msgId);
#endif


/***********************************************************************************
 *
 **********************************************************************************/
uint32_t gDebugCnt = 0;
void DebugDisplayTaskInf(void)
{
	uint8_t *buf= pvPortMalloc(4096);
	memset(buf,0,4096);
	vTaskList((char*)buf);
	DBG("\nTask           State  Prio+3  FreeStack(word)  PID\n***************************************************\r\n");
	DBG("%s\n",buf);
	vPortFree(buf);
}

/***********************************************************************************
 *
 **********************************************************************************/
void BtTwsPowerDownProcess(void)
{
	MessageContext		msgSend;

	msgSend.msgId = MSG_DEEPSLEEP;
	MessageSend(GetMainMessageHandle(), &msgSend);
}

/***********************************************************************************
 * 蓝牙断开连接流程
 **********************************************************************************/
extern FUNC_BT_DISCONNECT_PROCESS BtDisconnectProcess;
void BtStack_BtDisconnectProcess(void)
{
	static uint32_t btDisconnectCnt = 0;

	btDisconnectCnt++;
	if(btDisconnectCnt>=200) //200ms
	{
		btDisconnectCnt=0;
		if(btManager.linkedNumber)
		{
			BT_DBG("LINK[0] Discon\n");
			BTDisconnect(0);
			BT_DBG("LINK[1] Discon\n");
			BTDisconnect(1);
			return;
		}
		else
		{
			//注销
			BtAppiFunc_BtDisconnectProcess(NULL);
		}
	}
}

/***********************************************************************************
 * 蓝牙AVRCP快速连接状态确认
 **********************************************************************************/
extern FUNC_BT_AVRCP_CON_PROCESS BtAvrcpConProcess;
static uint32_t btAvrcpConIndex = 0;
void BtStack_BtAvrcpConProcess(void)
{
	static uint32_t btAvrcpConCnt = 0;

	btAvrcpConCnt++;
	if(btAvrcpConCnt>=200) //200ms
	{
		btAvrcpConCnt=0;
		if((btManager.btLinked_env[btAvrcpConIndex].a2dpState >= BT_A2DP_STATE_CONNECTED)
			&&(btManager.btLinked_env[btAvrcpConIndex].avrcpState != BT_AVRCP_STATE_CONNECTED))
		{
			BtAvrcpConnect(btAvrcpConIndex, btManager.btLinked_env[btAvrcpConIndex].remoteAddr);
		}
		//else
		{
			//注销
			BtAppiFunc_BtAvrcpConProcess(NULL);
		}
	}
}

void BtStack_BtAvrcpConRegister(uint8_t index)
{
	if(btAvrcpConIndex < BT_LINK_DEV_NUM)
	{
		btAvrcpConIndex = index;
		BtAppiFunc_BtAvrcpConProcess(BtStack_BtAvrcpConProcess);
	}
}


/***********************************************************************************
 * 蓝牙测试盒校准频偏完成回调函数
 **********************************************************************************/
void BtFreqOffsetAdjustComplete(unsigned char offset)
{
#ifndef CFG_FUNC_OPEN_SLOW_DEVICE_TASK
	int8_t ret = 0;
#endif
	APP_DBG("++++++[BT_OFFSET]  offset:0x%x ++++++\n", offset);

	btManager.btLastAddrUpgradeIgnored = 1;

	//判断是否和当前默认值一致,不一致更新保存到flash
	if(offset != btStackConfigParams->bt_trimValue)
	{
		btStackConfigParams->bt_ConfigHeader[0]='M';
		btStackConfigParams->bt_ConfigHeader[1]='V';
		btStackConfigParams->bt_ConfigHeader[2]='B';
		btStackConfigParams->bt_ConfigHeader[3]='T';
		
		btStackConfigParams->bt_trimValue = offset;
#ifdef CFG_FUNC_OPEN_SLOW_DEVICE_TASK
		SlowDevice_MsgSend(MSG_DEVICE_BT_FREQ_OFFSET_WRITE);
#else
		//save to flash
		ret = BtDdb_SaveBtConfigurationParams(btStackConfigParams);
		
		if(ret)
			APP_DBG("[BT_OFFSET]update Error!!!\n");
		else
			APP_DBG("$$$[BT_OFFSET] update $$$\n");
#endif
	}

	//清除所有的配对记录
	BtDdb_EraseBtLinkInforMsg();
}

/***********************************************************************************
 * 蓝牙middleware的消息传递入口函数
 **********************************************************************************/
void BtMidMessageManage(BtMidMessageId messageId, uint8_t Param)
{
	MessageContext		msgSend;
	MessageHandle 		msgHandle;

	switch(messageId)
	{
		case MSG_BT_MID_UART_RX_INT:
			//msgHandle = GetBtStackServiceMsgHandle();
			//msgSend.msgId = MSG_BTSTACK_RX_INT;
			//MessageSend(msgHandle, &msgSend);
			BtStackServiceMsgSend(MSG_BTSTACK_RX_INT);
			break;

		case MSG_BT_MID_ACCESS_MODE_IDLE:
			if(sys_parameter.bt_ReconnectionEnable)
				BtReconnectDevice();
			break;

		case MSG_BT_MID_STACK_INIT:
			{
				//APP_DBG("MSG_BT_MID_STACK_INIT\n");
				//此处配置协议栈初始化完成后，是否进入到蓝牙可被搜索可被连接状态;
				//1=进入到可被搜索可被连接状态;  0=进入到不可被搜索不可被连接状态
				#ifdef POWER_ON_BT_ACCESS_MODE_SET
				GetBtManager()->btAccessModeEnable = 0;
				#else
				GetBtManager()->btAccessModeEnable = 1;
				#endif
				
#ifdef BT_TWS_SUPPORT
			#if (CFG_TWS_ONLY_IN_BT_MODE == DISABLE)
				if(GetBtManager()->twsFlag)
					GetBtManager()->btConStateProtectCnt = 1;
				else
			#endif
					GetBtManager()->btConStateProtectCnt = 0;
#endif
			}
			break;

		case MSG_BT_MID_STATE_CONNECTED:
			{
				MessageContext		msgSend;
				msgSend.msgId		= MSG_BT_STATE_CONNECTED;
				MessageSend(GetMainMessageHandle(), &msgSend);
				
				DBG("BtMidMessageManage: MSG_BT_MID_STATE_CONNECTED\n");

				//SetBtPlayState(BT_PLAYER_STATE_STOP);
#if(BT_LINK_DEV_NUM == 2)
				if((btManager.btLinked_env[0].a2dpState != BT_A2DP_STATE_STREAMING)
					&&(btManager.btLinked_env[1].a2dpState != BT_A2DP_STATE_STREAMING)
					)
#endif
				SetBtPlayState(BT_PLAYER_STATE_STOP);

				BtStackServiceMsgSend(MSG_BTSTACK_RECONNECT_REMOTE_SUCCESS);
			#ifdef TWS_CODE_BACKUP// defined(BT_TWS_SUPPORT) && defined(CFG_FUNC_REMIND_SOUND_EN)
				{
					if(g_tws_need_init == 0 && GetBtManager()->twsState == BT_TWS_STATE_CONNECTED && GetBtManager()->twsRole == BT_TWS_MASTER)
					{
						g_tws_need_init = 1;
					}
				}
			#endif
			}
			break;
		
		case MSG_BT_MID_STATE_DISCONNECT:
			{
				SoftFlagRegister(SoftFlagDiscDelayMask);
				DBG("BtMidMessageManage: MSG_BT_MID_STATE_DISCONNECT\n");

				//SetBtPlayState(BT_PLAYER_STATE_STOP);
#if(BT_LINK_DEV_NUM == 2)
				if((btManager.btLinked_env[0].a2dpState != BT_A2DP_STATE_STREAMING)
					&&(btManager.btLinked_env[1].a2dpState != BT_A2DP_STATE_STREAMING)
					)
#endif
				SetBtPlayState(BT_PLAYER_STATE_STOP);
			}
			break;

//////////////////////////////////////////////////////////////////////////////////////////////////
//AVRCP
		case MSG_BT_MID_PLAY_STATE_CHANGE:
			if((Param == BT_PLAYER_STATE_PLAYING)&&(GetA2dpState(BtCurIndex_Get()) == BT_A2DP_STATE_STREAMING))
			{
				msgHandle = GetMainMessageHandle();
				msgSend.msgId		= MSG_BT_A2DP_STREAMING;
				MessageSend(msgHandle, &msgSend);
			}
			
			if(GetBtPlayState() != Param)
				SetBtPlayState(Param);
			
			break;

		case MSG_BT_MID_VOLUME_CHANGE:		
#if (BT_AVRCP_VOLUME_SYNC == ENABLE)
			SetBtSyncVolume(Param);
			// Send message to bt play mode
			msgSend.msgId		= MSG_BT_PLAY_SYNC_VOLUME_CHANGED;
			MessageSend(GetMainMessageHandle(), &msgSend);
#endif
			break;

//////////////////////////////////////////////////////////////////////////////////////////////////
//HFP
#if (BT_HFP_SUPPORT == ENABLE)
		case MSG_BT_MID_HFP_TASK_RESUME:
			BtHfModeRunningResume();
			break;

		//通话数据格式更新
		case MSG_BT_MID_HFP_CODEC_TYPE_UPDATE:
			BtHfCodecTypeUpdate(Param);
			break;

#ifdef CFG_FUNC_REMIND_SOUND_EN
		//通话模式下呼入电话播放来电提示音
		case MSG_BT_MID_HFP_PLAY_REMIND:
			msgHandle = GetBtHfMessageHandle();
			if(msgHandle == NULL)
				break;		
			// Send message to bt play mode
			msgSend.msgId		= MSG_BT_HF_MODE_REMIND_PLAY;
			MessageSend(msgHandle, &msgSend);
			break;
		
		//通话模式下停止播放提示音
		case MSG_BT_MID_HFP_PLAY_REMIND_END:
			msgHandle = GetBtHfMessageHandle();
			if(msgHandle == NULL)
				break;
			RemindSoundClearPlay();
			break;
#endif

#endif
//////////////////////////////////////////////////////////////////////////////////////////////////
		default:
			break;
	}
}

/***********************************************************************************
 * 定时检测和处理蓝牙相关事件
 **********************************************************************************/
static void CheckBtEventTimer(void)
{
#ifdef BT_TWS_SUPPORT
	if(sys_parameter.bt_TwsConnectedWhenActiveDisconSupport && tws_active_disconnection_get() == 1)
	{
		BtDdb_ClrTwsDevInfor();	
		tws_active_disconnection_set(0);
	}


	if(btManager.twsStopConnect == 1)
	{
		BtTwsExitPeerPairingMode();
		ME_CancelInquiry();	
		btManager.twsStopConnect = 0;
	}
#endif

	//获取蓝牙播放状态
	if(btManager.avrcpPlayStatusTimer.timerFlag)
	{
		if(IsTimeOut(&btManager.avrcpPlayStatusTimer.timerHandle))
		{
			BT_A2DP_STATE state = GetA2dpState(BtCurIndex_Get());
			if(state == BT_A2DP_STATE_STREAMING)
			{
				BTCtrlGetPlayStatus(BtCurIndex_Get());
				TimerStart_BtPlayStatus();
			}
			else
			{
				TimerStop_BtPlayStatus();
			}
		}
	}

	if(sys_parameter.bt_ReconnectionEnable)
	{
		BtReconnectProcess();

		if(GetBtManager()->btReconnectDelayCount)
		{
			GetBtManager()->btReconnectDelayCount++;
			if((GetBtManager()->btReconnectDelayCount>200)//&&(btManager.btReconTwsSt.ConnectionTimer.timerFlag == TIMER_UNUSED)
	#ifdef CFG_FUNC_REMIND_SOUND_EN
				&&(RemindSoundIsPlay() <= 1 )
	#endif
				)
			{
				GetBtManager()->btReconnectDelayCount = 0;
				BtReconnectDevice();
			}
		}
	}

#ifdef RECON_ADD
	if(GetBtManager()->btReconnectunusual && btManager.btReconPhoneSt.ConnectionTimer.timerFlag && (GetSystemMode() == ModeBtAudioPlay))
	{
		APP_DBG("--- btReconnectunusual need recon again!\n");
		GetBtManager()->btReconnectunusual = FALSE;
		extern void BtReconnectDevExcute(void);
		BtReconnectDevExcute();
	}
	else{
		GetBtManager()->btReconnectunusual = FALSE;
	}
#endif

	BtScanPageStateCheck();

	BtRstStateCheck();

#ifdef	BT_SNIFF_ENABLE
	BtStartEnterSniffStep();
	BtExitSniffReconnectPhone();
#endif

}

/**************************************************************************
 *
 **************************************************************************/
/**
 * @brief	Get message receive handle of bt stack manager
 * @param	NONE
 * @return	MessageHandle
 */
xTaskHandle GetBtStackServiceTaskHandle(void)
{
	if(!btStackServiceCt)
		return NULL;
	
	return btStackServiceCt->taskHandle;
}

uint8_t GetBtStackServiceTaskPrio(void)
{
	return BT_STACK_SERVICE_PRIO;
}

MessageHandle GetBtStackServiceMsgHandle(void)
{
	if(!btStackServiceCt)
		return NULL;
	
	return btStackServiceCt->msgHandle;
}

void BtStackServiceMsgSend(uint16_t msgId)
{
	MessageContext		msgSend;
	msgSend.msgId = msgId;
	if(btStackServiceCt)
		MessageSend(btStackServiceCt->msgHandle, &msgSend);
}

/***********************************************************************************
 * 处理蓝牙异常状态
 **********************************************************************************/
/*static void CheckBtErrorState(void)
{
	btEventListCount++;
	if(btCheckEventList)
	{
		//当接收到BT_STACK_EVENT_COMMON_CONNECTION_ABORTED事件,当前链路出现异常,会主动断开手机,需要主动回连手机,并自动播放歌曲
		//超时时间为30s
		if((btCheckEventList&BT_EVENT_L2CAP_LINK_DISCONNECT)&&(btEventListCount == btEventListB1Count))
		{
			APP_DBG("[btCheckEventList]: BT_EVENT_L2CAP_LINK_DISCONNECT\n");
			
			btCheckEventList &= ~BT_EVENT_L2CAP_LINK_DISCONNECT;
			btEventListB1Count = 0;
		}
	}
}
*/
/***********************************************************************************
 * 蓝牙时钟设置
 * 此函数lib中调用，客户可根据自己条件配置
 **********************************************************************************/
void BtCntClkSet(void)
{
	//btclk freq set
//	BACKUP_32KEnable(OSC32K_SOURCE);
//	sniff_cntclk_set(1);//sniff cnt clk 32768 Hz default not use
//_____________________________________________

#ifndef BT_SNIFF_RC_CLK
	//HOSC 32K
	Clock_BTDMClkSelect(OSC_32K_MODE);
	Clock_OSC32KClkSelect(LOSC_32K_MODE);
	Clock_32KClkDivSet(Clock_OSCClkDivGet());  //如果这里出现问题，请检查SystemClockInit，不要在这里直接改——Tony
	Clock_BBCtrlHOSCInDeepsleep(0);//禁止baseband进入sniff后硬件自动关闭HOSC 24M
#else
	//RC
	sniff_rc_init_set();//Rc 初始化参数
	//RC 32K
	Clock_BTDMClkSelect(RC_CLK32_MODE);//select rc_clk_32k
	Clock_32KClkDivSet(750);     //set osc_clk_32k = 24M/32K=750

	Clock_RcCntWindowSet(63);//64-1  --  32K/64 = 500

	Clock_RC32KClkDivSet( Clock_RcFreqGet(1) / ((uint32_t)(32*1000)) );
	Clock_RcFreqAutoCntStart();

	Clock_BBCtrlHOSCInDeepsleep(1);//Deepsleep时,BB接管HOSC
#endif
}

/***********************************************************************************
 * 蓝牙协议栈消息处理
 **********************************************************************************/
static void BtStackMsgProcess(uint16_t msgId)
{
	MessageContext		msgSend;
	MessageHandle 		msgHandle;
	msgHandle = GetMainMessageHandle();
	switch(msgId)
	{
		case MSG_BTSTACK_BB_ERROR:
			{
				msgSend.msgId = MSG_BTSTACK_BB_ERROR;

				MessageSend(msgHandle, &msgSend);

				if(btStackServiceCt->bbErrorMode == 1)
				{
					APP_DBG("BT ERROR:0x%lx\n", btStackServiceCt->bbErrorType);
				}
				else if(btStackServiceCt->bbErrorMode == 2)
				{
					APP_DBG("BLE ERROR:0x%lx\n", btStackServiceCt->bbErrorType);
				}
                else if(btStackServiceCt->bbErrorMode == 3)
                {
					APP_DBG("BB MSG ERROR:0x%lx\n", btStackServiceCt->bbErrorType);
                }

				BtReconnectDevStop();
#ifdef BT_TWS_SUPPORT
				BtReconnectTwsStop();
#endif
			}
			break;

#if defined(BT_TWS_SUPPORT) && (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
		case MSG_BTSTACK_BROADCAST_NAME_UPDATE:
			{
				extern void BtSetDeviceName(char *name, uint8_t len);
					//蓝牙名称下发到协议栈
				if(btManager.btLinked_env[0].btLinkState && btManager.twsState != BT_TWS_STATE_CONNECTED)
				{
					BtSetDeviceName((char *)gBtNameNull, 1);
				}
				else
				{
					BtSetDeviceName(btStackConfigParams->bt_LocalDeviceName, strlen(btStackConfigParams->bt_LocalDeviceName));
				}
			}
			break;
#endif

		case MSG_BTSTACK_BB_ERROR_RESTART:
			//蓝牙协议栈任务启动后,发起回连
#ifdef BT_TWS_SUPPORT
			APP_DBG("[BT_STACK_APP]:MSG_BTSTACK_BB_ERROR_RESTART, reconnect tws device\n");
			BtReconnectTwsCreate(btManager.btTwsDeviceAddr, sys_parameter.bt_TwsReconnectionTryCounts, sys_parameter.bt_TwsReconnectionInternalTime, 1000, BT_PROFILE_SUPPORTED_TWS);
#endif
			if(IsBtAudioMode())
			{
				APP_DBG("[BT_STACK_APP]:MSG_BTSTACK_BB_ERROR_RESTART, reconnect remote device\n");
				BtReconnectDevCreate(btManager.btDdbLastAddr, sys_parameter.bt_ReconnectionTryCounts, sys_parameter.bt_ReconnectionInternalTime, 0, btManager.btDdbLastProfile);
			}
			break;

		case MSG_BTSTACK_LOCAL_DEVICE_NAME_UPDATE:
			{
				extern void BtSetDeviceName(char *name, uint8_t len);
				//蓝牙名称下发到协议栈
				BtSetDeviceName(sys_parameter.bt_LocalDeviceName, strlen(sys_parameter.bt_LocalDeviceName));
				
#ifdef CFG_FUNC_OPEN_SLOW_DEVICE_TASK
				SlowDevice_MsgSend(MSG_DEVICE_BT_NAME_WRITE);
#endif
			}
			break;

#ifdef BT_TWS_SUPPORT
		case MSG_BTSTACK_TWS_CONNECT:
			APP_DBG("[BT_STACK_APP]:tws connect: %02x:%02x:%02x:%02x:%02x:%02x\n", 
				btManager.btTwsDeviceAddr[0],
				btManager.btTwsDeviceAddr[1],
				btManager.btTwsDeviceAddr[2],
				btManager.btTwsDeviceAddr[3],
				btManager.btTwsDeviceAddr[4],
				btManager.btTwsDeviceAddr[5]
				);
			
			if((!btManager.btTwsDeviceAddr[0])&&(!btManager.btTwsDeviceAddr[1])&&(!btManager.btTwsDeviceAddr[2])&&
				(!btManager.btTwsDeviceAddr[3])&&(!btManager.btTwsDeviceAddr[4])&&(!btManager.btTwsDeviceAddr[5])
				)
			{
				APP_DBG("addr error!!!\n");
				break;
			}
			
			tws_connect(btManager.btTwsDeviceAddr);
			break;
			
		case MSG_BTSTACK_TWS_DISCONNECT:
			APP_DBG("[BT_STACK_APP]:tws disconnect\n");
			tws_link_disconnect();
			break;

		case MSG_BT_STACK_TWS_PAIRING_START:			
#if (BT_LINK_DEV_NUM == 2)
			if(btManager.btLinked_env[0].btLinkState && btManager.btLinked_env[1].btLinkState)//连接了2台手机 禁止进行组网
			{
				APP_DBG("break : btLinked_env[0].btLinkState = 1, btLinked_env[1].btLinkState = 1\n");
				break;
			}
#endif
			APP_DBG("[BT_STACK_APP]:tws pairing start\n");
			BtTwsPairingStart();
			break;

		case MSG_BT_STACK_TWS_PAIRING_STOP:
			BtTwsExitPeerPairingMode();
			break;

		case MSG_BT_STACK_TWS_PAIRING_START_RESUME:
			if(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED ||gBtTwsAppCt->btTwsPairingStart == 1)
			{
				APP_DBG("tws connecting or connected \n");
				break;
			}
			BtTwsPairingStart();
			break;

		case MSG_BT_STACK_TWS_SYNC_POWERDOWN:
			if(btManager.twsState == BT_TWS_STATE_CONNECTED)
			{
				tws_sync_powerdown_cmd();
				btManager.twsState = BT_TWS_STATE_NONE;
			}
			break;
#endif
		case MSG_BTSTACK_RECONNECT_REMOTE_SUCCESS:
			APP_DBG("[BT_STACK_MSG]:MSG_BTSTACK_RECONNECT_REMOTE_SUCCESS\n");

			if(btManager.btTwsPairingStartDelayCnt)
			{
				btManager.btTwsPairingStartDelayCnt = 0;
				BtStackServiceMsgSend(MSG_BT_STACK_TWS_PAIRING_START_RESUME);
			}
			break;

		case MSG_BTSTACK_RECONNECT_REMOTE_PROFILE:
			APP_DBG("[BT_STACK_MSG]:MSG_BTSTACK_RECONNECT_REMOTE_PROFILE\n");
			if(btManager.btReconExcuteSt == (&btManager.btReconPhoneSt))
			{
				//蓝牙处于回连等待期,假如手机有主动连接BP10,则需要继续等待;
				if((btManager.btReconExcuteSt->ConnectionTimer.timerFlag & TIMER_WAITING) == 0)
				{
					APP_DBG("restart connect other profiles.\n");
					btManager.btReconExcuteSt->ConnectionTimer.timerFlag |= TIMER_STARTED;
				}
			}
			break;

		case MSG_BTSTACK_RECONNECT_REMOTE_STOP:
			APP_DBG("[BT_STACK_MSG]:MSG_BTSTACK_RECONNECT_REMOTE_STOP\n");
			BtReconnectDevStop();
			break;

		case MSG_BTSTACK_RECONNECT_REMOTE_PAGE_TIMEOUT:
			APP_DBG("[BT_STACK_MSG]:MSG_BTSTACK_RECONNECT_REMOTE_PAGE_TIMEOUT\n");
			if(btManager.btReconExcuteSt == (&btManager.btReconPhoneSt))
			{
				btManager.btReconExcuteSt->ConnectionTimer.timerFlag = TIMER_UNUSED;
			}

			if(btManager.btTwsPairingStartDelayCnt)
			{
				btManager.btTwsPairingStartDelayCnt = 0;
				BtStackServiceMsgSend(MSG_BT_STACK_TWS_PAIRING_START_RESUME);
			}
			break;
#ifdef BT_TWS_SUPPORT
		case MSG_BTSTACK_RECONNECT_TWS_STOP:
			APP_DBG("[BT_STACK_MSG]:MSG_BTSTACK_RECONNECT_TWS_STOP\n");
			BtReconnectTwsStop();
			break;
		
		case MSG_BTSTACK_RECONNECT_TWS_PAGE_TIMEOUT:
			APP_DBG("[BT_STACK_MSG]:MSG_BTSTACK_RECONNECT_TWS_PAGE_TIMEOUT\n");
			if(btManager.btReconExcuteSt == (&btManager.btReconTwsSt))
			{
				btManager.btReconExcuteSt->ConnectionTimer.timerFlag = TIMER_UNUSED;
			}
			break;
#endif
		case MSG_BTSTACK_RUN_START:
			APP_DBG("[BT_STACK_MSG]:MSG_BTSTACK_RUN_START\n");
			//BtMidMessageSend(MSG_BT_MID_STATE_FAST_ENABLE, 0);
			BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);
			if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_DISABLE)
				BtScanPageStateSet(BT_SCAN_PAGE_STATE_OPENING);
#ifdef BT_TWS_SUPPORT
			if(btManager.twsFlag)
			{
				#if (CFG_TWS_ONLY_IN_BT_MODE == ENABLE)
				if (IsBtAudioMode())
				{
					if(btManager.twsRole == BT_TWS_SLAVE)
					{
						BtReconnectTws_Slave();
					}
					else 
					{
						BtReconnectTws();
					}	
				}
				#elif (TWS_PAIRING_MODE != CFG_TWS_PEER_SLAVE)
				if(btManager.twsRole == BT_TWS_SLAVE)
				{
					BtReconnectTws_Slave();
				}
				else 
				{
					BtReconnectTws();
				}
				#endif
			}
#endif
			break;

			case MSG_BTSTACK_ACCESS_MODE_SET:
			APP_DBG("[BT_STACK_MSG]:MSG_BTSTACK_ACCESS_MODE_SET\n");
			BtAccessModeSetting();
			break;
///////////////////////////////////////////////////////////////////
		//COMMON
		case MSG_BTSTACK_MSG_BT_CONNECT_DEV_CTRL:
			{
				extern uint8_t BB_link_state_get(uint8_t type);
				BT_DBG("bt msg: connect dev");
				
				if(BB_link_state_get(1) == 0)
				{
					BtConnectCtrl();

					/*//没有连接记录则不打开延时标志
					if(BtAddrIsValid(btManager.btDdbLastAddr))
						break;

					BT_DBG(" start\n");
					btManager.btTwsPairingStartDelayCnt = 1;
					*/
				}
				else
				{
					BT_DBG(" busy...\n");
				}
			}
			break;

		case MSG_BTSTACK_MSG_BT_DISCONNECT_DEV_CTRL:
			BT_DBG("bt msg: disconnect dev\n");
			
			//BTHciDisconnectCmd(btManager.btDdbLastAddr);
			BtDisconnectCtrl();
			
			//btManager.btTwsPairingStartDelayCnt = 0;//bkd add
			break;

		//AVRCP CMD
		case MSG_BTSTACK_MSG_BT_PLAY:
			BT_DBG("bt msg: play\n");
			BTCtrlPlay(BtCurIndex_Get());
			break;
		
		case MSG_BTSTACK_MSG_BT_PAUSE:
			BT_DBG("bt msg: pause\n");
			BTCtrlPause(BtCurIndex_Get());
			break;

		case MSG_BTSTACK_MSG_BT_NEXT:
			BT_DBG("bt msg: next\n");
			BTCtrlNext(BtCurIndex_Get());
			break;
		
		case MSG_BTSTACK_MSG_BT_PREV:
			BT_DBG("bt msg: prev\n");
			BTCtrlPrev(BtCurIndex_Get());
			break;

		case MSG_BTSTACK_MSG_BT_FF_START:
			BT_DBG("bt msg: ff start\n");
			BTCtrlFF(BtCurIndex_Get());
			break;
		
		case MSG_BTSTACK_MSG_BT_FF_END:
			BT_DBG("bt msg: ff end\n");
			BTCtrlEndFF(BtCurIndex_Get());
			break;
		
		case MSG_BTSTACK_MSG_BT_FB_START:
			BT_DBG("bt msg: fb start\n");
			BTCtrlFB(BtCurIndex_Get());
			break;
		
		case MSG_BTSTACK_MSG_BT_FB_END:
			BT_DBG("bt msg: fb end\n");
			BTCtrlEndFB(BtCurIndex_Get());
			break;

#if (BT_AVRCP_VOLUME_SYNC == ENABLE)
		case MSG_BTSTACK_MSG_BT_VOLUME_SET:
			{
				uint8_t VolumePercent = BtLocalVolLevel2AbsVolme(GetBtSyncVolume());
				BT_DBG("bt msg: volume set %d\n", VolumePercent);
				BTCtrlSetVol(BtCurIndex_Get(), VolumePercent);
			}
			break;
#endif

#if (BT_HFP_SUPPORT == ENABLE)
		//HFP CMD
		case MSG_BTSTACK_MSG_BT_REDIAL:
			BT_DBG("bt msg: hf redial\n");
			HfpRedialNumber(BtCurIndex_Get());
			break;

		case MSG_BTSTACK_MSG_BT_OPEN_VOICE_RECONGNITION:
			BT_DBG("bt msg: hf open voice recognition\n");
			OpenBtHfpVoiceRecognitionFunc(BtCurIndex_Get());
			break;
		
		case MSG_BTSTACK_MSG_BT_CLOSE_VOICE_RECONGNITION:
			BT_DBG("bt msg: hf close voice recognition\n");
			HfpVoiceRecognition(BtCurIndex_Get(),0);
			break;

		case MSG_BTSTACK_MSG_BT_ANSWER_CALL:
			BT_DBG("bt msg: hf answer call\n");
			HfpAnswerCall(BtCurIndex_Get());
			break;

		case MSG_BTSTACK_MSG_BT_HANGUP:
			BT_DBG("bt msg: hf hang up\n");
			HfpHangup(BtCurIndex_Get());
			break;

		case MSG_BTSTACK_MSG_BT_HF_AUDIO_TRANSFER:
			BT_DBG("bt msg: hf audio transfer\n");
			HfpAudioTransfer(BtCurIndex_Get());
			break;

		case MSG_BTSTACK_MSG_BT_HF_CALL_HOLDCUR_ANSWER_CALL:
			BT_DBG("bt msg: hf call hold: hold current call and answer another call\n");
			HfpCallHold(BtCurIndex_Get(),HF_HOLD_HOLD_ACTIVE_CALLS);
			break;
		
		case MSG_BTSTACK_MSG_BT_HF_CALL_HANGUP_ANSWER_CALL:
			BT_DBG("bt msg: hf call hold: hangup current call and answer another call\n");
			HfpCallHold(BtCurIndex_Get(),HF_HOLD_RELEASE_ACTIVE_CALLS);
			break;
		
		case MSG_BTSTACK_MSG_BT_HF_CALL_HANGUP_ANOTHER_CALL:
			BT_DBG("bt msg: hf call hold: hangup another call \n");
			HfpCallHold(BtCurIndex_Get(),HF_HOLD_RELEASE_HELD_CALLS);
			break;

#ifdef BT_HFP_BATTERY_SYNC
		case MSG_BTSTACK_MSG_BT_HF_SET_BATTERY:
			BT_DBG("bt msg: set battery level:%d\n", btManager.HfpBatLevel);
			HfpSetBatteryState(BtCurIndex_Get(),btManager.HfpBatLevel,0);
			break;
#endif

		case MSG_BTSTACK_MSG_BT_HF_VOLUME_SET:
			//SetBtHfSyncVolume(mainAppCt.HfVolume);
			HfpSpeakerVolume(BtCurIndex_Get(),btManager.volGain);
			break;

		case MSG_BTSTACK_MSG_BT_HF_GET_CUR_CALLNUMBER:
			//BT_DBG("MSG_BTSTACK_MSG_BT_HF_GET_CUR_CALLNUMBER : %d\n", BtCurIndex_Get());
			HfpGetCurrentCalls(BtCurIndex_Get());
			break;

		case MSG_BTSTACK_MSG_BT_HF_DISABLE_NREC:
			BT_DBG("MSG_BTSTACK_MSG_BT_HF_DISABLE_NREC : %d\n", BtCurIndex_Get());
			HfpDisableNREC(BtCurIndex_Get());
			break;

		case MSG_BTSTACK_MSG_BT_HF_SCO_DISCONNECT:
			BT_DBG("MSG_BTSTACK_MSG_BT_HF_SCO_DISCONNECT : %d\n", btManager.hfpScoDiscIndex);
			HfpAudioDisconnect(btManager.hfpScoDiscIndex);
			break;

		case MSG_BTSTACK_MSG_BT_HF_SCO_CONNECT:
			BT_DBG("MSG_BTSTACK_MSG_BT_HF_SCO_CONNECT : %d\n", BtCurIndex_Get());
			HfpAudioConnect(BtCurIndex_Get());
			break;
#endif

#ifdef BT_TWS_SUPPORT
		//TWS
		case MSG_BTSTACK_MSG_BT_TWS_PAIRING_CANCEL:
#if (TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)
			BtTwsExitPeerPairingMode();
#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
			BtTwsExitPairingMode();
#endif
			break;
#endif
	}
}

/***********************************************************************************
 * 蓝牙协议栈任务处理
 **********************************************************************************/
static void BtStackServiceEntrance(void * param)
{
	BtBbParams			bbParams;
	MessageContext		msgRecv;
	extern uint32_t DebugBBLmLink;
	
	SetBluetoothMode(BLE_SUPPORT|(BT_SUPPORT<<1));
	
	DebugBBLmLink = 1;	//默认开启lm debug

	//load bt stack all params
	LoadBtConfigurationParams();
	
	//BB init
	ConfigBtBbParams(&bbParams);
	
	Bluetooth_common_init(&bbParams);
#if ( BT_SUPPORT == ENABLE )
	Bt_init((void*)&bbParams);
	tws_slave_cap = bbParams.freqTrim;
#endif

#if (BLE_SUPPORT == ENABLE)
	Ble_init();
#endif

	//host memory init
	SetBtPlatformInterface(&pfiOS, &pfiBtDdb);

#if (CFG_TWS_ONLY_IN_BT_MODE == ENABLE)
#if (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
	Other_confirm_Callback_Set(NULL);
#else
	Other_confirm_Callback_Set(BtConnectConfirm);
#endif
#endif
	Name_confirm_Callback_Set(BtConnectDecide);

	//在蓝牙开启后台运行时,host的内存采用数组,避免存在申请/释放带来碎片化的风险
	BTStackMemAlloc(BT_STACK_MEM_SIZE, gBtHostStackMemHeap, 0);

	APP_DBG("BtStackServiceEntrance.\n");
	
#if ( BT_SUPPORT == ENABLE )
	//BR/EDR init
	if(!BtStackInit())
	{
		APP_DBG("error init bt device\n");
		//出现初始化异常时,蓝牙协议栈任务挂起
		while(1)
		{
			MessageRecv(btStackServiceCt->msgHandle, &msgRecv, 0xFFFFFFFF);
		}
	}
	else
	{
		APP_DBG("bt device init success!\n");
	}
#endif

#if (BLE_SUPPORT == ENABLE) 
	//BLE init
	{
		extern void BleAdvSet(void);
		//extern void SetBleLog(uint8_t log);
		//SetBleLog(0xff);
		InitBlePlaycontrolProfile();
		
		if(!InitBleStack(&g_playcontrol_app_context, &g_playcontrol_profile))
		{
			APP_DBG("error ble stack init\n");
		}

		BleAdvSet(); 
		APP_DBG("ble stack init success\n");
	}
#endif

	SetBtStackState(BT_STACK_STATE_READY);
	//BtMidMessageSend(MSG_BT_MID_STATE_FAST_ENABLE, 0);
	if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_DISABLE)
		BtStackServiceMsgSend(MSG_BTSTACK_RUN_START);
	while(1)
	{
		MessageRecv(btStackServiceCt->msgHandle, &msgRecv, 1);
		
#if BT_HFP_SUPPORT
		extern void BtModeEnterDetect(void);
		BtModeEnterDetect();
#endif
#ifdef SOFT_WACTH_DOG_ENABLE
		little_dog_feed(DOG_INDEX3_BtStackTask);
#endif

		if(btManager.hfpScoDiscDelay)
		{
			btManager.hfpScoDiscDelay--;
			if(btManager.hfpScoDiscDelay == 0)
			{
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_HF_SCO_DISCONNECT);
			}
		}


		if(msgRecv.msgId)
			BtStackMsgProcess(msgRecv.msgId);
		
		rw_main();
		BTStackRun();
		BtEventFlagProcess();//bt事件处理

#ifdef BT_TWS_SUPPORT
		if(GetSystemMode() != ModeBtHfPlay)
		{
			tws_audio_state_sync();
		}
		BtTwsRunLoop();

		#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
		TwsDoubleKeyPairTimeOut();
		#endif
#endif
		if(GetAudioCoreServiceState() == TaskStatePaused)
		{
			MessageContext		msgSend;
			msgSend.msgId		= MSG_NONE;
			MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);
		}

#ifdef BT_HFP_MODE_DISABLE
		extern uint32_t gHfpCallNoneWaiting;
		if(gHfpCallNoneWaiting)
		{
			gHfpCallNoneWaiting++;
			if(gHfpCallNoneWaiting>=1000) //delay 1000ms
			{
				gHfpCallNoneWaiting=0;
//				APP_DBG("HfpCallNoneWaiting end\n");
			}
		}
#endif

		if(BtDisconnectProcess)
			BtDisconnectProcess();

		if(BtAvrcpConProcess)
			BtAvrcpConProcess();

		if(BtScoSendProcess)
			BtScoSendProcess();

		//通话过程中执行获取手机通话状态流程
		if(BtHfpGetCurCallStateProcess)
			BtHfpGetCurCallStateProcess();

		if(btManager.btConStateProtectCnt)
		{
			btManager.btConStateProtectCnt++;
			if(btManager.btConStateProtectCnt>=10000)
			{
				btManager.btConStateProtectCnt=0;
				APP_DBG("bt connect state protect end...\n");
			}
		}

		CheckBtEventTimer();
#ifdef	TWS_CODE_BACKUP
#ifdef BT_TWS_SUPPORT
		if(g_tws_need_init == 1)
		{
			if(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)
			{
				#ifdef CFG_FUNC_REMIND_SOUND_EN			
				if(RemindSoundIsPlay() <= 1 ) //没有播放提示音 
				#endif				
				{
					APP_DBG("tws_audio_init_sync...\n");

					
					g_tws_need_init = 2;
					tws_audio_init_sync();
				}
			}
		}
#endif
		//先去掉同步标志位，会导致无声问题
		g_tws_need_init = 0;
#endif //TWS_CODE_BACKUP

		extern uint32_t a2dp_pause_delay_cnt;
		if(a2dp_pause_delay_cnt)
		{
			a2dp_pause_delay_cnt--;
			if(a2dp_pause_delay_cnt == 0)
			{
				printf("a2dp_pause_delay_cnt  \n");
				uint8_t cfg_index = (btManager.cur_index == 0)? 1 : 0;
				if((GetA2dpState(cfg_index) == BT_A2DP_STATE_STREAMING) && (GetSystemMode() == ModeBtHfPlay))
				{
					printf("pause....%d...  \n", cfg_index);
					AvrcpCtrlPause(cfg_index);
				}
			}
		}
	}
}

/************************************************************************************
 * @brief	Start bluetooth stack service initial.
 * @param	NONE
 * @return	
 ***********************************************************************************/
static bool BtStackServiceInit(void)
{
	APP_DBG("bluetooth stack service init.\n");

	btStackServiceCt = (BtStackServiceContext*)osPortMalloc(sizeof(BtStackServiceContext));
	if(btStackServiceCt == NULL)
	{
		return FALSE;
	}
	memset(btStackServiceCt, 0, sizeof(BtStackServiceContext));
	
	btStackConfigParams = (BT_CONFIGURATION_PARAMS*)osPortMalloc(sizeof(BT_CONFIGURATION_PARAMS));
	if(btStackConfigParams == NULL)
	{
		return FALSE;
	}
	memset(btStackConfigParams, 0, sizeof(BT_CONFIGURATION_PARAMS));

	btStackServiceCt->msgHandle = MessageRegister(BT_STACK_NUM_MESSAGE_QUEUE);
	if(btStackServiceCt->msgHandle == NULL)
	{
		return FALSE;
	}

	//register bt middleware message send interface
	BtAppiFunc_MessageSend(BtMidMessageManage);

#ifdef BT_TWS_SUPPORT
	BtTwsAppInit();
#endif

	return TRUE;
}

/************************************************************************************
 * @brief	Start bluetooth stack service.
 * @param	NONE
 * @return	
 ***********************************************************************************/
bool BtStackServiceStart(void)
{
	bool		ret = TRUE;

	if((btStackServiceCt) && (btStackServiceCt->serviceWaitResume))
	{
		btStackServiceCt->serviceWaitResume = 0;
		return ret;
	}

#ifdef	BT_SNIFF_RC_CLK
	sniff_clk_set(0);//sniff use rc
#endif

	memset((uint8_t*)BB_EM_MAP_ADDR, 0, BB_EM_SIZE);//clear em erea
	
	SoftFlagDeregister(SoftFlagDiscDelayMask);
	ClearBtManagerReg();

	SetBtStackState(BT_STACK_STATE_INITAILIZING);
	
	ret = BtStackServiceInit();
	if(ret)
	{
		btStackServiceCt->taskHandle = NULL;

		xTaskCreate(BtStackServiceEntrance, 
					"BtStack", 
					BT_STACK_SERVICE_STACK_SIZE, 
					NULL, 
					BT_STACK_SERVICE_PRIO, 
					&btStackServiceCt->taskHandle);
		if(btStackServiceCt->taskHandle == NULL)
		{
			ret = FALSE;
		}
#ifdef SOFT_WACTH_DOG_ENABLE
		little_dog_adopt(DOG_INDEX3_BtStackTask);
#endif
	}
	if(!ret)
		APP_DBG("BtStack service create fail!\n");
	return ret;
}

/************************************************************************************
 * @brief	Kill bluetooth stack service.
 * @param	NONE
 * @return	
 ***********************************************************************************/
bool BtStackServiceKill(void)
{
#if ( BT_SUPPORT == ENABLE )
	int32_t ret = 0;
	if(btStackServiceCt == NULL)
	{
		return FALSE;
	}
	
	//btStackService
	//Msgbox
	if(btStackServiceCt->msgHandle)
	{
		MessageDeregister(btStackServiceCt->msgHandle);
		btStackServiceCt->msgHandle = NULL;
	}
	
	//task
	if(btStackServiceCt->taskHandle)
	{
		vTaskDelete(btStackServiceCt->taskHandle);
		btStackServiceCt->taskHandle = NULL;
	}

#ifdef BT_TWS_SUPPORT
	BtTwsAppDeinit();
#endif

	//deregister bt middleware message send interface
	BtAppiFunc_MessageSend(NULL);

	//stack deinit
	ret = BtStackUninit();
	if(!ret)
	{
		APP_DBG("Bt Stack Uninit fail!!!\n");
		return FALSE;
	}

	if(btStackConfigParams)
	{
		osPortFree(btStackConfigParams);
		btStackConfigParams = NULL;
	}
	//
	if(btStackServiceCt)
	{
		osPortFree(btStackServiceCt);
		btStackServiceCt = NULL;
	}
	APP_DBG("!!btStackServiceCt\n");
	
#ifdef SOFT_WACTH_DOG_ENABLE
	little_dog_deadopt(DOG_INDEX3_BtStackTask);
#endif

#endif
	return TRUE;
}

/***********************************************************************************
 * 
 **********************************************************************************/
void BtStackServiceWaitResume(void)
{
	btStackServiceCt->serviceWaitResume = 1;
}

void BtStackServiceWaitClear(void)
{
	btStackServiceCt->serviceWaitResume = 0;
}

/***********************************************************************************
 * BB错误报告
 * 注:需要判断当前是否在中断中，需要调用不同的消息发送函数接口
 **********************************************************************************/
void BBMatchReport(void)
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_BTSTACK_BB_ERROR;
	MessageSend(mainAppCt.msgHandle, &msgSend);

    APP_DBG("bb match or ke malloc fail\n");
}

void BBErrorReport(uint8_t mode, uint32_t errorType)
{
	MessageContext		msgSend;
	MessageHandle 		msgHandle;
	if(btStackServiceCt == NULL)
		return;
	
	msgHandle = btStackServiceCt->msgHandle;
	msgSend.msgId = MSG_BTSTACK_BB_ERROR;

	btStackServiceCt->bbErrorMode = mode;
	btStackServiceCt->bbErrorType = errorType;

	//isr
	MessageSendFromISR(msgHandle, &msgSend);
}

/***********************************************************************************
 * BB 中断关闭
 **********************************************************************************/
void BT_IntDisable(void)
{
	NVIC_DisableIRQ(18);//BT_Interrupt =18
	NVIC_DisableIRQ(19);//BLE_Interrupt =19
}

/***********************************************************************************
 * BB 模块关闭
 **********************************************************************************/
void BT_ModuleClose(void)
{
	Reset_RegisterReset(MDM_REG_SEPA);
	Reset_FunctionReset(BTDM_FUNC_SEPA|MDM_FUNC_SEPA|RF_FUNC_SEPA);
	Clock_Module2Disable(ALL_MODULE2_CLK_SWITCH); //close clock
}


/***********************************************************************************
 * 
 **********************************************************************************/
uint8_t GetBtStackCt(void)
{
	if(btStackServiceCt)
		return 1;
	else
		return 0;
}

/***********************************************************************************
 * 蓝牙恢复出厂设置
 **********************************************************************************/
static void BtRstStateCheck(void)
{
	switch(btManager.btRstState)
	{
		case BT_RST_STATE_NONE:
			break;
			
		case BT_RST_STATE_START:
			APP_DBG("bt reset start\n");
			// If there is a reconnectiong process, stop it
			if(sys_parameter.bt_ReconnectionEnable)
				BtReconnectDevStop();
#ifdef BT_TWS_SUPPORT
			if(sys_parameter.bt_ReconnectionEnable && sys_parameter.bt_TwsReconnectionEnable)
				BtReconnectTwsStop();
#endif
			// If there is a bt link, disconnect it
			if(GetBtCurConnectFlag())
			{

				BTDisconnect(0);
				BTDisconnect(1);

			}

			btManager.btRstState = BT_RST_STATE_WAITING;
			btManager.btRstWaitingCount = 0;
			break;
			
		case BT_RST_STATE_WAITING:
			if(btManager.btRstWaitingCount>=3000)
			{
				btManager.btRstWaitingCount = 2000;
				if(GetBtManager()->btConnectedProfile)
				{

					BTDisconnect(0);
					BTDisconnect(1);

				}
				else if(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_ALL)
				{
					btManager.btRstState = BT_RST_STATE_FINISHED;
				}
			}
			else
			{
				btManager.btRstWaitingCount++;
			}
			break;
			
		case BT_RST_STATE_FINISHED:
			btManager.btRstWaitingCount = 0;
			memset(btManager.remoteAddr, 0, 6);
			memset(btManager.btDdbLastAddr, 0, 6);
			
			BtDdb_EraseBtLinkInforMsg();
			
			btManager.btRstState = BT_RST_STATE_NONE;
			APP_DBG("bt reset complete\n");
			break;
			
		default:
			btManager.btRstState = BT_RST_STATE_NONE;
			break;
	}
}

/***********************************************************************************
 * 开关蓝牙
 * 断开蓝牙连接，删除蓝牙协议栈任务，关闭蓝牙晶振
 **********************************************************************************/
void BtPowerOff(void)
{
	uint8_t btDisconnectTimeout = 0;
	if(!btStackServiceCt)
		return;
	
	APP_DBG("[Func]:Bt off\n");
	
	if(GetBtStackState() == BT_STACK_STATE_INITAILIZING)
	{
		while(GetBtStackState() == BT_STACK_STATE_INITAILIZING)
		{
			vTaskDelay(10);
			btDisconnectTimeout++;
			if(btDisconnectTimeout>=100)
				break;
		}

		//快速在BT模式和其他模式(共2个模式)切换，需要delay(500);避免蓝牙初始化和反初始化状态未完成导致的错误
		//vTaskDelay(500);
		vTaskDelay(50);
	}
	
	if(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_NONE)
	{

		BTDisconnect(0);
		BTDisconnect(1);

	}

	//在蓝牙回连时,需要先取消蓝牙回连行为
	BtReconnectDevStop();
	vTaskDelay(50);

	//wait for bt disconnect, 2S timeout
	while(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_NONE)
	{
		vTaskDelay(10);
		btDisconnectTimeout++;
		if(btDisconnectTimeout>=200)
			break;
	}
	
	//bb reset
	rwip_reset();
	BT_IntDisable();
	//Kill bt stack service
	BtStackServiceKill();
	vTaskDelay(10);
	//reset bt module and close bt clock
	BT_ModuleClose();
}

void BtPowerOn(void)
{
	APP_DBG("[Func]:Bt on\n");
	vTaskDelay(50);
	Clock_Module2Enable(ALL_MODULE2_CLK_SWITCH);
	vTaskDelay(50);

	//bt stack restart
	BtStackServiceStart();
}


/***********************************************************************************
 * 进入蓝牙DUT模式
 * 退出DUT模式后,最好重启系统
 **********************************************************************************/
void BtEnterDutModeFunc(void)
{
	uint8_t btDisconnectTimeout = 0;
	if(!GetBtStackCt())
	{
		APP_DBG("Enter dut mode fail\n");
		return;
	}

	if(!btManager.btDutModeEnable)
	{
		btManager.btDutModeEnable = 1;
		
		if(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_NONE)
		{

			BTDisconnect(0);
			BTDisconnect(1);

		}
		if(sys_parameter.bt_ReconnectionEnable)
			BtReconnectDevStop();
		
		APP_DBG("confirm bt disconnect\n");
		while(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_NONE)
		{
			//2s timeout
			vTaskDelay(100);
			btDisconnectTimeout++;
			if(btDisconnectTimeout>=50)
				break;
		}

		APP_DBG("clear all pairing list\n");
		BtDdb_EraseBtLinkInforMsg();
		
		APP_DBG("[Enter dut mode]\n");
		BTEnterDutMode();
	}
}


/***********************************************************************************
 * 快速开关蓝牙
 * 断开蓝牙连接，蓝牙进入不可被搜索，不可被连接状态
 **********************************************************************************/
void BtFastPowerOff(void)
{
	BtScanPageStateSet(BT_SCAN_PAGE_STATE_CLOSING);
}

void BtFastPowerOn(void)
{
	if(btStackServiceCt->serviceWaitResume)
	{
		//从机从tws slave模式退出,无连接手机,则开启可被搜索可被连接
		if(!btManager.btLinkState)
		{
			BtSetAccessMode_Disc_Con();
		}
		
		btStackServiceCt->serviceWaitResume = 0;
		return;
	}

	if(btManager.btExitSniffReconPhone)
	{
		BtScanPageStateSet(BT_SCAN_PAGE_STATE_OPEN_WAITING);
	}
	else
	{
		BtScanPageStateSet(BT_SCAN_PAGE_STATE_OPENING);
	}
}


/***********************************************************************************
 * 蓝牙名称的更新
 **********************************************************************************/
void BtLocalDeviceNameUpdate(char *deviceName)
{
	MessageContext		msgSend;
	MessageHandle 		msgHandle;
	if(btStackServiceCt == NULL)
		return;
	
	memset(sys_parameter.bt_LocalDeviceName, 0, BT_NAME_SIZE);
	memcpy(sys_parameter.bt_LocalDeviceName, deviceName, strlen(deviceName));
	
	msgHandle = btStackServiceCt->msgHandle;
	msgSend.msgId = MSG_BTSTACK_LOCAL_DEVICE_NAME_UPDATE;

	MessageSend(msgHandle, &msgSend);
}

#if defined(BT_TWS_SUPPORT) && (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
/***********************************************************************************
 * 广播的蓝牙名称的更新
 **********************************************************************************/
void BtBroadcastNameUpdate(void)
{
	MessageContext		msgSend;
	MessageHandle 		msgHandle;
	if(btStackServiceCt == NULL)
		return;

	msgHandle = btStackServiceCt->msgHandle;
	msgSend.msgId = MSG_BTSTACK_BROADCAST_NAME_UPDATE;
	MessageSend(msgHandle, &msgSend);
}
void TwsDoubleKeyPairTimeOut(void)
{
	extern uint32_t doubleKeyCnt;
	extern void tws_stop_pairing_doubleKey(void);
	if(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)
	{
		doubleKeyCnt = 0;
	}
	if(doubleKeyCnt==1)
	{
		doubleKeyCnt = 0;
		tws_stop_pairing_doubleKey();
		BtBroadcastNameUpdate();//20230519
		printf("tws doublekey pairing stop!\n");
	}
}
#endif

#else

void BBErrorReport(uint8_t mode, uint32_t errorType)
{
}

void BBMatchReport(void)
{
}

void BtFreqOffsetAdjustComplete(unsigned char offset)
{
}

#endif

