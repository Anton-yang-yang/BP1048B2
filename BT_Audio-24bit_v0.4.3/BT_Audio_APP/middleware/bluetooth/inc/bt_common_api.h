/**
 *******************************************************************************
 * @file    bt_mid_common.h
 * @author  Halley
 * @version V1.0.1
 * @date    27-Apr-2016
 * @brief   common related api
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

/**
* @addtogroup Bluetooth
* @{
* @defgroup bt_common_api bt_common_api.h
* @{
*/

#ifndef _BT_COMMON_API_H_
#define _BT_COMMON_API_H_

#include "type.h"
#ifdef CFG_APP_CONFIG
#include "app_config.h"
#endif
#include "bt_config.h"
#include "bt_manager.h"
#include "bt_tws_api.h"
#include "bt_hfp_api.h"
#include "bt_a2dp_api.h"
#include "bt_avrcp_api.h"
#include "bt_spp_api.h"

/*---------------------------------------------------------------------------
 * BtClassOfDevice type
 *
 *     Bit pattern representing the class of device along with the 
 *     supported services. There can be more than one supported service.
 *     Service classes can be ORed together. The Device Class is composed
 *     of a major device class plus a minor device class. ORing together
 *     each service class plus one major device class plus one minor device
 *     class creates the class of device value. The minor device class is
 *     interpreted in the context of the major device class.
 */

typedef uint32_t BtClassOfDevice;

/* Group: Major Service Classes. Can be ORed together */
#define COD_LIMITED_DISCOVERABLE_MODE 0x00002000
#define COD_POSITIONING               0x00010000
#define COD_NETWORKING                0x00020000
#define COD_RENDERING                 0x00040000
#define COD_CAPTURING                 0x00080000
#define COD_OBJECT_TRANSFER           0x00100000
#define COD_AUDIO                     0x00200000
#define COD_TELEPHONY                 0x00400000
#define COD_INFORMATION               0x00800000

/* Group: Major Device Classes (Select one) */
#define COD_MAJOR_MISCELLANEOUS       0x00000000
#define COD_MAJOR_COMPUTER            0x00000100
#define COD_MAJOR_PHONE               0x00000200
#define COD_MAJOR_LAN_ACCESS_POINT    0x00000300
#define COD_MAJOR_AUDIO               0x00000400
#define COD_MAJOR_PERIPHERAL          0x00000500
#define COD_MAJOR_IMAGING             0x00000600
#define COD_MAJOR_UNCLASSIFIED        0x00001F00

/* Group: Minor Device Class - Audio Major class */
#define COD_MINOR_AUDIO_UNCLASSIFIED  0x00000000
#define COD_MINOR_AUDIO_HEADSET       0x00000004
#define COD_MINOR_AUDIO_HANDSFREE     0x00000008
#define COD_MINOR_AUDIO_MICROPHONE    0x00000010
#define COD_MINOR_AUDIO_LOUDSPEAKER   0x00000014
#define COD_MINOR_AUDIO_HEADPHONES    0x00000018
#define COD_MINOR_AUDIO_PORTABLEAUDIO 0x0000001C
#define COD_MINOR_AUDIO_CARAUDIO      0x00000020
#define COD_MINOR_AUDIO_SETTOPBOX     0x00000024
#define COD_MINOR_AUDIO_HIFIAUDIO     0x00000028
#define COD_MINOR_AUDIO_VCR           0x0000002C
#define COD_MINOR_AUDIO_VIDEOCAMERA   0x00000030
#define COD_MINOR_AUDIO_CAMCORDER     0x00000034
#define COD_MINOR_AUDIO_VIDEOMONITOR  0x00000038
#define COD_MINOR_AUDIO_VIDEOSPEAKER  0x0000003C
#define COD_MINOR_AUDIO_CONFERENCING  0x00000040
#define COD_MINOR_AUDIO_GAMING        0x00000048

