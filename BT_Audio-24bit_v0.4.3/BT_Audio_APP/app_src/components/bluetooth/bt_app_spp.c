/**
 *******************************************************************************
 * @file    bt_app_spp.h
 * @author  Halley
 * @version V1.0.1
 * @date    27-Apr-2016
 * @brief   Spp callback events and actions
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

#include "bt_manager.h"
#include "bt_config.h"
#include "bt_spp_api.h"
#include "mode_task.h"

#if (BT_SPP_SUPPORT == ENABLE ||(defined(CFG_FUNC_BT_OTA_EN)))
#ifdef CFG_FUNC_BT_OTA_EN
extern const char cmd_chip_reset[];
extern const char cmd_chip_spp_link[];
#endif
void BtSppCallback(BT_SPP_CALLBACK_EVENT event, BT_SPP_CALLBACK_PARAMS * param)
{
	//DBG("boot_BtSppCallback %x\n",event);
	switch(event)
	{
		case BT_STACK_EVENT_SPP_CONNECTED:
			DBG("SPP EVENT:connected\n");
#ifdef CFG_FUNC_BT_OTA_EN
			SppSendData((uint8_t *)cmd_chip_spp_link,8);
#endif
			break;
		
		case BT_STACK_EVENT_SPP_DISCONNECTED:
			DBG("SPP EVENT:disconnect\n");			
			break;
		
		case BT_STACK_EVENT_SPP_DATA_RECEIVED:
#ifdef CFG_FUNC_BT_OTA_EN
			//DBG("SPP EVENT:receive len:%d\n", param->paramsLen);
			if(!SoftFlagGet(SoftFlagBtOtaUpgradeOK))
			{
				if(memcmp(param->params.sppReceivedData,cmd_chip_reset,10) == 0)
				{
					MessageContext		msgSend;

					msgSend.msgId = MSG_BT_START_OTA;
					MessageSend(GetMainMessageHandle(), &msgSend);
				}
			}
#endif
			{
			#if 0
				uint16_t i;
				for(i=0;i<param->paramsLen;i++)
				{
					DBG("0x%02x ", param->params.sppReceivedData[i]);
					if((i!=0)&&(i%16 == 0))
						DBG("\n");
				}
			#endif	
			}
			break;
		
		case BT_STACK_EVENT_SPP_DATA_SENT:
			break;
		
		default:
			break;
	}
}

void BtSppHookFunc(void)
{

}

#endif

