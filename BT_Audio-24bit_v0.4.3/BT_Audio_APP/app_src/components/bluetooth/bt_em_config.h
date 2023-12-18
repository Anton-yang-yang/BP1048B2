
///////////////////////////////////////////////////////////////////////////////
//               Mountain View Silicon Tech. Inc.
//  Copyright 2012, Mountain View Silicon Tech. Inc., Shanghai, China
//                       All rights reserved.
//  Filename: bt_em_config.h
//  maintainer: kk
///////////////////////////////////////////////////////////////////////////////

#ifndef BT_EM_CONFIG_H_
#define BT_EM_CONFIG_H_

#include "bt_config.h"
#include "bt_common_api.h"

//EM BT BB CONFIG DATA
#define MAX_NB_ACTIVE_ACL			(3)
#define BT_RXDESC_NB				(5)
#define ACL_DATA_BUF_NB_RX			(BT_RXDESC_NB + 4)

#define ACL_DATA_BUF_NB_TX			(MAX_NB_ACTIVE_ACL + 2 + 6 + 1)
#define BT_LMP_BUF_NB_TX			(10)
#define CSB_SUPPORT					(0)
#define REG_EM_BT_CS_SIZE			(102)
#define MAX_NB_SYNC					(1)
#define REG_EM_BT_RXDESC_SIZE		(14)
#define REG_EM_BT_TXDESC_SIZE		(10)
#define ACL_DATA_BUF_SIZE			(1021)


#define BB_EM_MAP_ADDR				0x80000000

#if( ( BLE_SUPPORT == ENABLE ) && ( BT_SUPPORT == ENABLE ))
#define BB_EM_SIZE					(24*1024)//(( EM_ALL_SIZE & 0xF000 ) + 0x1000 )//
#define BB_EM_START_PARAMS			((320*1024-BB_EM_SIZE)/1024)
#elif( ( BLE_SUPPORT == DISABLE ) && ( BT_SUPPORT == ENABLE ))
#define BB_EM_SIZE					(16*1024)
#define BB_EM_START_PARAMS			((320*1024-BB_EM_SIZE)/1024)
#else
#define BB_EM_SIZE					0
#define BB_EM_START_PARAMS			0
#endif
#define BB_MPU_START_ADDR			(0x20050000 - BB_EM_SIZE)


/*
 * RF SPI part
 ****************************************************************************************
 */

void app_bt_em_params_config(void);

#endif /*__BT_EM_CFG_H__*/