/* Group: Minor Device Class - Peripheral Major class */
#define COD_MINOR_PERIPH_KEYBOARD     0x00000040
#define COD_MINOR_PERIPH_POINTING     0x00000080
#define COD_MINOR_PERIPH_COMBOKEY     0x000000C0
/* End of BtClassOfDevice */

/*******************************************************
 * support profiles 
 *******************************************************/
#define BT_PROFILE_SUPPORTED_A2DP				0x0001
#define BT_PROFILE_SUPPORTED_AVRCP				0x0002
#define BT_PROFILE_SUPPORTED_HFP				0x0004
#define BT_PROFILE_SUPPORTED_MFI				0x0008
#define BT_PROFILE_SUPPORTED_SPP				0x0010
#define BT_PROFILE_SUPPORTED_OBEX				0x0020
#define BT_PROFILE_SUPPORTED_HID				0x0040
#define BT_PROFILE_SUPPORTED_PBAP				0x0080
#define BT_PROFILE_SUPPORTED_TWS				0x8000

#define BT_PROFILE_SUPPORT_GENERAL				(BT_PROFILE_SUPPORTED_A2DP|BT_PROFILE_SUPPORTED_AVRCP|BT_PROFILE_SUPPORTED_HFP)

/*******************************************************************************
*
* Inquiry mode define
*/
typedef uint8_t					BTInquiryMode;

#define INQUIRY_MODE_NORMAL		0 /*!< Normal Inquiry Response format			*/
#define INQUIRY_MODE_RSSI		1 /*!< RSSI Inquiry Response format				*/
#define INQUIRY_MODE_EXTENDED	2 /*!< Extended or RSSI Inquiry Response format	*/


typedef struct _InquriyResultParam
{
	uint8_t	*addr;
	signed short	rssi;
	uint8_t	*extResp;
	uint16_t extRespLen;
}InquriyResultParam;


typedef struct _RequestRemNameParam
{
	const uint8_t	*addr;
	const uint8_t	*name;
	uint16_t		nameLen;
}RequestRemNameParam;


/**
 * BtAccessibleMode type
 */
typedef uint8_t		BtAccessMode;

#define BtAccessModeNotAccessible		0x00 /*!< Non-discoverable or connectable		*/
#define BtAccessModeDiscoverbleOnly		0x01 /*!< Discoverable but not connectable		*/
#define BtAccessModeConnectableOnly		0x02 /*!< Connectable but not discoverable		*/
#define BtAccessModeGeneralAccessible	0x03 /*!< General discoverable and connectable	*/

/**
 * BTLinkMode type
 */
typedef uint8_t		BTLinkMode;

#define BTLinkModeActive		0x00
#define BTLinkModeHold			0x01
#define BTLinkModeSniff			0x02
#define BTLinkModePark			0x03
#define BTLinkModeScatter		0x04

typedef struct _ModeChange
{
	BTLinkMode	mode;
	uint8_t		*addr;
}ModeChange;

/**
 * @brief  enable device under test mode.
 * @param  NONE.
 * @return True for the command implement successful 
 * @Note 
 *
 */
bool BTEnterDutMode(void);

/**
 * @brief  User Command: Inquiry command, the bluetooth device will start inquiry with inquiry mode.
 * @param  inquiryMode The inquiry mode, see BTInquiryMode.
 * @return True for the command implement successful 
 * @Note 
 *
 */
bool BTInquiry(BTInquiryMode inquiryMode);

/**
 * @brief  User Command: Cancel inquiry .
 * @param  NONE.
 * @return True for the command implement successful 
 * @Note 
 *
 */
bool BTInquiryCancel(void);

/**
 * @brief  User Command: Get remote device name.
 * @param  addr The bt address of remote device.
 * @return True for the command implement successful 
 * @Note 
 *
 */
bool BTGetRemoteDeviceName(uint8_t * addr);

/**
 * @brief  User Command: Set local bluetooth access mode.
 * @param  accessMode The local bluetooth access mode.
 * @return True for the command implement successful 
 * @Note 
 *
 */
