/**
 **************************************************************************************
 * @file    bluetooth_tws_connect.h
 * @brief	tws相关功能函数接口
 *
 * @author  kk
 * @version V1.0.0
 *
 * $Created: 2019-10-28 18:00:00$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __BT_TWS_APP_FUNC_H__
#define __BT_TWS_APP_FUNC_H__

#include "type.h"
#include "bt_manager.h"

#define BT_TWS_RECONNECT_TIMEOUT		50*30 //(基于20ms处理一次,大概30s超时)


typedef uint32_t BT_TWS_EVENT_BIT;

#define BT_TWS_EVENT_NONE				0x00
#define BT_TWS_EVENT_PAIRING			0x01	//重新配对
#define BT_TWS_EVENT_SLAVE_TIMEOUT		0x02	//开机slave超时,允许进入可被搜索可被连接状态
#define BT_TWS_EVENT_LINKLOSS			0x04	//连接丢失,slave回连
#define BT_TWS_EVENT_SIMPLE_PAIRING		0x08	//开启TWS简易配对
#define BT_TWS_EVENT_PAIRING_READY		0x10	//开启TWS配对前准备工作(等待TWS断开)
#define BT_TWS_EVENT_REPAIR_CLEAR		0x20	//重新配对,等待双方情况配对

typedef struct _BtTwsAppCt
{
	BT_TWS_EVENT_BIT	btTwsEvent;

	//配对超时寄存器
	uint32_t			btTwsPairingStart;
	uint32_t			btTwsPairingTimeroutCnt;
	uint32_t			btTwsRepairTimerout;

	//link loss, slave回连开启等待count
	uint32_t			btTwsLinkLossTimeoutCnt;

	uint32_t			btTwsSlaveTimeoutCount;	//超时进入可被搜索可被连接状态
}BtTwsAppCt;

extern BtTwsAppCt *gBtTwsAppCt;

void BtTwsAppInit(void);
void BtTwsAppDeinit(void);

void BtTwsDeviceReconnect(void);

void BtTwsDeviceDisconnect(void);

void BtTwsDeviceDisconnectExt(void);

void BtTwsLinkLoss(void);

void BtTwsConnectApi(void);
void BtTwsDisconnectApi(void);

void BtTwsEnterPeerPairingMode(void);

void BtTwsExitPeerPairingMode(void);

void CheckBtTwsPairing(void);
void BtTwsEnterPairingMode(void);
void BtTwsExitPairingMode(void);
void BtTwsPeerEnterPairingMode(void);
void BtTwsEnterSimplePairingMode(void);//soundbar
void BtTwsExitSimplePairingMode(void);
void BtTwsRepairDiscon(void);
void BtTwsRunLoop(void);
//void sniff_wakeup_set(uint8_t set);
//uint8_t sniff_wakeup_get();
void sniff_lmpsend_set(uint8_t set);
uint8_t sniff_lmpsend_get();
bool BtTwsPairingState(void);
void tws_msg_process(uint16_t msg);
void BtTwsPairingStart(void);

#endif /*__BT_TWS_APP_FUNC_H__*/



