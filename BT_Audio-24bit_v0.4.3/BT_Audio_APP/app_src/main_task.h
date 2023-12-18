/**
 **************************************************************************************
 * @file    main_task.h
 * @brief   Program Entry 
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2016-6-29 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef __MAIN_TASK_H__
#define __MAIN_TASK_H__


#include "type.h"
#include "rtos_api.h"
#include "app_config.h"
#include "audio_core_api.h"
#include "app_message.h"
#include "timeout.h"
#include "mode_task.h"
#include "flash_boot.h"
#ifdef CFG_FUNC_DISPLAY_EN
#include "display.h"
#endif

#define SoftFlagMask			0xFFFFFFFF

#ifdef CFG_RES_IR_NUMBERKEY
extern bool Number_select_flag;
extern uint16_t Number_value;
extern TIMER Number_selectTimer;
#endif

typedef struct _SysVolContext
{
	bool		MuteFlag;	//AudioCore软件mute
	int8_t	 	AudioSourceVol[AUDIO_CORE_SOURCE_MAX_NUM];	//Source增益控制step，小于等于32
	int8_t	 	AudioSinkVol[AUDIO_CORE_SINK_MAX_NUM];		//Sink增益控制step，小于等于32	
	
}SysVolContext;

typedef struct _MainAppContext
{
	xTaskHandle			taskHandle;
	MessageHandle		msgHandle;
	TaskState			state;

	SysModeNumber		SysCurrentMode;
	SysModeNumber		SysPrevMode;

/*************************mode common*************************************/
#ifdef CFG_RES_AUDIO_DAC0_EN
	uint32_t			*DACFIFO;
	uint32_t			DACFIFO_LEN;
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
	uint32_t			*DACXFIFO;
	uint32_t			DACXFIFO_LEN;
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN
	uint32_t			*I2SFIFO;
	uint32_t			I2SFIFO_LEN;
#endif

#ifdef CFG_RES_AUDIO_I2S0OUT_EN
	uint32_t			*I2S0_TX_FIFO;
#endif

#ifdef CFG_RES_AUDIO_I2S1OUT_EN
	uint32_t			*I2S1_TX_FIFO;
#endif

#ifdef CFG_RES_AUDIO_I2S0IN_EN
	uint32_t			*I2S0_RX_FIFO;
#endif

#ifdef CFG_RES_AUDIO_I2S1IN_EN
	uint32_t			*I2S1_RX_FIFO;
#endif

	uint32_t			*ADCFIFO;
#ifdef BT_TWS_SUPPORT
//	PCM_DATA_TYPE		*PauseFrame;
#endif
/******************************************************************/

	AudioCoreContext 	*AudioCore;
	uint16_t			SourcesMuteState;//纪录source源mute使能情况,目前只判断left

	uint32_t 			SampleRate;

	bool				AudioCoreSync;
//#ifdef CFG_FUNC_DISPLAY_EN
//	bool				DisplaySync;
//#endif
#ifdef CFG_FUNC_ALARM_EN
	uint32_t 			AlarmID;//闹钟ID对应bit位
	bool				AlarmFlag;
	bool				AlarmRemindStart;//闹铃提示音开启标志
	uint32_t 			AlarmRemindCnt;//闹铃提示音循环次数
	#ifdef CFG_FUNC_SNOOZE_EN
	bool				SnoozeOn;
	uint32_t 			SnoozeCnt;// 贪睡时间计数
	#endif
#endif
	SysVolContext		gSysVol;
    uint8_t     MusicVolume;
#ifdef CFG_APP_BT_MODE_EN
    uint8_t     HfVolume;
#endif
	uint8_t     EffectMode;
    uint8_t     MicVolume;
	uint8_t     MicVolumeBak;
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
	uint8_t     EqMode;
	#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN   
    uint8_t     EqModeBak;
	uint8_t     EqModeFadeIn;
	uint8_t     eqSwitchFlag;
	#endif
#endif

#ifdef CFG_FUNC_MIC_AUTOTUNE_STEP_EN
	uint8_t 	MicAutoTuneStep;
	uint8_t 	MicAutoTuneStepBak;
