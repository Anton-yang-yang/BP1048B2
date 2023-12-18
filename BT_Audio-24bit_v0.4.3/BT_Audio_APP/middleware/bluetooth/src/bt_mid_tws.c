/**
 *******************************************************************************
 * @file    bt_tws_app.c
 * @author  Owen
 * @version V1.0.1
 * @date    10-Oct-2019
 * @brief   tws callback events and actions
 *******************************************************************************
 * @attention
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, MVSILICON SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */

#include "type.h"
#include "debug.h"
#include "clk.h"
#include "gpio.h"
#include "irqn.h"
#include "stdlib.h"
#include "bt_manager.h"
#include "bt_interface.h"
#include "bt_tws_api.h"
#include "mcu_circular_buf.h"
#ifdef CFG_APP_CONFIG
#include "app_config.h"
#include "bt_play_mode.h"
#include "main_task.h"
#include "audio_core_api.h"
#include "ctrlvars.h"
#endif
#include "bt_avrcp_api.h"
#include "dac_interface.h"
#include "adc_interface.h"
#include "sbcenc_api.h"
#include "uarts.h"
#include "dma.h"
#include "i2s.h"
#include "audio_adc.h"
#include "i2s_interface.h"
#include "irqn.h"
#include "audio_vol.h"
#include "bt_app_tws.h"
#include "bt_app_tws_connect.h"
#include "bt_app_ddb_info.h"
#include "bt_app_sniff.h"
#include "remind_sound.h"
#include "tws_mode.h"
uint32_t gBtTwsSniffLinkLoss = 0; //1=在sniff下tws linkloss,需要进行回连处理

#ifdef BT_TWS_SUPPORT

#define TWS_ACL_LINK		0xF1
#define TWS_ACL_UNLINK		0xF2

uint32_t gBtTwsDelayConnectCnt = 0;//配对组网,主动发起连接异常,延时1s再次发起连接

extern void tws_slave_fifo_clear(void);
extern void tws_device_resume(void);

/**************************************************************************
 *  
 *************************************************************************/
int tws_cmp_paired_mac(uint8_t *addr)
{
	return memcmp(addr, btManager.btTwsDeviceAddr, 6);
}

/**************************************************************************
 *  
 *************************************************************************/
bool tws_connect_cmp(uint8_t * addr)
{
	if(memcmp(btManager.btTwsDeviceAddr, addr, 6) == 0)
		return 0;
	else
		return 1;
}

/**************************************************************************
 *  
 *************************************************************************/
int tws_cmp_local_mac(uint8_t *addr)
{
	return memcmp(addr, btManager.btDevAddr, 6);
}

/**************************************************************************
 *  
 *************************************************************************/
int tws_inquiry_cmp_name(char* name, uint8_t len)
{
#ifdef TWS_FILTER_NAME
	extern BT_CONFIGURATION_PARAMS		*btStackConfigParams;
	uint8_t localLen = strlen(( char*)btStackConfigParams->bt_LocalDeviceName);
	if(len != localLen)
		return -1;
	return memcmp(name, &btStackConfigParams->bt_LocalDeviceName[0], localLen);
#else
	return 0;
#endif
}

/**************************************************************************
 *  
 *************************************************************************/
int tws_filter_user_defined_infor_cmp(uint8_t *infor)
{
#ifdef TWS_FILTER_USER_DEFINED
	return memcmp(infor, &btManager.TwsFilterInfor[0], 6);
#else
	return 0;
#endif
}

/**************************************************************************
 *  
 *************************************************************************/
BT_TWS_ROLE tws_get_role(void)
{
	return btManager.twsRole;
}


/**************************************************************************
 *  
 *************************************************************************/
unsigned char tws_role_match(unsigned char *addr)
{
#if (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
	if(GetSystemMode() == ModeTwsSlavePlay)
		return BT_TWS_SLAVE;
	else
		return BT_TWS_MASTER;
#endif
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
	if(btManager.btLinkState)
	{
		return BT_TWS_MASTER;
	}
#endif
	if(memcmp(btManager.btTwsDeviceAddr, addr, 6) == 0)
	{
		return btManager.twsRole;
	}
	else
	{
		return 0xff;
	}
}

