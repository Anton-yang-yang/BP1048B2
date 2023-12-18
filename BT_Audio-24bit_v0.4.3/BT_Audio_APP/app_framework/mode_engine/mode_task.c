/**
 **************************************************************************************
 * @file    sys_mode.c
 * @brief   Program Entry 
 *
 * @author  ken
 * @version V1.0.0
 *
 * $Created: 2021-02-26 
 *
 * @Copyright (C) 2021, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <components/soft_watchdog/soft_watch_dog.h>
#include <string.h>
#include "type.h"
#include "app_config.h"
#include "rtos_api.h"
#include "app_message.h"
#include "debug.h"
#include "main_task.h"
#include "remind_sound.h"
#include "bt_config.h"
#include "mode_parameter.h"
#include "idle_mode.h"
#include "breakpoint.h"
#include "device_detect.h"
#include "rtos_api.h"
#include "audio_core_service.h"
#include "mode_task_api.h"
#include "audio_vol.h"
#include "recorder_service.h"
#define SYS_MODE_NUM_MESSAGE_QUEUE			20
#define SYS_MODE_MSG_TIMEOUT 				1
#define MODE_TASK_PRIO						4
#define MODE_TASK_STACK_SIZE				1024//1024

static SysModeContext SysModeCt = {NULL,NULL};
static uint32_t sPlugOutMode = BIT(ModeIdle) | BIT(ModeTwsSlavePlay) | BIT(ModeBtHfPlay);

extern void IdlePrevModeSet(SysModeNumber mode);
extern void IdleModeEnter(void);
extern void IdleModeExit(void);

osMutexId SysModeMutex = NULL;

static struct
{
	uint16_t 		Msg;
	uint16_t 		cnt;
	uint16_t 		delay;
	SysModeNumber 	EnterMode;
}EventSendAgain = {0,0,0,0};

void StartEventSendAgain(uint16_t Msg,SysModeNumber EnterMode)
{
	if(EventSendAgain.Msg != Msg)
	{
		EventSendAgain.Msg			= Msg; 			//
		EventSendAgain.cnt 			= 10;  			//重新发送次数
		EventSendAgain.delay 		= 200; 			// 发送的间隔
		EventSendAgain.EnterMode 	= EnterMode; 	//
	}	
}

bool IsEventSendAgain(void)
{
	return (EventSendAgain.Msg != 0);
}

void EventSendAgainProcess(void)
{
	if(EventSendAgain.Msg != 0)
	{
		if(GetSysModeState(EventSendAgain.EnterMode) == ModeStateInit || 
			GetSysModeState(EventSendAgain.EnterMode) == ModeStateRunning ||
			GetSysModeState(EventSendAgain.EnterMode) == ModeStateSusend
			)
		{
			EventSendAgain.Msg = 0;
		}
		else
		{
			if(EventSendAgain.delay > 0)
			{
				EventSendAgain.delay--;
				if(EventSendAgain.delay == 0)
				{
					if(EventSendAgain.cnt > 0)
					{
						MessageContext		msgSend;
						EventSendAgain.cnt--;

						msgSend.msgId		= EventSendAgain.Msg;
						MessageSend(GetMainMessageHandle(), &msgSend);
						APP_DBG("---------------------------------plug msg again 0x%x\n",EventSendAgain.Msg);
						
						if(EventSendAgain.cnt == 0)
							EventSendAgain.Msg = 0;
						else
							EventSendAgain.delay = 200;
					}
				}
			}
		}
	}
}


/**
 * @brief printf mode string
 * @param Mode:ModeBtAudioPlay etc...
 * @return none
 */
static  const char* GetModeNameStr(SysModeNumber Mode)
{

	uint32_t i_count = 0;

	for(;i_count < MODE_STR_AND_REMIND_MAX_NUMBER;i_count++)
	{
		if(Mode == (ModeNameStr[i_count]).ModeNumber)
		{
			return (ModeNameStr[i_count]).ModeName;
		}
	}
	return "unknow mode";
	
}

/**
 * @brief mode task creat
 * @param none
 * @return none
 */
void SysModeTaskCreat(void)
{	
	if(xTaskCreate(SysModeEntrance, "SysMode", MODE_TASK_STACK_SIZE, NULL, MODE_TASK_PRIO, &SysModeCt.taskHandle) != pdTRUE)
	{
		vTaskDelay(50);
		APP_DBG("system error ,mode task create fail 1\n");
		if(xTaskCreate(SysModeEntrance, "SysMode", MODE_TASK_STACK_SIZE, NULL, MODE_TASK_PRIO, &SysModeCt.taskHandle) != pdTRUE)
		{
			while(1)
			{
				vTaskDelay(50);
				APP_DBG("system error ,mode task create fail 2\n");
			}
		}
	}
}