bool BTSetAccessMode(BtAccessMode accessMode);


/**
 * @brief  User Command: Info remote device into sniff mode
 * @param  remAddr The bluetooth address of remote device.
 * @return True for the command implement successful 
 * @Note 
 *
 */

void BTSetRemDevIntoSniffMode(uint8_t * remAddr);

/**
 * @brief  User Command: Info remote device exit from sniff mode
 * @param  remAddr The bluetooth address of remote device.
 * @return True for the command implement successful 
 * @Note 
 *
 */	
void BTSetRemDevExitSniffMode(uint8_t * remAddr);

/**
 * @brief  User Command: Disconnect all connected profile
 * @param  NONE
 * @return True for the command implement successful 
 * @Note 
 *
 */	

bool BTDisconnect(uint8_t index);

bool BTHciDisconnectCmd(const uint8_t *addr);

/**
 * @brief  User Command: set page timeout params
 * @param  timeout(ms)
 * @return True for the command implement successful 
 * @Note 
 *
 */	
bool BTSetPageTimeout(uint16_t timeout);

/**
 * @brief  config class of device params
 * @param  bt cod
 * @return NONE 
 * @Note 
 *
 */	
void SetBtClassOfDevice(uint32_t BtCod);

/**
 * @brief  bt local device name set
 * @param  device name
 *		   len
 * @return NONE
 * @Note 
 *
 */	
void BtSetDeviceName(char *name, uint8_t len);

/********************************************************************************
 ********************************************************************************/

/********************************************************
 * Max length of local bluetooth device name
 * DON'T modify this value!!!
 *******************************************************/
typedef enum
{
	BT_STACK_EVENT_NONE = 0,

	/**
	 *@brief
	 * The stack is initialized
	 *
	 *@note
	 * No params
	 */
	BT_STACK_EVENT_COMMON_STACK_INITIALIZED,

	/**
	 *@brief
	 * The stack memory is out of size
	 *
	 *@note
	 * No params
	 */
	BT_STACK_EVENT_COMMON_STACK_OUT_OF_MEM_ERR,

	/**
	 *@brief
	 * The stack memory free size
	 *
	 *@note
	 * No params
	 */
	BT_STACK_EVENT_COMMON_STACK_FREE_MEM_SIZE,

	/**
	 *@brief
	 * The stack has been uninitialized
	 *
	 *@note
	 * No params
	 */
	BT_STACK_EVENT_COMMON_STACK_UNINITIALIZED,

	/**
	 *@brief
	 * A remote device is found.
	 *
	 *@note
	 * params.inquiryResult is valid
	 */
	BT_STACK_EVENT_COMMON_INQUIRY_RESULT,

	/**
	 *@brief
	 * The inquiry process is completed.
	 *
	 *@note
	 * No params
	 */
	BT_STACK_EVENT_COMMON_INQUIRY_COMPLETE,

	/**
	 *@brief
	 * The inquiry process is cancelled.
	 *
	 *@note
	 * No params
	 */
	BT_STACK_EVENT_COMMON_INQUIRY_CANCELED,

	/**
	 *@brief
	 * Indicate the mode of the link changed.
	 *
	 *@note
	 * params.modeChange is valid
	 */
	BT_STACK_EVENT_COMMON_MODE_CHANGE,

	/**
	 *@brief
	 * Indicate the name of a remote device
	 *
	 *@note
	 * params.remDevName is valid
	 */
	BT_STACK_EVENT_COMMON_GET_REMDEV_NAME,

	/**
	 *@brief
	 * Indicate that an accessibility change is complete.
	 *
	 *@note
	 * params.accessMode is valid.
	 */
	BT_STACK_EVENT_COMMON_ACCESS_MODE,

	/**
	 *@brief
	 * Handles a L2CAP packet reassembly error by disconnecting the faulty link.
	 *
	 *@note
	 * No params
	 */
	BT_STACK_EVENT_COMMON_CONNECTION_ABORTED,

	/**
	 *@brief
	 * page time out(hf) or src not ready(acl waiting for timeout).
	 *
	 *@note
	 * No params
	 */
	BT_STACK_EVENT_COMMON_PAGE_TIME_OUT,

	/**
	 *@brief
	 * add new record information.
	 *
	 *@note
	 * No params
	 */
	BT_STACK_EVENT_TRUST_LIST_ADD_NEW_RECORD,

	/**
	 *@brief
	 * delete new record information.
	 *
	 *@note
	 * No params
	 */
	BT_STACK_EVENT_TRUST_LIST_DELETE_RECORD,

	/**
	 *@brief
	 * baseband lost connection. need to reconnect
	 *
	 *@note
	 * No params
	 */
	BT_STACK_EVENT_COMMON_BB_LOST_CONNECTION,

	/**
	 *@brief
	 * baseband lost connection. need to reconnect
	 *
	 *@note
	 * No params
	 */
	BT_STACK_EVENT_COMMON_TWS_LOST_CONNECTION,

	BT_STACK_EVENT_COMMON_TWS_IS_FOUND,

	BT_STACK_EVENT_COMMON_TWS_PAIRING_TIMEOUT,

	BT_STACK_EVENT_COMMON_TWS_PAIRING_STOP,	// 主动发起停止组网请求

	/**
	 *@brief
	 * Indicate that app setting access mode.
	 *
	 *@note
	 */
	BT_STACK_EVENT_SETTING_ACCESS_MODE,

    BT_STACK_EVENT_COMMON_LINK_DISCON_LOCAL,

}BT_STACK_CALLBACK_EVENT;

