/**
 **************************************************************************************
 * @file    audio_core_service.c
 * @brief   
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2018-1-10 20:21:00$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

//#include "soft_watch_dog.h"
#include <string.h>
#include "type.h"
#include "rtos_api.h"
#include "app_message.h"
#include "debug.h"
#include "main_task.h"
#include "app_config.h"
#include "communication.h"
#include "otg_device_standard_request.h"
#include "device_detect.h" 
#include "audio_core_service.h"
#include "audio_core_api.h"



#if (AUDIO_CORE_SERVICE_SIZE <= 512)
#define USE_SYS_STACK	//can free AUDIO_CORE_SERVICE_SIZE*4 btyes RAM
#endif

typedef struct _AudioCoreServiceContext
{
	xTaskHandle			taskHandle;
	MessageHandle		msgHandle;
	MessageHandle		parentMsgHandle;

	TaskState			serviceState;
}AudioCoreServiceContext;

static AudioCoreServiceContext		audioCoreServiceCt;

/**
 * @brief	Audio core servcie init
 * @param	NONE
 * @return	0 for success
 */
static int32_t AudioCoreServiceInit(MessageHandle parentMsgHandle)
{
	memset(&audioCoreServiceCt, 0, sizeof(AudioCoreServiceContext));

	/* register message handle */
	audioCoreServiceCt.msgHandle = MessageRegister(ACS_NUM_MESSAGE_QUEUE);
	if(audioCoreServiceCt.msgHandle == NULL)
	{
		return -1;
	}
	audioCoreServiceCt.serviceState = TaskStateCreating;
	audioCoreServiceCt.parentMsgHandle = parentMsgHandle;

	AudioCoreInit();

	return 0;
}

/*static void AudioCoreServiceDeinit(void)
{
	AudioCoreDeinit();
	audioCoreServiceCt.msgHandle = NULL;
	audioCoreServiceCt.serviceState = TaskStateNone;
	audioCoreServiceCt.parentMsgHandle = NULL;
}*/
uint32_t 	IsAudioCorePause = FALSE;
uint32_t 	IsAudioCorePauseMsgSend = FALSE;
extern void AudioSinkSetForPause(void);
static void AudioCoreServiceEntrance(void * param)
{
	MessageContext		msgRecv;
	MessageContext		msgSend;

	audioCoreServiceCt.serviceState = TaskStateReady;

	/* Send message to parent */
	msgSend.msgId		= MSG_AUDIO_CORE_SERVICE_CREATED;
	MessageSend(audioCoreServiceCt.parentMsgHandle, &msgSend);

	while(1)
	{
		MessageRecv(audioCoreServiceCt.msgHandle, &msgRecv, AUDIO_CORE_SERVICE_TIMEOUT);
#ifdef SOFT_WACTH_DOG_ENABLE
		little_dog_feed(DOG_INDEX2_AudioCoreTask);
#endif
		switch(msgRecv.msgId)
		{
			case MSG_TASK_START:
				APP_DBG("---MSG_TASK_START---\n");
				if(audioCoreServiceCt.serviceState == TaskStateStarting)
				{
					audioCoreServiceCt.serviceState = TaskStateRunning;

					msgSend.msgId		= MSG_AUDIO_CORE_SERVICE_STARTED;
					MessageSend(audioCoreServiceCt.parentMsgHandle, &msgSend);
				}
				break;
				
			case MSG_TASK_PAUSE:
//				audioCoreServiceCt.serviceState = TaskStatePaused;
				//msgSend.msgId		= MSG_AUDIO_CORE_SERVICE_PAUSED;
				//MessageSend(audioCoreServiceCt.parentMsgHandle, &msgSend);
				IsAudioCorePause = TRUE;
				IsAudioCorePauseMsgSend = TRUE;
				break;
			
			case MSG_AUDIO_CORE_HOLD:
				APP_DBG("MSG_AUDIO_CORE_HOLD\n");
				audioCoreServiceCt.serviceState = TaskStatePaused;
				//msgSend.msgId		= MSG_AUDIO_CORE_SERVICE_PAUSED;
				//MessageSend(audioCoreServiceCt.parentMsgHandle, &msgSend);
				break;
			
			case MSG_TASK_RESUME:
				IsAudioCorePause = FALSE;
				if(audioCoreServiceCt.serviceState == TaskStatePaused)
				{
					audioCoreServiceCt.serviceState = TaskStateRunning;
				}
				break;
			case MSG_TASK_STOP:
				//Set para
				AudioCoreDeinit();
				//clear msg
				MessageClear(audioCoreServiceCt.msgHandle);
				//Set state
				audioCoreServiceCt.serviceState = TaskStateStopped;
				//reply
				msgSend.msgId		= MSG_AUDIO_CORE_SERVICE_STOPPED;
				MessageSend(audioCoreServiceCt.parentMsgHandle, &msgSend);
				break;

			default:
				break;
		}

		if(audioCoreServiceCt.serviceState == TaskStateRunning)
		{
			//audiocore 在转模式的时候可能暂停 Mark
			AudioCoreRun();
#ifdef CFG_FUNC_RECORDER_EN
			void MediaRecorderEncode(void);
			MediaRecorderEncode();
#endif
		}
		else
			AudioSinkSetForPause();
		
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN//usb path
		if((GetSystemMode() != ModeIdle)
		&& (GetSystemMode() != ModeUsbDevicePlay)
		&& (GetSystemMode() != ModeUDiskAudioPlay)
		)
		{
			if(GetUSBDeviceInitState()) 	
			{
				OTG_DeviceRequestProcess();
			}
		}
#endif
	}
}