/**
 * @brief mode task delete
 * @param none
 * @return none
 */
static void SysModeTaskKill(void)
{
	MessageHandle		msgHandleTemp = NULL;
	
	if(SysModeCt.taskHandle == NULL)
	{
		return;
	}
	msgHandleTemp =	SysModeCt.msgHandle;
	SysModeCt.msgHandle = NULL;
	if(msgHandleTemp != NULL)
	{
		MessageClear(msgHandleTemp);
	}
	if(ModeInputFunction.UdiskUnlock)
	{
		ModeInputFunction.UdiskUnlock();
	}
	if(ModeInputFunction.SDCardForceExit)
	{
		ModeInputFunction.SDCardForceExit();
	}
	vTaskDelete(SysModeCt.taskHandle);
	SysModeCt.taskHandle = NULL;
	MessageDeregister(msgHandleTemp);
	while(!TaskDeleteCompleted());
	
}

/**
 * @brief get mode message handle
 * @param none
 * @return message handle
 */
MessageHandle GetSysModeMsgHandle(void)
{
	return SysModeCt.msgHandle;
}

/**
 * @brief get the mode index in the SysMode[]
 * @param mode varible pointer
 * @return mode index in the SysMode[]
 */
static uint32_t GetModeIndexInModeLoop(SysModeNumber *sys_mode)
{
	uint32_t mode_index=0;
	for(;mode_index < SYS_MODE_MAX_NUMBER;mode_index++)
	{
		if(SysMode[mode_index].ModeNumber == *sys_mode)// find the index in sysmode
			return mode_index;
	}
	
	*sys_mode = SysMode[SYS_MODE_MAX_NUMBER-1].ModeNumber;
	return SYS_MODE_MAX_NUMBER-1;
}

/**
 * @brief get power on mode and set it to mainAppCt.PowerOnMode
 * @param mode varible pointer
 * @return mode index in the SysMode[]
 */
void PowerOnModeGenerate(void *BpSysInfo)
{
	uint32_t i_count = ModeIdle;
	uint32_t default_count = SYS_MODE_MAX_NUMBER-1;
	SysModeNumber	Mode = ModeIdle;

#ifdef CFG_FUNC_BREAKPOINT_EN
	BP_SYS_INFO * pBpSysInfo = BpSysInfo;
	if(pBpSysInfo !=NULL)
	{
		if((pBpSysInfo->CurModuleId > ModeIdle)&&(pBpSysInfo->CurModuleId < SysModeMax))
		{
			Mode = pBpSysInfo->CurModuleId; 
		}
	}
#endif
	for(i_count = 0;i_count < SYS_MODE_MAX_NUMBER;i_count++)
	{
		if(SysMode[i_count].ModeState == ModeStateInit)
		{
			SysMode[i_count].ModeState = ModeStateReady;
			if(default_count == SYS_MODE_MAX_NUMBER-1)
				default_count = i_count;	
		}
	}	

	mainAppCt.SysPrevMode = SysMode[SYS_MODE_MAX_NUMBER-1].ModeNumber;
	
	if(Mode == ModeIdle)// Do not have memory mode, set a default mode 	
	{
		Mode = SysMode[default_count].ModeNumber;
	}
	
	APP_DBG("power on mode %s\n",GetModeNameStr(Mode));

	if(GetModeDefineState(ModeIdle))
	{
		mainAppCt.SysPrevMode = Mode;
		mainAppCt.SysCurrentMode = ModeIdle;
	}
	else
	{
		mainAppCt.SysCurrentMode = Mode;
	}
	SysMode[GetModeIndexInModeLoop(&mainAppCt.SysCurrentMode)].ModeState = ModeStateInit;

	IdlePrevModeSet(Mode);
}

/**
 * @brief set mode state 
 * @param sys_mode: mode ,sys_mode_state:mode state,example ModeStateReady
 * @return none
 */
void SetSysModeState(SysModeNumber sys_mode,SysModeState sys_mode_state)
{
	SysMode[GetModeIndexInModeLoop(&sys_mode)].ModeState=sys_mode_state;
}

/**
 * @brief get mode state 
 * @param sys_mode: mode 
 * @return mode state,example ModeStateReady
 */
