/**
 *******************************************************************************
 * @file    bt_hfp_app.c
 * @author  Halley
 * @version V1.0.1
 * @date    27-Apr-2016
 * @brief   Hfp callback events and actions
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
#include "app_config.h"
#include "bt_config.h"
#include "main_task.h"
#include "bt_hfp_api.h"
#include "bt_manager.h"
#include "bt_interface.h"
#include "app_message.h"
#include "bt_app_hfp_deal.h"
#include "audio_vol.h"

#define BT_MANAGER_HFP_ERROR_NONE					0
#define BT_MANAGER_ERROR_PARAMETER_INVAILD			-1
#define BT_MANAGER_ERROR_NOT_INITIALIZED			-2
#define BT_MANAGER_HFP_ERROR_NOT_CONNECTED			-3

/*
* Previous declare
*/

#if (BT_LINK_DEV_NUM == 2)
static uint8_t FirstTalkingPhoneIndex = 0xff;
static uint8_t SecondTalkingPhoneIndex = 0xff;
#endif


#if (BT_LINK_DEV_NUM == 2)
extern void Set_Current_SCO_ID(uint32_t ID);
extern uint32_t Get_Current_SCO_ID(void);

void FirstTalkingPhoneIndexSet(uint8_t Index)
{
	APP_DBG("set frist index:%d\n",Index);

	if(FirstTalkingPhoneIndex == Index)
		return;

	if(FirstTalkingPhoneIndex == 0xff)
	{
		Set_Current_SCO_ID(0);
	}
	else if((FirstTalkingPhoneIndex != 0xff)&&(SecondTalkingPhoneIndex != 0xff))
	{
		if(Get_Current_SCO_ID() == 0)
		{
			Set_Current_SCO_ID(1);
		}
		else
		{
			Set_Current_SCO_ID(0);
		}
		extern int32_t HfpChangeDelayCount;
		HfpChangeDelayCount = 100;
	}

	FirstTalkingPhoneIndex = Index;
}

uint8_t FirstTalkingPhoneIndexGet(void)
{
	return FirstTalkingPhoneIndex;
}

//void SecondTalkingPhoneIndexSet(uint8_t Index)//目前代码secondindex暂未使用
//{
//	APP_DBG("set Second index:%d\n",Index);
//	SecondTalkingPhoneIndex = Index;
//}
//
//uint8_t SecondTalkingPhoneIndexGet(void)
//{
//	return SecondTalkingPhoneIndex;
//}
#endif

