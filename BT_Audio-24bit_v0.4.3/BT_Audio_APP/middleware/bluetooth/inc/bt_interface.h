/**
 **************************************************************************************
 * @file    platform_interface.h
 * @brief   platfomr interface
 *
 * @author  Halley
 * @version V1.1.0
 *
 * $Created: 2016-07-22 16:24:11$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

/**
* @addtogroup Bluetooth
* @{
* @defgroup platform_interface platform_interface.h
* @{
*/


#ifndef __BT_PLATFORM_INTERFACE_H__
#define __BT_PLATFORM_INTERFACE_H__

#include "bt_manager.h"

typedef struct _PLATFORM_INTERFACE_OS_T
{
	void*	(*osMalloc)(uint32_t size);
	void	(*osFree)(void* ptr);

	void*	(*osMemcpy)(void* dest, const void * src, uint32_t size);
	void*	(*osMemset)(void * dest, uint8_t byte, uint32_t size);
}PLATFORM_INTERFACE_OS_T;

PLATFORM_INTERFACE_OS_T		pfiOS;

typedef struct _PLATFORM_INTERFACE_BT_DDB_T
{
	bool	(*OpenBtRecord)(const uint8_t * localBdAddr);
	bool	(*CloseBtRecord)(void);
	bool	(*AddBtRecord)(const BT_DB_RECORD * btDbRecord);
	bool	(*DeleteBtRecord)(const uint8_t * bdAddr);
	bool	(*FindBtRecord)(const uint8_t * bdAddr, BT_DB_RECORD * btDbRecord);
	bool	(*FindBtRecordByIndex)(uint8_t index, BT_DB_RECORD * btDbRecord);
}PLATFORM_INTERFACE_BT_DDB_T;

PLATFORM_INTERFACE_BT_DDB_T	pfiBtDdb;

void SetBtPlatformInterface(PLATFORM_INTERFACE_OS_T * pfiOS,
							PLATFORM_INTERFACE_BT_DDB_T * pfiBtDdb
							);



typedef enum
{
	//
	MSG_BT_MID_NONE = 0,
	MSG_BT_MID_UART_RX_INT,

	MSG_BT_MID_ACCESS_MODE_IDLE,	//蓝牙进入空闲状态

	//COMMON
	MSG_BT_MID_STACK_INIT,			//蓝牙协议栈初始化完成
	MSG_BT_MID_STATE_CONNECTED,		//蓝牙连接成功状态
	MSG_BT_MID_STATE_DISCONNECT,	//蓝牙断开连接状态
//	MSG_BT_MID_STATE_FAST_ENABLE,	//蓝牙初始化完成后状态更新，在开启快速开关机时，需要针对当前模式进行处理

	//A2DP
	MSG_BT_MID_PLAY_STATE_CHANGE,	//播放状态改变
	MSG_BT_MID_VOLUME_CHANGE,		//音量同步
	MSG_BT_MID_STREAM_PAUSE,		//播放暂停

	
	MSG_BT_MID_AVRCP_PANEL_KEY,		

	//HFP
	MSG_BT_MID_HFP_CONNECTED,
	MSG_BT_MID_HFP_PLAY_REMIND,		//通话模式下呼入电话播放来电提示音
	MSG_BT_MID_HFP_PLAY_REMIND_END,	//通话模式下停止播放提示音
	MSG_BT_MID_HFP_CODEC_TYPE_UPDATE,//通话数据格式更新
	MSG_BT_MID_HFP_TASK_RESUME,		//通话模式下恢复通话

}BtMidMessageId;

//message发送到APP层
typedef void (*FUNC_MESSAGE_SEND)(BtMidMessageId messageId, uint8_t Param);
void BtAppiFunc_MessageSend(FUNC_MESSAGE_SEND MessageSendFunc);
extern FUNC_MESSAGE_SEND BtMidMessageSend;


//歌曲信息
typedef void (*FUNC_GET_MEDIA_INFO)(void *Param);
void BtAppiFunc_GetMediaInfo(FUNC_GET_MEDIA_INFO CallbackFunc);
extern FUNC_GET_MEDIA_INFO GetMediaInfo;

//hfp sco数据保存接口
typedef int16_t (*FUNC_SAVE_SCO_DATA)(uint8_t* Param, uint16_t ParamLen);
void BtAppiFunc_SaveScoData(FUNC_SAVE_SCO_DATA CallbackFunc);
extern FUNC_SAVE_SCO_DATA SaveHfpScoDataToBuffer;

//蓝牙断开流程函数接口
typedef void (*FUNC_BT_DISCONNECT_PROCESS)(void);
void BtAppiFunc_BtDisconnectProcess(FUNC_BT_DISCONNECT_PROCESS CallbackFunc);
extern FUNC_BT_DISCONNECT_PROCESS BtDisconnectProcess;

//蓝牙AVRCP快速连接函数接口
typedef void (*FUNC_BT_AVRCP_CON_PROCESS)(void);
void BtAppiFunc_BtAvrcpConProcess(FUNC_BT_AVRCP_CON_PROCESS CallbackFunc);
extern FUNC_BT_AVRCP_CON_PROCESS BtAvrcpConProcess;
void BtStack_BtAvrcpConRegister(uint8_t index);

//蓝牙通话数据发送函数接口
typedef void (*FUNC_BT_SCO_SEND_PROCESS)(void);
void BtAppiFunc_BtScoSendProcess(FUNC_BT_SCO_SEND_PROCESS CallbackFunc);
extern FUNC_BT_SCO_SEND_PROCESS BtScoSendProcess;

//蓝牙断开流程函数接口
typedef void (*FUNC_BT_HFP_GET_CUR_CALL_STATE_PROCESS)(void);
void BtAppiFunc_BtHfpGetCurCallStateProcess(FUNC_BT_HFP_GET_CUR_CALL_STATE_PROCESS CallbackFunc);
extern FUNC_BT_HFP_GET_CUR_CALL_STATE_PROCESS BtHfpGetCurCallStateProcess;

#endif //__PLATFORM_INTERFACE_H__