SysModeState GetSysModeState(SysModeNumber sys_mode)
{
	return SysMode[GetModeIndexInModeLoop(&sys_mode)].ModeState;
}

bool GetModeDefineState(SysModeNumber sys_mode)
{
	uint32_t i_count = 0;
	for(;i_count < SYS_MODE_MAX_NUMBER ;i_count++)
	{
		if(SysMode[i_count].ModeNumber == sys_mode)return TRUE;
	}
	return FALSE;
}
/**
 * @brief send mode msg to main task
 * @param none
 * @return none
 */
void SendModeKeyMsg(void)
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_SOFT_MODE;
	MessageSend(GetMainMessageHandle(), &msgSend);
}

/**
 * @brief mode enter
 * @param SetMode:which mode you want to enter
 * @return none
 * @note : setmode must be ModeStateReady state.if (setmode==ModeStateSusend), please : SetSysModeState(setmode,ModeStateReady)
*/
void SysModeEnter(SysModeNumber SetMode)
{
	gChangeModeTimeOutTimer = CHANGE_MODE_TIMEOUT_COUNT;
	if(GetSysModeState(mainAppCt.SysCurrentMode) == ModeStateDeinit)
	{
		return;
	}
	if(GetSysModeState(SetMode) == ModeStateSusend)
	{
		SendModeKeyMsg();
		return;
	}
	if(GetSysModeState(SetMode) == ModeStateInit || GetSysModeState(SetMode) == ModeStateDeinit || GetSysModeState(SetMode) == ModeStateRunning )
	{
		return;	
	}

	if(GetSysModeState(mainAppCt.SysCurrentMode) == ModeStateInit || GetSysModeState(mainAppCt.SysCurrentMode) == ModeStateRunning)
	{
		SysMode[GetModeIndexInModeLoop(&mainAppCt.SysCurrentMode)].ModeState = ModeStateDeinit;
	}
	
	SysMode[GetModeIndexInModeLoop(&SetMode)].ModeState = ModeStateInit;
}

/**
 * @brief plug event process
 * @param Msg:plug event msg ,for example MSG_DEVICE_SERVICE_U_DISK_IN
 * @return none
 */
static void SysModeGenerateByPlugEvent(uint16_t Msg)
{
	uint32_t i_count=0;
	uint32_t i;
	
	for(;i_count < SYS_DEVICE_DETECT_MAX_NUMBER;i_count++)
	{
		if(Msg == DeviceEventMsgTableArray[i_count].MsgID)
		{
			osMutexLock(SysModeMutex);
			for(i=0;i < SYS_MODE_MAX_NUMBER;i++)
			{
				SysModeNumber mode = SysMode[i].ModeNumber;
				//不允许插拔事件的模式
				if( ((!(BIT(mode) & DeviceEventMsgTableArray[i_count].SupportMode)) &&
					(GetSysModeState(mode) == ModeStateInit || GetSysModeState(mode) == ModeStateRunning))
#ifdef	CFG_FUNC_RECORDER_EN
					|| SoftFlagGet(SoftFlagRecording)	//录音屏蔽拔插事件
#endif
					)
				{
					break;
				}
			}
			if((i >= SYS_MODE_MAX_NUMBER) && (!gInsertEventDelayActTimer))
			{
				APP_DBG("plug msg 0x%x and msg total number %ld\n",Msg,SYS_DEVICE_DETECT_MAX_NUMBER);
				if(DeviceEventMsgTableArray[i_count].EnterMode != ENTERR_PREV_MODE)
				{
					SysModeEnter(DeviceEventMsgTableArray[i_count].EnterMode);
					// plug in进入模式失败，启动消息重新发送
					if(GetSysModeState(DeviceEventMsgTableArray[i_count].EnterMode) != ModeStateInit)
					{
						StartEventSendAgain(Msg,DeviceEventMsgTableArray[i_count].EnterMode);
					}
				}
				else
				{
					sPlugOutMode |= BIT(mainAppCt.SysCurrentMode);
					SysModeEnter(mainAppCt.SysPrevMode);
				}
				osMutexUnlock(SysModeMutex);
				break;
			}
			osMutexUnlock(SysModeMutex);
		}
	}

}

/**
 * @brief all mode generate except PowerOnModeGenerate()
 * @param Msg:plug event msg ,for example MSG_DEVICE_SERVICE_U_DISK_IN
 * @return none
 */
