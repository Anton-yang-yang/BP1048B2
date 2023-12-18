/**
 **************************************************************************************
 * @file    bluetooth_common.h
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

#ifndef __BLUETOOTH_COMMON_H__
#define __BLUETOOTH_COMMON_H__

#include "type.h"
#include "bt_common_api.h"

void BtStackInitialized(void);

uint32_t comfirm_connecting_condition(uint8_t *addr);

void BtAccessModeUpdate(BtAccessMode accessMode);

void BtGetRemoteName(BT_STACK_CALLBACK_PARAMS * param);

void BtLinkModeChanged(BT_STACK_CALLBACK_PARAMS * param);

void BtLinkPageTimeout(BT_STACK_CALLBACK_PARAMS * param);

void BtDevConnectionAborted(void);

void BtPairingFail(void);

void BtDevConnectionLinkLoss(BT_STACK_CALLBACK_PARAMS * param);

void BtTwsConnectionLinkLoss(void);

void BtTwsPairingTimeout(void);

void BtAccessModeSetting(void);

uint32_t GetSupportProfiles(void);

void SetBtConnectedProfile(uint8_t index, uint16_t connectedProfile);

void SetBtDisconnectProfile(uint8_t index, uint16_t disconnectProfile);

uint16_t GetBtConnectedProfile(void);

uint8_t GetNumOfBtConnectedProfile(void);

void BtEventFlagRegister(uint32_t SoftEvent, uint32_t SoftTimeOut);
void BtEventFlagDeregister(uint32_t SoftEvent);

void BtEventFlagProcess(void);



#endif/*__BLUETOOTH_COMMON_H__*/