void BtHfpCallback(BT_HFP_CALLBACK_EVENT event, BT_HFP_CALLBACK_PARAMS * param)
{
	if(param->index>=BT_LINK_DEV_NUM)
	{
		APP_DBG("ERROR: index%d,event:0x%x\n", param->index, event);
		return;
	}
	
	switch(event)
	{
		case BT_STACK_EVENT_HFP_CONNECTED:
			{
				BtHfpConnectedDev(param);
				break;
			}
		case BT_STACK_EVENT_HFP_DISCONNECTED:
			{
				BtHfpDisconnectedDev(param);
				break;
			}
		case BT_STACK_EVENT_HFP_SCO_CONNECTED:
			{
				BtHfpScoLinkConnected(param);
			}
			break;

		case BT_STACK_EVENT_HFP_SCO_DISCONNECTED:
			{
				BtHfpScoLinkDisconnected(param);
			}
			break;

#ifndef BT_HFP_MODE_DISABLE
		case BT_STACK_EVENT_HFP_SCO_STREAM_PAUSE:
			APP_DBG("Hfp sco stream pause %d\n", param->index);
			break;
#endif

		case BT_STACK_EVENT_HFP_CALL_CONNECTED:
			{
				BtHfpCallConnected(param);
			}
			break;

		case BT_STACK_EVENT_HFP_CALL_DISCONNECTED:
			{
 				BtHfpCallDisconnected(param);
			}
			break;

#ifndef BT_HFP_MODE_DISABLE
		case BT_STACK_EVENT_HFP_SCO_DATA_RECEIVED:
			{
				BtHfpScoDataReceived(param);
			}
			break;
#endif

		case BT_STACK_EVENT_HFP_CALLSETUP_NONE:
			{
				BtHfpCallSetupNone(param);
			}
			break;

#ifndef BT_HFP_MODE_DISABLE
		case BT_STACK_EVENT_HFP_CALLSETUP_IN:
			{
				BtHfpCallSetupIncoming(param);
			}
			break;

		case BT_STACK_EVENT_HFP_CALLSETUP_OUT:
			{
				BtHfpCallSetupOutgoing(param);
			}
			break;

		case BT_STACK_EVENT_HFP_CALLSETUP_ALERT:
			{
				BtHfpCallSetupAlert(param);
			}
			break;

		case BT_STACK_EVENT_HFP_RING:
			{
				BtHfpCallRing(param);
			}
			break;

		case BT_STACK_EVENT_HFP_CALLER_ID_NOTIFY:
			{
				BtHfpCallerIdNotify(param);
			}
			break;

		case BT_STACK_EVENT_HFP_CURRENT_CALLS:
			{
				BtHfpCurCallState(param);
			}
			break;

		case BT_STACK_EVENT_HFP_CALL_WAIT_NOTIFY:
			{
				BtHfpCallWaitNotify(param);
			}
			break;

		case BT_STACK_EVENT_HFP_BATTERY:  //手机端的电池电量
			{
				BtHfpBatteryLevel(param);
			}
			break;

		case BT_STACK_EVENT_HFP_SIGNAL:
			{
				BtHfpSignalLevel(param);
			}
			break;

		case BT_STACK_EVENT_HFP_VOICE_RECOGNITION:
			{
				BtHfpVoiceRecognition(param);
			}
			break;

		case BT_STACK_EVENT_HFP_SPEAKER_VOLUME:
			BtHfpSpeakerVolume(param);
			break;

		case BT_STACK_EVENT_HFP_IN_BAND_RING:
			{
				if(param->params.hfpInBandRing)
					APP_DBG("Hfp inBandRing TRUE\n");
				else
					APP_DBG("Hfp inBandRing FALSE\n");
			}
			break;
		
		case BT_STACK_EVENT_HFP_MANUFACTORY_INFO:
			BtHfpManufactoryInfo(param);
			break;

		case BT_STACK_EVENT_HFP_CODEC_TYPE:
			BtHfpCodecType(param);
			break;
			
		case BT_STACK_EVENT_HFP_INDICATE_INFO:  //手机端的通话信息
			{
				//APP_DBG("HF indicate value: %d  %d\n", param->params.indicateParms.indicator, param->params.indicateParms.value);
				/*switch(param->params.indicateParms.indicator)
				{
					case 7:
//						{
//							extern uint8_t	btHfHoldCallsFlag;
//							if(param->params.indicateParms.value == 2)
//								btHfHoldCallsFlag = 1;
//							else
//								btHfHoldCallsFlag = 0;
//						}
						break;
					case 2:	
						if(!btManager.appleDeviceFlag)
							break;
					case 1:
						if(param->params.indicateParms.value == 0 && BT_HFP_STATE_3WAY_ATCTIVE_CALL == GetHfpState())
						{
							APP_DBG("BT_STACK_EVENT_HFP_INDICATE_INFO -- BT_HFP_STATE_3WAY_ATCTIVE_CALL\n");
							SetHfpState(BT_HFP_STATE_CONNECTED);
						}
						break;
					default:
						break;
				}*/
			}
			break;
#endif

		default:
			break;
	}
}

/***************************************************************************
 **************************************************************************/