void SysModeGenerate(uint16_t Msg)
{
	uint32_t count=0;
	SysModeNumber mode_search_count=0;
	
	if(Msg == MSG_MODE||Msg ==MSG_SOFT_MODE)
	{
		if(Msg == MSG_MODE)
		{
			if(IsEventSendAgain()) // 已经在plug in消息重发流程中 暂时屏蔽按键消息
				return;
			for(count = 0;count < MODE_KEY_INVALID_MAX_NUMBER;count++)
			{
				if(GetSysModeState(ModeKeyInvalid[count]) == ModeStateInit || GetSysModeState(ModeKeyInvalid[count]) == ModeStateRunning)
				{
					return;
				}
			}
		}
		SoftFlagRegister(SoftFlagUpgradeOK);
		osMutexLock(SysModeMutex);
		count = 0;
		mode_search_count = GetModeIndexInModeLoop(&mainAppCt.SysCurrentMode);// find the index in sysmode
		do
		{//find the ready mode
			count++;
			mode_search_count++;
			if(mode_search_count >= SYS_MODE_MAX_NUMBER)
			{
				mode_search_count=0;
			}
		}while(SysMode[mode_search_count].ModeState!=ModeStateReady&&count < SYS_MODE_MAX_NUMBER);
		
		if(count == SYS_MODE_MAX_NUMBER)
		{
			osMutexUnlock(SysModeMutex);
			return;// not find mode,not action
		}
		SysModeEnter(SysMode[mode_search_count].ModeNumber);
		osMutexUnlock(SysModeMutex);
	}
	else if(Msg == MSG_ENTER_IDLE_MODE)
	{
		IdleModeEnter();
	}
	else if(Msg == MSG_QUIT_IDLE_MODE)
	{
		IdleModeExit();
	}
	else
	{
		SysModeGenerateByPlugEvent(Msg);
	}
	
}

/**
 * @brief manage change mode time out 
 * @param none
 * @return none
 */
void SysModeChangeTimeoutProcess(void)
{
	if(SysMode[GetModeIndexInModeLoop(&mainAppCt.SysCurrentMode)].ModeState == ModeStateDeinit)
	{
		if(gChangeModeTimeOutTimer == 0)
		{
			osMutexLock(SysModeMutex);
			SysModeTaskKill();			
			APP_DBG("create mode task again\n");
			SysModeTaskCreat();
			osSemaphoreMutexUnlock();
			gChangeModeTimeOutTimer = CHANGE_MODE_TIMEOUT_COUNT;
		}
	}
	else
	{
		gChangeModeTimeOutTimer = CHANGE_MODE_TIMEOUT_COUNT;
	}		
}

extern bool AudioCoreSinkMutedForClear(void);
/**
 * @brief mode de initialization
 * @param none
 * @return none
 */
static void SysModeDeinit(void)
{
	uint32_t Deinit_count=0;
	for(;Deinit_count < SYS_MODE_MAX_NUMBER;Deinit_count++)
	{
		if(SysMode[Deinit_count].ModeState == ModeStateDeinit)
		{
			TIMER time_out;
			gChangeModeTimeOutTimer = CHANGE_MODE_SYS_REMIND;
			osMutexLock(SysModeMutex);
			if(ModeInputFunction.RemindRun)
			{
				TimeOutSet(&time_out,1000);
				while(ModeInputFunction.RemindRun(ModeStateDeinit) && (!IsTimeOut(&time_out)));
				if(IsTimeOut(&time_out))
					APP_DBG("RemindRun Deinit time out!\n");
			}

			APP_DBG("--Deinit mode-- %s\n",GetModeNameStr(SysMode[Deinit_count].ModeNumber));
			gChangeModeTimeOutTimer = CHANGE_MODE_TIMEOUT_COUNT*7;
#ifdef CFG_FUNC_RECORDER_EN
			if(GetMediaRecorderTaskHandle() != NULL)
			{
				APP_DBG("media rec task del\n");
				MediaRecorderServiceDeinit();
			}
#endif
			SysMode[Deinit_count].SysModeDeInit();
			if(SysModeCt.msgHandle != NULL)
			{
				MessageClear(SysModeCt.msgHandle);
			}
			if(sPlugOutMode&(BIT(SysMode[Deinit_count].ModeNumber)))
			{
				SysMode[Deinit_count].ModeState = ModeStateSusend;
				sPlugOutMode &= ~(BIT(SysMode[Deinit_count].ModeNumber));
				sPlugOutMode = BIT(ModeIdle) | BIT(ModeTwsSlavePlay) | BIT(ModeBtHfPlay);	
			}
			else
			{
#ifdef CFG_FUNC_RECORDER_EN
				if(SysMode[Deinit_count].ModeNumber == ModeCardPlayBack || SysMode[Deinit_count].ModeNumber == ModeUDiskPlayBack)
				{
					SysMode[Deinit_count].ModeState = ModeStateSusend;
				}else
#endif
				{
					SysMode[Deinit_count].ModeState = ModeStateReady;
				}
			}
			mainAppCt.SysPrevMode = SysMode[Deinit_count].ModeNumber;
			gChangeModeTimeOutTimer = CHANGE_MODE_TIMEOUT_COUNT * 2;
			TimeOutSet(&time_out,1000);
			while(!AudioCoreSinkMutedForClear() && (!IsTimeOut(&time_out)));
			if(IsTimeOut(&time_out))
				APP_DBG("AudioCoreSinkMutedForClear time out!\n");
			osMutexUnlock(SysModeMutex);
		}
	}
	
}

