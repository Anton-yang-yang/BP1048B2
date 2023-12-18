#ifndef		__BT_API_ENTRY_H__
#define		__BT_API_ENTRY_H__

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct BT_API_ENTRY
{
	void (*pSendDeepSleepMsg)(void);
	void (*pBBMatchReport)(void);
	void (*pBtCancelConnect)(void);
	void (*pBtCancelReconnect)(void);
	void (*pTWS_SlaveL2capCallback)(void);
	void (*pTWS_MasterL2capCallback)(void);
	void (*ptwsRunLoop)(void);
	void (*pBtStackRunNotify)(void);
	void (*ptws_device_sync_isr)(uint8_t);

	void (*pBBErrorReport)(uint8_t,uint32_t);

	void (*pBtFreqOffsetAdjustComplete)(unsigned char);

	void (*ptws_tx_end)(uint32_t,uint32_t,uint32_t);
	void (*ptws_rx_end)(uint32_t,uint32_t,uint32_t);

	void (*pload_tws_filter_infor)(uint8_t *);

	uint8_t* (*papp_bt_page_state)(void);
	uint8_t* (*pget_cur_tws_addr)(void);

	int (*ptws_inquiry_cmp_name)(char*,uint8_t);

	int (*ptws_filter_user_defined_infor_cmp)(uint8_t *);

	bool (*ptws_connect_cmp)(uint8_t *);

	uint32_t (*pcomfirm_connecting_condition)(uint8_t *);

	bool (*pGetBtLinkState)(uint8_t);

	void (*ptws_clk_sync)(uint32_t,int16_t,uint32_t,int16_t);

	bool (*pBtTwsPairingState)(void);
}bt_api_entry_t;

void bt_api_entry_register(bt_api_entry_t * entry);

#endif
