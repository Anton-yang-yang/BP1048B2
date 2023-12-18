/**
 **************************************************************************************
 * @file    bt_stack_memory.h
 * @brief   
 *
 * @author  kk
 * @version V1.0.0
 *
 * $Created: 2016-6-29 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __BT_STACK_MEMORY_H__
#define __BT_STACK_MEMORY_H__

#include "type.h"
#include "bt_config.h"


/*****************************************************************
 * ram config
 *****************************************************************/
#define BT_BASE_MEM_SIZE			(8000) //common

#if (BT_A2DP_SUPPORT == ENABLE)		//a2dp+avrcp
#define BT_A2DP_MEM_SIZE			(15500)
#else
#define BT_A2DP_MEM_SIZE			0
#endif

#if (BLE_SUPPORT == ENABLE)
#define BT_BLE_MEM_SIZE				2600
#else
#define BT_BLE_MEM_SIZE				0
#endif

#ifdef BT_AUDIO_AAC_ENABLE
#define BT_AUDIO_AAC_MEM_SIZE		900
#else
#define BT_AUDIO_AAC_MEM_SIZE		0
#endif

#if ((BT_AVRCP_VOLUME_SYNC == ENABLE)||(BT_AVRCP_PLAYER_SETTING == ENABLE))
#define BT_AVRCP_TG_MEM_SIZE		(6300)
#else
#define BT_AVRCP_TG_MEM_SIZE		0
#endif

#if (BT_HFP_SUPPORT == ENABLE)
#define BT_HFP_MEM_SIZE				(5800)
#else
#define BT_HFP_MEM_SIZE				0
#endif

#if (BT_SPP_SUPPORT == ENABLE ||(defined(CFG_FUNC_BT_OTA_EN)))
#define BT_SPP_MEM_SIZE				700
#else
#define BT_SPP_MEM_SIZE				0
#endif

#ifdef BT_TWS_SUPPORT
#define BT_TWS_MEM_SIZE				300
#else
#define BT_TWS_MEM_SIZE				0
#endif

#define BT_STACK_MEM_SIZE	(BT_BASE_MEM_SIZE + BT_A2DP_MEM_SIZE + BT_AUDIO_AAC_MEM_SIZE + BT_AVRCP_TG_MEM_SIZE + \
							BT_HFP_MEM_SIZE + BT_SPP_MEM_SIZE + BT_BLE_MEM_SIZE + BT_TWS_MEM_SIZE )

#endif //__BT_STACK_MEMORY_H__