/**
 * @brief mode  initialization
 * @param none
 * @return none
 */
static void SysModeInit(void)
{
	uint32_t init_count=0;
	uint32_t count;

	for(init_count = 0;init_count < SYS_MODE_MAX_NUMBER;init_count++)
	{
		if(SysMode[init_count].ModeState == ModeStateInit)
		{
			osMutexLock(SysModeMutex);
			//先判断有没有模式需要 Deinit ，有的话直接返回等下一个轮回在init
			for(count = 0;count < SYS_MODE_MAX_NUMBER;count++)
			{
				if(SysMode[count].ModeState == ModeStateDeinit)
				{
					osMutexUnlock(SysModeMutex);
					return;
				}
			}			
			mainAppCt.SysCurrentMode = SysMode[init_count].ModeNumber;
			if(ModeInputFunction.RemindRun)
			{
				ModeInputFunction.RemindRun(ModeStateInit);
			}
			#ifdef TWS_SLAVE_MODE_SWITCH_EN
			extern void TwsSlaveModeSwitchDeal(SysModeNumber pre, SysModeNumber Cur);
			TwsSlaveModeSwitchDeal(mainAppCt.SysPrevMode, mainAppCt.SysCurrentMode);
			#endif
			
			APP_DBG("\n SysModeInit-----SysPrevMode = %s SysCurrentMode = %s-----\n",GetModeNameStr(mainAppCt.SysPrevMode),GetModeNameStr(mainAppCt.SysCurrentMode));
			if(SysMode[init_count].SysModeInit() == TRUE)
			{
				SysMode[init_count].ModeState = ModeStateRunning;	
				SoftFlagDeregister(SoftFlagAudioCoreSourceIsDeInit);
				if(ModeInputFunction.AudioCoreResume)
				{
					ModeInputFunction.AudioCoreResume();
				}
				
			}
			else
			{				
				SendModeKeyMsg();
				osMutexUnlock(SysModeMutex);
				APP_DBG("mode init failure,and search next mode\n");// send  MSG_MODE msg 	
				break;
			}
			osMutexUnlock(SysModeMutex);
		}
	}
}

/**
 * @brief mode  run
 * @param none
 * @return none
 */
static void SysModeRun(uint16_t MsgId)
{
	uint32_t i_count=0;
	for(i_count = 0;i_count < SYS_MODE_MAX_NUMBER;i_count++)
	{
		if(SysMode[i_count].ModeState == ModeStateDeinit)
		{
			return;
		}
	}
	for(i_count = 0;i_count < SYS_MODE_MAX_NUMBER;i_count++)
	{
		if(SysMode[i_count].ModeState == ModeStateRunning)
		{
			SysMode[i_count].SysModeRun(MsgId);
		}
	}
}

/**
 * @brief mode  task
 * @param none
 * @return none
 */
static void SysModeEntrance(void * param)
{
	MessageContext		msg;
	
	SysModeCt.msgHandle = MessageRegister(SYS_MODE_NUM_MESSAGE_QUEUE);
	
	while(1)
	{
		MessageRecv(SysModeCt.msgHandle, &msg, SYS_MODE_MSG_TIMEOUT);
#ifdef SOFT_WACTH_DOG_ENABLE
		little_dog_feed(DOG_INDEX1_ModeTask);
#endif

		SysModeDeinit();
		SysModeInit();
		if(ModeInputFunction.RemindRun)
		{
			ModeInputFunction.RemindRun(ModeStateRunning);
		}
#ifdef CFG_FUNC_RECORDER_EN
		RecServierMsg(&msg.msgId);
#endif
		SysModeRun(msg.msgId);
		EventSendAgainProcess();
	}
}

