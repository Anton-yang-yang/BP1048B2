/**
 *******************************************************************************
 * @file    bt_a2dp_app.h
 * @author  Halley
 * @version V1.0.1
 * @date    27-Apr-2016
 * @brief   A2dp callback events and actions
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
#include "bt_app_a2dp_deal.h"
#include "bt_interface.h"

void BtA2dpCallback(BT_A2DP_CALLBACK_EVENT event, BT_A2DP_CALLBACK_PARAMS * param)
{
//	uint8_t i;
	switch(event)
	{
		case BT_STACK_EVENT_A2DP_CONNECTED:
			BtA2dpConnectedDev(param);
			break;

		case BT_STACK_EVENT_A2DP_DISCONNECTED:
			BtA2dpDisconnectedDev(param);
			break;

		case BT_STACK_EVENT_A2DP_CONNECT_TIMEOUT:
			BtA2dpConnectTimeout(param);
			break;

		case BT_STACK_EVENT_A2DP_STREAM_START:
			BtA2dpStreamStart(param);
			break;

		case BT_STACK_EVENT_A2DP_STREAM_SUSPEND:
			BtA2dpStreamSuspend(param);
			break;

		case BT_STACK_EVENT_A2DP_STREAM_DATA_IND:
			BtA2dpStreamDataRecevied(param);
			break;

		case BT_STACK_EVENT_A2DP_STREAM_DATA_TYPE:
			BtA2dpStreamDataType(param);
			break;

		default:
			break;
	}
}

void SetA2dpState(uint8_t index, BT_A2DP_STATE state)
{
	GetBtManager()->btLinked_env[index].a2dpState = state;
}

BT_A2DP_STATE GetA2dpState(uint8_t index)
{
	return btManager.btLinked_env[index].a2dpState;
}

void BtA2dpConnect(uint8_t index, uint8_t *addr)
{
#if (BT_LINK_DEV_NUM == 2)
	uint8_t i;
	for(i=0;i<BT_LINK_DEV_NUM;i++)
	{
		if((btManager.btLinked_env[i].a2dp_index == index)
			&&(GetBtManager()->btLinked_env[i].a2dpState > BT_A2DP_STATE_NONE))
		{
			APP_DBG("BtA2dpConnect:a2dpState index[%d] is %d\n",i,GetBtManager()->btLinked_env[i].a2dpState);
			return;
		}
	}
#endif

	{
		APP_DBG("A2dpConnect index = %d,addr:%x:%x:%x:%x:%x:%x\n", index,addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
		A2dpConnect(index ,addr);
	}
}

