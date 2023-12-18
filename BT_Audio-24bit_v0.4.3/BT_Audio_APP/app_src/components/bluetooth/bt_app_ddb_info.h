/**
 **************************************************************************************
 * @file    bluetooth_ddb_info.h
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

#ifndef __BLUETOOTH_DDB_INFO_H__
#define __BLUETOOTH_DDB_INFO_H__
#include "app_config.h"
#include "bt_manager.h"

#define DDB_NOT_FOUND  0xffff

//蓝牙参数保存位置:size:4KB(0x1ff000~0x1fffff)
#define BTDB_CONFIGDATA_START_ADDR		(get_bt_config_addr())//BT_CONFIG_ADDR

//参数配置表:0x1ff000-0x1fffff(4K)
#define BTDB_CONFIG_ADDR				BTDB_CONFIGDATA_START_ADDR
#define BTDB_CONFIG_MEM_SIZE			(4*1024)

//蓝牙连接设备地址保存位置:size:12KB(0x1fb000~0x1fdfff)
#define BTDB_USERDATA_START_ADDR		(get_bt_data_addr())//BT_DATA_ADDR

//配对设备信息:0x1fb000-0x1fcfff(8K)
#define BTDB_TOTAL_RECORD_ADDR			(BTDB_USERDATA_START_ADDR)
#define BTDB_TOTAL_RECORD_MEM_SIZE		(8*1024)

//最后1次配对设备信息:0x1fd000-0x1fdfff(4K) -- unused

//BT RECORD INFOR
//最后1次配对设备记录将从配对过的9个设备中寻找,找列表中的最后一个设备记录
#define MVBT_DB_FLAG						"MVBT"
#define MVBT_DB_FLAG_SIZE					4

//保存的连接过的设备信息
#define MAX_BT_DEVICE_NUM					9 //(tws1+device8)
#define BT_REC_INFO_LEN						25//(6+16+1+1+1) = (addr+linkkey+flag+role+profile)

/*extern uint8_t KeyEnc;*/
//#define PRINT_RECORD_INFOR

/**
 * @brief  open bt database
 * @param  localBdAddr - bt device address
 * @return offset
 * @Note 
 *
 */
uint8_t BtDdb_Open(const uint8_t * localBdAddr);

/**
 * @brief  close bt database
 * @param  NONE
 * @return TRUE - success
 * @Note 
 *
 */
bool BtDdb_Close(void);

/**
 * @brief  add one record to bt database
 * @param  record - the structure pointer(BT_DB_RECORD)
 * @return TRUE - success
 * @Note 
 *
 */
bool BtDdb_AddOneRecord(const BT_DB_RECORD * record);

/**
 * @brief  clear bt database area
 * @param  NONE
 * @return TRUE - success
 * @Note 
 *
 */
bool BtDdb_Erase(void);
void BtDdb_EraseBtLinkInforMsg(void);


int8_t BtDdb_LoadBtConfigurationParams(BT_CONFIGURATION_PARAMS *params);

int8_t BtDdb_SaveBtConfigurationParams(BT_CONFIGURATION_PARAMS *params);

int8_t BtDdb_InitBtConfigurationParams(BT_CONFIGURATION_PARAMS *params);

void DdbDeleteRecord(uint8_t index);

uint32_t DdbFindRecord(const uint8_t *bdAddr);

bool BtDdb_GetLastBtAddr(uint8_t *BtLastAddr, uint8_t* profile);

bool BtDdb_ClearTwsDeviceAddrList(void);

void BtDdb_ClrTwsDevInfor(void);

bool BtDdb_GetTwsDeviceAddr(uint8_t *BtTwsAddr);

uint32_t BtDdb_UpgradeTwsInfor(uint8_t *BtTwsAddr);

//清除tws infor 相关寄存器的记录,不清除flash的内容
bool BtDdb_ClearTwsDeviceRecord(void);

bool BtDdb_UpgradeLastBtAddr(uint8_t *BtLastAddr, uint8_t BtLastProfile);

bool BtDdb_UpgradeLastBtProfile(uint8_t *BtLastAddr, uint8_t BtLastProfile);

bool BtDdb_UpLastPorfile(uint8_t BtLastProfile);

void SaveTotalDevRec2Flash(int OneFullRecBlockSize, int TotalRecNum);

uint32_t GetCurTotaBtRecNum(void);

int32_t BtDeviceSaveNameToFlash(char* deviceName, uint8_t deviceLen,uint8_t name_type);

#endif //__BT_DDB_FLASH_H__


