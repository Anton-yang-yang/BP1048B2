/**
 **************************************************************************************
 * @file    bluetooth_boot_api.c
 * @brief   
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2021-5-18 18:00:00$
 *
 * @Copyright (C) Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
 

#include "bt_api_entry.h"
#include "bt_manager.h"

extern void SendDeepSleepMsg(void);
extern void BBMatchReport(void);
extern void BtCancelConnect(void);
extern void BtCancelReconnect(void);
extern void TWS_SlaveL2capCallback();
extern void TWS_MasterL2capCallback();
extern void TwsL2capCallback();
extern void twsRunLoop(void);
extern void BtStackRunNotify(void);
extern void BBErrorReport(uint8_t mode, uint32_t errorType);
extern void BtFreqOffsetAdjustComplete(unsigned char offset);
extern void tws_tx_end(uint32_t cmd1,uint32_t cmd2,uint32_t cmd3);
extern void tws_rx_end(uint32_t cmd1,uint32_t cmd2,uint32_t cmd3);
extern void load_tws_filter_infor(uint8_t *infor);
extern uint8_t* app_bt_page_state(void);
extern uint8_t* get_cur_tws_addr(void);
extern int tws_inquiry_cmp_name(char* name, uint8_t len);
extern int tws_filter_user_defined_infor_cmp(uint8_t *infor);
extern bool tws_connect_cmp(uint8_t * addr);
extern uint32_t comfirm_connecting_condition(uint8_t *addr);
extern bool GetBtLinkState(uint8_t index);
extern void update_btDdb(uint8_t addr);
extern void AudioCoreServiceResume(void);
extern void PauseAuidoCore(void);
extern uint16_t AudioDAC0DataLenGet(void);
extern bool is_tws_slave(void);
extern void tws_clk_sync(uint32_t ,int16_t ,uint32_t ,int16_t );

extern void tws_device_sync_isr(uint8_t cmd);
extern bool BtTwsPairingState(void);

const bt_api_entry_t bt_sdk_api =
{
	.pSendDeepSleepMsg = SendDeepSleepMsg,
	.pBBMatchReport = BBMatchReport,
	.pBtCancelConnect = BtCancelConnect,
	.pBtCancelReconnect = BtCancelReconnect,
	.pTWS_SlaveL2capCallback = TwsL2capCallback,//TWS_SlaveL2capCallback;//
	.pTWS_MasterL2capCallback = TwsL2capCallback,//TWS_MasterL2capCallback;//
	.ptwsRunLoop = twsRunLoop,
	.pBtStackRunNotify = BtStackRunNotify,
	.pBBErrorReport = BBErrorReport,
	.pBtFreqOffsetAdjustComplete = BtFreqOffsetAdjustComplete,
	.ptws_tx_end = tws_tx_end,
	.ptws_rx_end = tws_rx_end,
	.ptws_device_sync_isr = tws_device_sync_isr,
	.pload_tws_filter_infor = load_tws_filter_infor,
	.papp_bt_page_state = app_bt_page_state,
	.pget_cur_tws_addr = get_cur_tws_addr,
	.ptws_inquiry_cmp_name = tws_inquiry_cmp_name,
	.ptws_filter_user_defined_infor_cmp = tws_filter_user_defined_infor_cmp,
	.ptws_connect_cmp = tws_connect_cmp,
	.pcomfirm_connecting_condition = comfirm_connecting_condition,
	.pGetBtLinkState = GetBtLinkState,
	.ptws_clk_sync = tws_clk_sync,
	.pBtTwsPairingState = BtTwsPairingState,
};

void bt_api_init(void)
{
	bt_api_entry_register((bt_api_entry_t * )&bt_sdk_api);
}