typedef struct _BT_STACK_MEM_PARAMS
{
	uint32_t 					stackFreeMemSize;
	uint32_t 					stackMallocMemSize;
}BT_STACK_MEM_PARAMS;

typedef struct _BT_STACK_CALLBACK_PARAMS
{
	uint16_t					paramsLen;
	bool						status;
	uint16_t					errorCode;

	union
	{
		BT_STACK_MEM_PARAMS		stackMemParams;
		uint8_t					*bd_addr;
		InquriyResultParam		inquiryResult;
		ModeChange				modeChange;
		RequestRemNameParam		remDevName;
		BtAccessMode			accessMode;
	}params;
}BT_STACK_CALLBACK_PARAMS;

void BtStackCallback(BT_STACK_CALLBACK_EVENT event, BT_STACK_CALLBACK_PARAMS * param);

typedef void (*BTStackCallbackFunc)(BT_STACK_CALLBACK_EVENT event, BT_STACK_CALLBACK_PARAMS * param);

typedef uint32_t (*BTHfScoCallbackFunc)(uint8_t* data, uint32_t len);


typedef struct _BtStackParams
{
	uint8_t				*localDevName;

	uint32_t			supportProfiles;
	BTStackCallbackFunc	callback;
	BTHfScoCallbackFunc scoCallback;

	uint8_t				btSimplePairing;
	uint8_t				btPinCode[16];//max 16
	uint8_t				btPinCodeLen;

	/*TWS releated features*/
	TwsAppFeatures		twsFeatures;

	/*HFP releated features*/
	HfpAppFeatures		hfpFeatures;

	/*A2DP releated features*/
	A2dpAppFeatures		a2dpFeatures;

	/*AVRCP releated features*/
	AvrcpAppFeatures	avrcpFeatures;
}BtStackParams;


/**
 * @brief
 * @param
 * @return
 * @Note
 */
typedef struct _BT_HOST_PARAM{
	//common_part
	unsigned short host_NUM_BT_DEVICES;
	unsigned short host_NUM_SCO_CONNS;
}BT_HOST_PARAM;

void BTHostParamsConfig(BT_HOST_PARAM *params);

/**
 * @brief  bt stack run init
 * @param  stackParams
 * @return 0 - init success
 *			other - init error
 * @Note
 *
 */
