/**
 **************************************************************************************
 * @file    bluetooth_connect.h
 * @brief	蓝牙连接、回连相关功能函数接口
 *
 * @author  kk
 * @version V1.0.0
 *
 * $Created: 2019-10-28 18:00:00$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef __BLUETOOTH_CONNECT_H__
#define __BLUETOOTH_CONNECT_H__


#include "type.h"
#include "bt_manager.h"

void BtReconProfilePrioRegister(uint32_t profile);
void BtReconProfilePrioDeregister(uint32_t profile);
uint32_t BtReconProfilePrioFlagGet(void);

void BtReconnectDevStop(void);
void BtReconnectProcess(void);
uint8_t BtReconnectDevIsUsed(void);
uint8_t BtReconnectDevIsExcute(void);
void BtReconnectDevAgain(uint32_t delay);


void BtScanPageStateSet(BT_SCAN_PAGE_STATE state);
void BtScanPageStateCheck(void);

void BtReconnectDevCreate(uint8_t *addr, uint8_t tryCount, uint8_t interval, uint32_t delayMs, uint32_t profile);
void BtReconnectDevice(void);

void BtReconnectTwsStop(void);
void BtReconnectTws_Slave(void);
void BtReconnectTwsCreate(uint8_t *addr, uint8_t tryCount, uint8_t interval, uint32_t delayMs, uint32_t profile);
void BtReconnectTws(void);
void BtStartReconnectTws(uint8_t tryCount, uint8_t interval);

#endif