/**
 * @brief	Get message receive handle of audio core manager
 * @param	NONE
 * @return	MessageHandle
 */
MessageHandle GetAudioCoreServiceMsgHandle(void)
{
	return audioCoreServiceCt.msgHandle;
}

TaskState GetAudioCoreServiceState(void)
{
	return audioCoreServiceCt.serviceState;
}

void SetAudioCorePause(void)
{
	audioCoreServiceCt.serviceState = TaskStatePaused;
}

/**
 * @brief	Start audio core service.
 * @param	 NONE
 * @return  
 */
int32_t AudioCoreServiceCreate(MessageHandle parentMsgHandle)
{
	int32_t		ret = 0;
	uint32_t*	sys_stack_addr = NULL;

#ifdef	USE_SYS_STACK
	sys_stack_addr = (uint32_t*)(0x20003900 - AUDIO_CORE_SERVICE_SIZE*4);//将协同stack给audiocore使用
																		 //如果改变sag的栈低，请注意修改。
	memset(sys_stack_addr, 0 ,AUDIO_CORE_SERVICE_SIZE*4);
#endif

	ret = AudioCoreServiceInit(parentMsgHandle);
	if(!ret)
	{
		audioCoreServiceCt.taskHandle = NULL;

		if(sys_stack_addr)
		{
			APP_DBG("Define USE_SYS_STACK!!!\r\n");

			xTaskGenericCreate(  AudioCoreServiceEntrance ,
								 "AudioCore" ,
								 AUDIO_CORE_SERVICE_SIZE ,
								 NULL ,
								 AUDIO_CORE_SERVICE_PRIO ,
								 &audioCoreServiceCt.taskHandle ,
								 sys_stack_addr ,
								 NULL );
		}
		else
		{
			xTaskCreate(AudioCoreServiceEntrance,
						"AudioCore",
						AUDIO_CORE_SERVICE_SIZE,
						NULL, AUDIO_CORE_SERVICE_PRIO,
						&audioCoreServiceCt.taskHandle);
		}
		if(audioCoreServiceCt.taskHandle == NULL)
		{
			ret = -1;
		}
	}
	if(ret)
	{
		APP_DBG("AudioCoreService create fail!\n");
	}
	else
	{
#ifdef SOFT_WACTH_DOG_ENABLE
		little_dog_adopt(DOG_INDEX2_AudioCoreTask);
#endif
	}
	return ret;
}

int32_t AudioCoreServiceStart(void)
{
	MessageContext		msgSend;

	audioCoreServiceCt.serviceState = TaskStateStarting;

	msgSend.msgId		= MSG_TASK_START;
	MessageSend(audioCoreServiceCt.msgHandle, &msgSend);
	return 0;
}

int32_t AudioCoreServicePause(void)
{
	MessageContext		msgSend;

	msgSend.msgId		= MSG_TASK_PAUSE;
	MessageSend(audioCoreServiceCt.msgHandle, &msgSend);
	return 0;
}

void AudioCoreServiceResume(void)
{
	MessageContext		msgSend;

	if(SoftFlagGet(SoftFlagAudioCoreSourceIsDeInit))
		return;
	
	msgSend.msgId		= MSG_TASK_RESUME;
	MessageSend(audioCoreServiceCt.msgHandle, &msgSend);
}


/**
 * @brief	Exit audio core service.
 * @param	NONE
 * @return  
 */
void AudioCoreServiceStop(void)
{
	MessageContext		msgSend;

	msgSend.msgId		= MSG_TASK_STOP;
	MessageSend(audioCoreServiceCt.msgHandle, &msgSend);
}

int32_t AudioCoreServiceKill(void)
{
	//task	先删任务，再删邮箱，收资源
	if(audioCoreServiceCt.taskHandle != NULL)
	{
		vTaskDelete(audioCoreServiceCt.taskHandle);
		audioCoreServiceCt.taskHandle = NULL;
	}
	
	//Msgbox
	if(audioCoreServiceCt.msgHandle != NULL)
	{
		MessageDeregister(audioCoreServiceCt.msgHandle);
		audioCoreServiceCt.msgHandle = NULL;
	}
#ifdef SOFT_WACTH_DOG_ENABLE
	little_dog_deadopt(DOG_INDEX2_AudioCoreTask);
#endif
	//PortFree...
	return 0;
}