int32_t BTStackRunInit(BtStackParams *stackParams);

/**
 * @brief  bt stack run
 * @param  NONE
 * @return NONE
 * @Note
 *
 */
void  BTStackRun(void);

/**
 * @brief  bt stack run uninit
 * @param  NONE
 * @return NONE
 * @Note
 *
 */
void  BTStackRunUninit(void);

/**
 * @brief  has bt data to process
 * @param  NONE
 * @return TRUE/FALSE
 * @Note
 *
 */
bool HasBtDataToProccess(void);

/**
 * @brief  get current bt library version
 * @param  NONE
 * @return the pointer of string
 * @Note
 *
 */
const unsigned char * GetLibVersionBt(void);
const unsigned char *GetLibVersionBt_Flashboot(void);


/**
 * @brief  get local bt address
 * @param  bt_addr
 * @return TRUE/FALSE
 * @Note
 *
 */
bool GetLocalBtAddr(uint8_t * bt_addr);

/**
 * @brief  get local bt name
 * @param  bt_name
 *			bt_name_len
 * @return TRUE/FALSE
 * @Note
 *
 */
bool GetLocalBtName(uint8_t * bt_name, uint8_t * bt_name_len);


/*
*
*/
typedef uint8_t (*BTStackRecvHookFunc) (uint8_t* data, uint32_t data_len);
void BTStackSetReceivedDataHook(BTStackRecvHookFunc HookFunc);


typedef void (*BTStackSendHookFunc)(void);
void BTStackSetSendDataHook(BTStackSendHookFunc HookFunc);

/**
 * @brief  bt stack memory alloc
 * @param  btStackSize
 * @return TRUE/FALSE
 * @Note
 *
 */
uint32_t BTStackMemAlloc(uint32_t btStackSize, void* stackBuf, uint8_t IsOsSys);

/**
 * @brief  bt stack memory free
 * @param  NONE
 * @return 0 - SUCESS
 * @Note
 *
 */
int32_t BTStackMemFree(void);

/**
 * @brief
 * 		Write Inquiry Scan Params: Interval and Window
 *
 * @param
 *		Interval: 0x0012~0x1000 (default: 0x1000); only even values are valid
 *		Window: 0x0011~Inquiry Scan Interval(Max:0x1000) (default: 0x0012);
 *		time = N*0.625ms
 *
 * @note
 *		在bt_common_app.c里面调用
 */
void WriteInquiryScanActivity(uint16_t newInterval, uint16_t newWindow);

/**
 * @brief
 * 		Write Page Scan Params: Interval and Window
 *
 * @param
 *		Interval: 0x0012~0x1000 (default: 0x1000); only even values are valid
 *		Window: 0x0011~Inquiry Scan Interval(Max:0x1000) (default: 0x0012);
 *		time = N*0.625ms
 *
 * @note
 */
void WritePageScanActivity(uint16_t newInterval, uint16_t newWindow);

/**
 * @brief
 * 		page time out value
 *
 * @param
 *		TimeOutValueInMs: 0x0001~0xffff (default: 0x2000); only even values are valid
 *		time = N*0.625ms
 *
 * @note
 */
void BTStatckSetPageTimeOutValue(uint16_t TimeOutValueInMs);

/**
 * @brief
 * 		get bt disconnect reason
 *
 * @param
 *		NULL
 *
 * @return
 *		reason:
 *		1=remote device terminate connection
 *		2=BP10 terminate connection
 *		3=remote device link loss
 *
 * @note
 */
#define BT_DISCON_REASON_ERROR			0
#define BT_DISCON_REASON_REMOTE_DIS		1
#define BT_DISCON_REASON_LOCAL_DIS		2
#define BT_DISCON_REASON_LINKLOSS		3
uint32_t GetBtDisconReason(void);


/**
 * @}
 * @}
 */
void BtTwsPairingConnect(uint8_t *tws_addr);

#endif

