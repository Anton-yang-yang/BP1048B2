/**
 **************************************************************************************
 * @file    bluetooth_hfp_deal.h
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

#ifndef _BLUETOOTH_HFP_DEAL_H_
#define _BLUETOOTH_HFP_DEAL_H_

#include "bt_hfp_api.h"

//"bt_mid_hfp.c"
int16_t SetBtCallInPhoneNumber(uint8_t index, const uint8_t * number, uint16_t len);
int16_t SetBtCallWaitingNotify(bool flag);
int16_t SetBtBatteryLevel(uint8_t level);
int16_t SetBtHfpSignalLevel(uint8_t level);

//3way-calling 
//挂断当前通话,接听另一个电话
void BtHfp_Hangup_Answer_Call(void);
//挂断 另外通话, 保持当前通话
void BtHfp_Hangup_Another_Call(void);
//挂起当前通话, 接听另一个电话
void BtHfp_HoldCur_Answer_Call(void);


void BtHfpConnectedDev(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpDisconnectedDev(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpScoLinkConnected(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpScoLinkDisconnected(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpCallConnected(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpCallDisconnected(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpCallSetupNone(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpCallSetupIncoming(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpCallSetupOutgoing(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpCallSetupAlert(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpCallRing(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpCallerIdNotify(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpCallWaitNotify(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpBatteryLevel(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpSignalLevel(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpGetCurCallState(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpVoiceRecognition(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpSpeakerVolume(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpCodecType(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpManufactoryInfo(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpScoDataReceived(BT_HFP_CALLBACK_PARAMS * param);

void BtHfpCurCallState(BT_HFP_CALLBACK_PARAMS * param);


void BtHfpGetCurCallStateRunloop(void);
void BtHfpRunloopRegister(void);
void BtHfpRunloopDeregister(void);


#endif