signed char BtHfpConnect(uint8_t index, uint8_t * addr)
{
	signed char status = 0;
#if (BT_LINK_DEV_NUM == 2)
	uint8_t hf_index = GetBtManagerHfpIndex(index);

	if((hf_index < BT_LINK_DEV_NUM)
		&&(btManager.btLinked_env[hf_index].hfpState > BT_HFP_STATE_NONE))
	{
		APP_DBG("BtHfpConnect:hfpState index[%d] is %d\n", hf_index, btManager.btLinked_env[hf_index].hfpState);
		return status;
	}
#endif

	APP_DBG("BtHfpConnect index = %d,addr:%x:%x:%x:%x:%x:%x\n", index,addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
	status = HfpConnect(index, addr);
	return status;
}

void BtHfpDisconnect(uint8_t index)
{
	HfpDisconnect(index);
}

/***************************************************************************
 **************************************************************************/
void SetHfpState(uint8_t index, BT_HFP_STATE state)
{
	btManager.btLinked_env[index].hfpState = state;
}

BT_HFP_STATE GetHfpState(uint8_t index)
{
	return btManager.btLinked_env[index].hfpState;
}


/***************************************************************************
 **************************************************************************/
int16_t SetBtCallInPhoneNumber(uint8_t index, const uint8_t * number, uint16_t len)
{
	uint16_t		tempLen;
	
	if(number == NULL || len == 0 )
		return BT_MANAGER_ERROR_PARAMETER_INVAILD;

	if(btManager.btLinked_env[index].hfpState < BT_HFP_STATE_CONNECTED)
		return BT_MANAGER_HFP_ERROR_NOT_CONNECTED;

	tempLen = len > (MAX_PHONE_NUMBER_LENGTH - 1) ? (MAX_PHONE_NUMBER_LENGTH - 1) : len;
	memset(btManager.phoneNumber, 0, MAX_PHONE_NUMBER_LENGTH);
	strncpy((char*)btManager.phoneNumber, (char*)number, tempLen);

	return tempLen;
}

int16_t GetBtCallInPhoneNumber(uint8_t index, uint8_t * number)
{
	uint16_t		tempLen;

	if(number == NULL)
		return BT_MANAGER_ERROR_PARAMETER_INVAILD;

	if(btManager.btLinked_env[index].hfpState < BT_HFP_STATE_CONNECTED)
		return BT_MANAGER_HFP_ERROR_NOT_CONNECTED;

	tempLen = strlen((const char*)btManager.phoneNumber);

	strncpy((char*)number, (const char*)btManager.phoneNumber, tempLen);

	return tempLen;
}

/***************************************************************************
 **************************************************************************/
int16_t SetBtCallWaitingNotify(bool flag)
{
	btManager.callWaitingFlag = flag;
	return BT_MANAGER_HFP_ERROR_NONE;
}

int16_t GetBtCallWaitingNotify(bool * flag)
{
	if(flag == NULL)
		return BT_MANAGER_ERROR_PARAMETER_INVAILD;

	*flag = btManager.callWaitingFlag;
	return BT_MANAGER_HFP_ERROR_NONE;
}

/***************************************************************************
 **************************************************************************/
int16_t SetBtBatteryLevel(uint8_t level)
{
	btManager.batteryLevel = level;
	return BT_MANAGER_HFP_ERROR_NONE;
}

int16_t GetBtBatteryLevel(uint8_t * level)
{
	if(level == NULL)
		return BT_MANAGER_ERROR_PARAMETER_INVAILD;

	*level = btManager.batteryLevel;
	return BT_MANAGER_HFP_ERROR_NONE;
}

/***************************************************************************
 **************************************************************************/
int16_t SetBtHfpSignalLevel(uint8_t level)
{
	btManager.signalLevel = level;
	return BT_MANAGER_HFP_ERROR_NONE;
}

int16_t GetBtSignalLevel(uint8_t * level)
{
	if(level == NULL)
		return BT_MANAGER_ERROR_PARAMETER_INVAILD;

	*level = btManager.signalLevel;
	return BT_MANAGER_HFP_ERROR_NONE;
}

/**
* 设置sco链路连接标志
*/
void SetScoConnectFlag(bool flag)
{
	btManager.scoConnected = flag;
}
/**
* 获取sco链路连接标志
*/
bool GetScoConnectFlag(void)
{
	return btManager.scoConnected;
}

/**
* 设置语音助手标志
*/
void SetBtHfpVoiceRecognition(bool flag)
{
	btManager.voiceRecognition = flag;
}
/**
* 获取语音助手标志
*/
bool GetBtHfpVoiceRecognition(void)
{
	return btManager.voiceRecognition;
}
/**
* 进入语音助手
*/
void OpenBtHfpVoiceRecognitionFunc(uint8_t index)
{
	if(GetHfpState(index) == BT_HFP_STATE_CONNECTED)
	{
		APP_DBG("open voicerecognition\n");
		HfpVoiceRecognition(index, 1);
		SetBtHfpVoiceRecognition(1);
	}
}

/***************************************************************************
 **************************************************************************/
int16_t SetBtHfpSpeakerVolume(uint8_t gain)
{
#ifdef CFG_APP_BT_MODE_EN
	uint32_t hfpVolume = 0;

	btManager.volGain = gain;
	hfpVolume = (btManager.volGain*CFG_PARA_MAX_VOLUME_NUM)/15;
	if(GetSysModeState(ModeBtHfPlay) == ModeStateRunning || GetSysModeState(ModeBtHfPlay) == ModeStateInit)
		AudioHfVolSet((uint8_t)hfpVolume);
	else
	{
		if(hfpVolume > CFG_PARA_MAX_VOLUME_NUM)
			mainAppCt.HfVolume = CFG_PARA_MAX_VOLUME_NUM;
		else
			mainAppCt.HfVolume = hfpVolume;
	}
#endif
	return BT_MANAGER_HFP_ERROR_NONE;
}

int16_t GetBtHfpSpeakerVolume(uint8_t * gain)
{
	*gain = btManager.volGain;
	return BT_MANAGER_HFP_ERROR_NONE;
}