/**************************************************************************
 * @brief  bt tws callback function(master / slave)
 * @param  event	
 * @param  param	
 *************************************************************************/
void BtTwsCallback(BT_TWS_CALLBACK_EVENT event, BT_TWS_CALLBACK_PARAMS * param)
{
	//uint8_t tws_addr[6];
	switch(event)
	{
		//master
		case BT_STACK_EVENT_TWS_CONNECTED:
#if defined(CFG_APP_CONFIG) && defined(CFG_FUNC_REMIND_SOUND_EN)
			SoftFlagRegister(SoftFlagTwsRemind);
#endif
			if(param->role == BT_TWS_SLAVE)
			{
				BtTws_Slave_Connected(param);
			}
			else
			{
				BtTws_Master_Connected(param);
				#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
				extern uint32_t doubleKeyCnt;
				extern void BtBroadcastNameUpdate(void);
				doubleKeyCnt = 0;
				BtBroadcastNameUpdate();//20230519
				#endif
			}
			break;
			
		case BT_STACK_EVENT_TWS_DISCONNECT:
#if defined(CFG_APP_CONFIG) && defined(CFG_FUNC_REMIND_SOUND_EN)
			SoftFlagDeregister(SoftFlagTwsRemind);
#endif
			tws_device_resume();
			if(param->role == BT_TWS_SLAVE)
			{
				printf("slave dis\n");
				BtTws_Slave_Disconnected(param);
			}
			else
			{
				BtTws_Master_Disconnected(param);
			}
	#if (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
			#include "main_task.h"
			if(GetSystemMode() == ModeTwsSlavePlay)
			{
				MessageContext		msgSend;
				msgSend.msgId		= MSG_BT_TWS_DISCONNECT;
				//MessageSend(GetSysModeMsgHandle(), &msgSend);
			}
	#endif
			break;
			
		case BT_STACK_EVENT_TWS_DATA_IND:
			BtTwsRecvCmdData(param);
			break;

			
		case BT_STACK_EVENT_TWS_SLAVE_STREAM_START:
#ifdef CFG_APP_CONFIG
			tws_slave_fifo_clear();
#endif
			break;

		case BT_STACK_EVENT_TWS_SLAVE_STREAM_STOP:
			break;
			
		//当前TWS链路存在,则延时再次发起连接
		case BT_STACK_EVENT_TWS_CONNECT_DELAY:
			if(gBtTwsAppCt->btTwsPairingStart)
			{
				gBtTwsDelayConnectCnt = 50;//延时1s //20ms*50=1000ms=1s
			}
			break;
			
		default:
			break;
	}
}

/**************************************************************************
 * @brief  tws master active mode
 * @note   slave 接收到cmd: master当前处于active模式,需要退出sniff和deepsleep状态
 *************************************************************************/
void tws_master_active_mode(void)
{
#ifdef BT_SNIFF_ENABLE
	if(GetBtManager()->twsRole == BT_TWS_SLAVE)
	{
		if(Bt_sniff_sniff_start_state_get())
		{
			Bt_sniff_sniff_stop();
		}
	}
#endif
}


/**************************************************************************
 * @brief  
 *************************************************************************/
bool tws_slave_switch_mode_enable(void)
{
#ifdef CFG_AUTO_ENTER_TWS_SLAVE_MODE
	return 1;
#else
	return 0;
#endif
}



#else
int tws_inquiry_cmp_name(char* name, uint8_t len)
{
	return 0;
}

int tws_filter_user_defined_infor_cmp(uint8_t *infor)
{
	return 0;
}

bool tws_connect_cmp(uint8_t * addr)
{
	return 1;
}

unsigned char tws_role_match(unsigned char *addr)
{
	return 0xff;
}

bool tws_slave_switch_mode_enable(void)
{
	return 0;
}

void tws_master_active_mode(void)
{
}

BT_TWS_ROLE tws_get_role(void)
{
	return 0;
}

#endif
