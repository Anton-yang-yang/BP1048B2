/**
 **************************************************************************************
 * @file    bluetooth_init.h
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

#ifndef __BLUETOOTH_INIT_H__
#define __BLUETOOTH_INIT_H__

#include "type.h"

typedef unsigned char (*BBSniffNotifyCallback)(void);

typedef struct _BtBbParams
{
	uint8_t		*localDevName;
	uint8_t		localDevAddr[BT_ADDR_SIZE];
	uint8_t		freqTrim;
	uint32_t	em_start_addr;

	//bb agc config
	uint8_t		pAgcDisable;
	uint8_t		pAgcLevel;
	
	//sniff config
	uint8_t		pSniffNego;
	uint16_t	pSniffDelay;
	uint16_t	pSniffInterval;
	uint16_t	pSniffAttempt;
	uint16_t	pSniffTimeout;
	
	BBSniffNotifyCallback	bbSniffNotify;
}BtBbParams;

/***********************************************************************************
 * @brief  bt stack memory init
 * @param  NONE
 * @return NONE
 * @Note 
 **********************************************************************************/
void BtStackMemoryInit(void);

/***********************************************************************************
 * @brief  bb params configuration
 * @param  BtBbParams
 * @return NONE
 * @Note 
 **********************************************************************************/
void ConfigBtBbParams(BtBbParams *params);

/***********************************************************************************
 * @brief  load params from flash
 * @param  NONE
 * @return NONE
 * @Note 
 **********************************************************************************/
void LoadBtConfigurationParams(void);

/***********************************************************************************
 * @brief  bt stack init
 * @param  NONE
 * @return TRUE - init success
 * @Note 
 **********************************************************************************/
bool BtStackInit(void);

/***********************************************************************************
 * @brief  bt stack uninit
 * @param  NONE
 * @return TRUE - uninit success
 * @Note 
 **********************************************************************************/
bool BtStackUninit(void);



#endif/*__BLUETOOTH_INIT_H__*/