#endif

    //uint8_t  	ReverbStep;
	uint8_t		MicEffectDelayStep;
    //uint8_t  	ReverbStepBak;
	uint8_t		MicEffectDelayStepBak;
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN	
	uint8_t 	MusicBassStep;
    uint8_t 	MusicTrebStep;
#endif

	bool	    muteFlagPre;
#ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN
	uint32_t    Silence_Power_Off_Time;
#endif

#ifdef  CFG_APP_HDMIIN_MODE_EN
	uint8_t  	hdmiArcOnFlg;
	uint8_t     hdmiSourceMuteFlg;
	uint8_t     hdmiResetFlg;
#endif

}MainAppContext;

extern MainAppContext	mainAppCt;
#define SoftFlagNoRemind				BIT(0)	//提示音故障
#define SoftFlagMediaDevicePlutOut		BIT(1)
#if FLASH_BOOT_EN 
#define SoftFlagMvaInCard				BIT(2)	//文件预搜索发现SD卡有MVA包 卡拔除时清理
#define SoftFlagMvaInUDisk				BIT(3)	//文件预搜索发现U盘有Mva包 U盘拔除时清理
#endif
#define SoftFlagDiscDelayMask			BIT(4)//通话模式,蓝牙断开连接,延时播放提示音,即退回到每个模式时播放
#define SoftFlagWaitBtRemindEnd			BIT(5)//标记来电时等待提示音播放完成再进入通话状态
#define SoftFlagDelayEnterBtHf			BIT(6)//标记延时进入通话状态
#define SoftFlagFrameSizeChange			BIT(7)//旨在登记系统帧切换流程这一状态，避免打断。
#define SoftFlagBtCurPlayStateMask		BIT(8)//标记来电时记录当前蓝牙播放的状态
#ifdef BT_TWS_SUPPORT
#define SoftFlagTwsRemind				BIT(9)//标记tws连接成功事件 等待unmute后提示音开播
#define SoftFlagTwsSlaveRemind			BIT(10)
#endif
#ifdef CFG_FUNC_BT_OTA_EN
#define SoftFlagBtOtaUpgradeOK			BIT(11)
#endif
#ifdef CFG_APP_IDLE_MODE_EN
#ifdef CFG_IDLE_MODE_DEEP_SLEEP
#define SoftFlagIdleModeEnterSleep		BIT(12)//标记进入睡眠模式
#endif
#ifdef	CFG_IDLE_MODE_POWER_KEY
#define SoftFlagIdleModeEnterPowerDown	BIT(13)
#endif
#endif
#define SoftFlagMediaModeRead			BIT(14)// 进入media mode 读一次U或SD
#define SoftFlagMediaNextOrPrev			BIT(15)// 0:next 1:prev
#define SoftFlagUpgradeOK				BIT(16)

#define SoftFlagAudioCoreSourceIsDeInit		BIT(18)	//AudioCoreSource资源已经被删除

#define SoftFlagUDiskEnum				BIT(19)	//u盘枚举标志
#define SoftFlagRecording				BIT(20)	//录音进行标记，禁止后插先播，模式切换需清理

//标记本次deepsleep消息是否来自于TV
#define SoftFlagDeepSleepMsgIsFromTV 	BIT(21)

//标记本次唤醒源是否为CEC唤醒
#define SoftFlagWakeUpSouceIsCEC BIT(22)

void SoftFlagRegister(uint32_t SoftEvent);
void SoftFlagDeregister(uint32_t SoftEvent);
bool SoftFlagGet(uint32_t SoftEvent);
int32_t MainAppTaskStart(void);
MessageHandle GetMainMessageHandle(void);
uint32_t GetSystemMode(void);

uint32_t IsBtAudioMode(void);

uint32_t IsBtTwsSlaveMode(void);
uint32_t IsIdleModeReady(void);
void PowerOffMessage(void);
void BatteryLowMessage(void);

//uint8_t Get_Resampler_Polyphase(int32_t resampler);
#endif /*__MAIN_TASK_H__*/

