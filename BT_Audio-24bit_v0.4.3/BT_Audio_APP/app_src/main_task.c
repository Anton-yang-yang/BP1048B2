/**
 **************************************************************************************
 * @file    main_task.c
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
#include <string.h>
#include "type.h"
#include "app_config.h"
#include "timeout.h"
#include "rtos_api.h"
#include "app_message.h"
#include "debug.h"
#include "dma.h"
#include "clk.h"
#include "main_task.h"
#include "timer.h"
#include "irqn.h"
#include "watchdog.h"
#include "dac.h"
#include "ctrlvars.h"
#include "delay.h"
#include "efuse.h"
#include "uarts_interface.h"
//functions
#include "powercontroller.h"
#include "deepsleep.h"
#include "flash_boot.h"
#include "breakpoint.h"
#include "remind_sound.h"
#include "rtc_alarm.h"
#include "rtc.h"
#include "audio_vol.h"
#include "device_detect.h"
#include "otg_detect.h"
#include "sadc_interface.h"
#include "adc.h"
#include "adc_key.h"
#include "ir_key.h"
#include "otg_device_hcd.h"
#include "otg_device_standard_request.h"
//services
#include "shell.h"
#include "audio_core_service.h"
#include "audio_core_api.h"
#include "bt_stack_service.h"
#include "bb_api.h"
#include "bt_config.h"
#include "bt_app_ddb_info.h"
#ifdef BT_TWS_SUPPORT
#include "bt_tws_api.h"
#include "bt_app_tws.h"
#endif
#include "bt_hf_mode.h"
#include "idle_mode.h"
#include "bt_app_tws_connect.h"
#include "tws_mode.h"
#include "bt_app_connect.h"
#include "backup_interface.h"
#include <components/soft_watchdog/soft_watch_dog.h>
#include "display_task.h"

uint32_t SoftwareFlag;// soft flag register
MainAppContext	mainAppCt;
#ifdef CFG_RES_IR_NUMBERKEY
bool Number_select_flag = 0;
uint16_t Number_value = 0;
TIMER Number_selectTimer;
#endif

extern volatile uint32_t gInsertEventDelayActTimer;
extern void BtTwsDisconnectApi(void);
#ifdef BT_TWS_SUPPORT
extern void tws_master_mute_send(bool MuteFlag);
#endif
#if FLASH_BOOT_EN
void start_up_grate(uint32_t UpdateResource);
#endif
extern void PowerOnModeGenerate(void *BpSysInfo);
#ifdef CFG_APP_HDMIIN_MODE_EN
extern void HDMI_CEC_ActivePowerOff(uint32_t timeout_value);
extern void	HDMI_CEC_DDC_Init(void);
#endif

#define MAIN_APP_TASK_STACK_SIZE		1024//512//1024
#ifdef	CFG_FUNC_OPEN_SLOW_DEVICE_TASK
#define MAIN_APP_TASK_PRIO				4
#define MAIN_APP_MSG_TIMEOUT			10	
#else
#define MAIN_APP_TASK_PRIO				3
#define MAIN_APP_MSG_TIMEOUT			1	
#endif
#define MAIN_APP_TASK_SLEEP_PRIO		6 //进入deepsleep 需要相对其他task最高优先级。
#define MAIN_NUM_MESSAGE_QUEUE			20
#define SHELL_TASK_STACK_SIZE			512//1024
#define SHELL_TASK_PRIO					2


/**根据appconfig缺省配置:DMA 8个通道配置**/
/*1、cec需PERIPHERAL_ID_TIMER3*/
/*2、SD卡录音需PERIPHERAL_ID_SDIO RX/TX*/
/*3、在线串口调音需PERIPHERAL_ID_UART1 RX/TX,建议使用USB HID，节省DMA资源*/
/*4、线路输入需PERIPHERAL_ID_AUDIO_ADC0_RX*/
/*5、Mic开启需PERIPHERAL_ID_AUDIO_ADC1_RX，mode之间通道必须一致*/
/*6、Dac0开启需PERIPHERAL_ID_AUDIO_DAC0_TX mode之间通道必须一致*/
/*7、DacX需开启PERIPHERAL_ID_AUDIO_DAC1_TX mode之间通道必须一致*/
/*注意DMA 8个通道配置冲突:*/
/*a、默认在线调音使用USB HID*/

static const uint8_t DmaChannelMap[29] = {
	255,//PERIPHERAL_ID_SPIS_RX = 0,	//0
	255,//PERIPHERAL_ID_SPIS_TX,		//1
#ifdef CFG_APP_HDMIIN_MODE_EN
	5,//PERIPHERAL_ID_TIMER3,			//2
#else
	255,//PERIPHERAL_ID_TIMER3,			//2
#endif

	255,//PERIPHERAL_ID_SDIO_RX,			//3
	255,//PERIPHERAL_ID_SDIO_TX,			//4

	255,//PERIPHERAL_ID_UART0_RX,		//5
	255,//PERIPHERAL_ID_TIMER1,			//6
	255,//PERIPHERAL_ID_TIMER2,			//7
	255,//PERIPHERAL_ID_SDPIF_RX,		//8 SPDIF_RX /TX same chanell
	255,//PERIPHERAL_ID_SDPIF_TX,		//8 SPDIF_RX /TX same chanell
	255,//PERIPHERAL_ID_SPIM_RX,		//9
	255,//PERIPHERAL_ID_SPIM_TX,		//10
	255,//PERIPHERAL_ID_UART0_TX,		//11
	255,//PERIPHERAL_ID_UART1_RX,		//12
	255,//PERIPHERAL_ID_UART1_TX,		//13
	255,//PERIPHERAL_ID_TIMER4,			//14
	255,//PERIPHERAL_ID_TIMER5,			//15
	255,//PERIPHERAL_ID_TIMER6,			//16
	0,//PERIPHERAL_ID_AUDIO_ADC0_RX,	//17
	1,//PERIPHERAL_ID_AUDIO_ADC1_RX,	//18
	2,//PERIPHERAL_ID_AUDIO_DAC0_TX,	//19
	3,//PERIPHERAL_ID_AUDIO_DAC1_TX,	//20

#if defined (CFG_FUNC_I2S_MIX_MODE) && defined (CFG_RES_AUDIO_I2S0IN_EN)	
	6,//PERIPHERAL_ID_I2S0_RX,			//21
#else
	255,//PERIPHERAL_ID_I2S0_RX,		//21
#endif

#if	((defined(CFG_RES_AUDIO_I2SOUT_EN )&&(CFG_RES_I2S == 0)) || defined(CFG_RES_AUDIO_I2S0OUT_EN))
	7,//PERIPHERAL_ID_I2S0_TX,			//22
#else	
	255,//PERIPHERAL_ID_I2S0_TX,		//22
#endif	

#if defined (CFG_FUNC_I2S_MIX_MODE) && defined (CFG_RES_AUDIO_I2S1IN_EN)	
	4,//PERIPHERAL_ID_I2S1_RX,			//23
#else
	255,//PERIPHERAL_ID_I2S1_RX,		//23
#endif

#if	((defined(CFG_RES_AUDIO_I2SOUT_EN )&&(CFG_RES_I2S == 1))|| defined(CFG_RES_AUDIO_I2S1OUT_EN))
	5,	//PERIPHERAL_ID_I2S1_TX,		//24
#else
	255,//PERIPHERAL_ID_I2S1_TX,		//24
#endif

	255,//PERIPHERAL_ID_PPWM,			//25
	255,//PERIPHERAL_ID_ADC,     		//26
	255,//PERIPHERAL_ID_SOFTWARE,		//27
};

static void MainAppInit(void)
{
	memset(&mainAppCt, 0, sizeof(MainAppContext));

	mainAppCt.msgHandle = MessageRegister(MAIN_NUM_MESSAGE_QUEUE);
	mainAppCt.state = TaskStateCreating;
	mainAppCt.SysCurrentMode = ModeIdle;
	mainAppCt.SysPrevMode = ModeIdle;
}

static void SysVarInit(void)
{
	int16_t i;

#ifdef CFG_FUNC_BREAKPOINT_EN
	BP_SYS_INFO *pBpSysInfo = NULL;

	pBpSysInfo = (BP_SYS_INFO *)BP_GetInfo(BP_SYS_INFO_TYPE);

	
#ifdef BT_TWS_SUPPORT
	if(pBpSysInfo->CurModuleId != ModeBtAudioPlay)
	{
		tws_delay = BT_TWS_DELAY_DEINIT;
	}
	else
	{
		tws_delay = BT_TWS_DELAY_BTMODE;
	}
#endif
	mainAppCt.MusicVolume = pBpSysInfo->MusicVolume;
	if((mainAppCt.MusicVolume > CFG_PARA_MAX_VOLUME_NUM) || (mainAppCt.MusicVolume <= 0))
	{
		mainAppCt.MusicVolume = CFG_PARA_MAX_VOLUME_NUM;
	}
	APP_DBG("MusicVolume:%d,%d\n", mainAppCt.MusicVolume, pBpSysInfo->MusicVolume);	

#ifdef CFG_EFFECT_PARAM_IN_FLASH_EN
	mainAppCt.EffectMode = EFFECT_MODE_FLASH_Music;
#else
	mainAppCt.EffectMode = EFFECT_MODE_NORMAL;
#endif	
	APP_DBG("EffectMode:%d,%d\n", mainAppCt.EffectMode, pBpSysInfo->EffectMode);
	
	mainAppCt.MicVolume = pBpSysInfo->MicVolume;
	if((mainAppCt.MicVolume > CFG_PARA_MAX_VOLUME_NUM) || (mainAppCt.MicVolume <= 0))
	{
		mainAppCt.MicVolume = CFG_PARA_MAX_VOLUME_NUM;
	}
	mainAppCt.MicVolumeBak = mainAppCt.MicVolume;
	APP_DBG("MicVolume:%d,%d\n", mainAppCt.MicVolume, pBpSysInfo->MicVolume);

	#ifdef CFG_APP_BT_MODE_EN
	mainAppCt.HfVolume = pBpSysInfo->HfVolume;
	if((mainAppCt.HfVolume > CFG_PARA_MAX_VOLUME_NUM) || (mainAppCt.HfVolume <= 0))
	{
		mainAppCt.HfVolume = CFG_PARA_MAX_VOLUME_NUM;
	}
	APP_DBG("HfVolume:%d,%d\n", mainAppCt.HfVolume, pBpSysInfo->HfVolume);
	#endif
	
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
	mainAppCt.EqMode = pBpSysInfo->EqMode;
	if(mainAppCt.EqMode > EQ_MODE_VOCAL_BOOST)
	{
		mainAppCt.EqMode = EQ_MODE_FLAT;
	}
	EqModeSet(mainAppCt.EqMode);
	#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN    
	mainAppCt.EqModeBak = mainAppCt.EqMode;
	mainAppCt.EqModeFadeIn = 0;
	mainAppCt.eqSwitchFlag = 0;
	#endif
	APP_DBG("EqMode:%d,%d\n", mainAppCt.EqMode, pBpSysInfo->EqMode);
#endif

	mainAppCt.MicEffectDelayStep = pBpSysInfo->MicEffectDelayStep;
    if((mainAppCt.MicEffectDelayStep > MAX_MIC_EFFECT_DELAY_STEP) || (mainAppCt.MicEffectDelayStep <= 0))
	{
		mainAppCt.MicEffectDelayStep = MAX_MIC_EFFECT_DELAY_STEP;
	}
	mainAppCt.MicEffectDelayStepBak = mainAppCt.MicEffectDelayStep;
	APP_DBG("MicEffectDelayStep:%d,%d\n", mainAppCt.MicEffectDelayStep, pBpSysInfo->MicEffectDelayStep);
	
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN	
    mainAppCt.MusicBassStep = pBpSysInfo->MusicBassStep;
    if((mainAppCt.MusicBassStep > MAX_MUSIC_DIG_STEP) || (mainAppCt.MusicBassStep <= 0))
	{
		mainAppCt.MusicBassStep = 7;
	}
    mainAppCt.MusicTrebStep = pBpSysInfo->MusicTrebStep;
    if((mainAppCt.MusicTrebStep > MAX_MUSIC_DIG_STEP) || (mainAppCt.MusicTrebStep <= 0))
	{
		mainAppCt.MusicTrebStep = 7;
	}
	APP_DBG("MusicTrebStep:%d,%d\n", mainAppCt.MusicTrebStep, pBpSysInfo->MusicTrebStep);
	APP_DBG("MusicBassStep:%d,%d\n", mainAppCt.MusicBassStep, pBpSysInfo->MusicBassStep);
#endif

#else
	//mainAppCt.appBackupMode = ModeBtAudioPlay;		  
	mainAppCt.MusicVolume = CFG_PARA_MAX_VOLUME_NUM;
	#ifdef CFG_APP_BT_MODE_EN
	mainAppCt.HfVolume = CFG_PARA_MAX_VOLUME_NUM;
	#endif
	
	#ifdef CFG_EFFECT_PARAM_IN_FLASH_EN
	mainAppCt.EffectMode = EFFECT_MODE_FLASH_Music;
	#else
	mainAppCt.EffectMode = EFFECT_MODE_NORMAL;
	#endif	
	mainAppCt.MicVolume = CFG_PARA_MAX_VOLUME_NUM;
	
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
	mainAppCt.EqMode = EQ_MODE_FLAT;
 	#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN    
    mainAppCt.EqModeBak = mainAppCt.EqMode;
	mainAppCt.eqSwitchFlag = 0;
	#endif
#endif
	//mainAppCt.ReverbStep = MAX_MIC_DIG_STEP;

#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN	
	mainAppCt.MusicBassStep = 7;
	mainAppCt.MusicTrebStep = 7;
#endif	

#endif

#ifdef CFG_APP_HDMIIN_MODE_EN
	mainAppCt.hdmiArcOnFlg = 1;
	mainAppCt.hdmiResetFlg = 0;
#endif

#ifdef CFG_FUNC_BREAKPOINT_EN
#ifdef CFG_APP_BT_MODE_EN
	if(SoftFlagGet(SoftFlagUpgradeOK))
	{
		pBpSysInfo->CurModuleId = ModeBtAudioPlay;
		DBG("u or sd upgrade ok ,set mode to ModeBtAudioPlay \n");
	}
#endif
	PowerOnModeGenerate((void *)pBpSysInfo);
#else
	PowerOnModeGenerate(NULL);
#endif


    for(i = 0; i < AUDIO_CORE_SINK_MAX_NUM; i++)
	{
		mainAppCt.gSysVol.AudioSinkVol[i] = CFG_PARA_MAX_VOLUME_NUM;
	}

	for(i = 0; i < AUDIO_CORE_SOURCE_MAX_NUM; i++)
	{
		if(i == MIC_SOURCE_NUM)
		{
			mainAppCt.gSysVol.AudioSourceVol[MIC_SOURCE_NUM] = mainAppCt.MicVolume;
		}
		else if(i == APP_SOURCE_NUM)
		{
			mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM] = mainAppCt.MusicVolume;
		}
#ifdef CFG_FUNC_REMIND_SOUND_EN
		else if(i == REMIND_SOURCE_NUM)
		{
			#if CFG_PARAM_FIXED_REMIND_VOL
			mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM] = CFG_PARAM_FIXED_REMIND_VOL;
			#else
			mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM] = mainAppCt.MusicVolume;
			#endif
		}
#endif
#ifdef BT_TWS_SUPPORT
//		else if(i == TWS_SOURCE_NUM)
//		{
//			mainAppCt.gSysVol.AudioSourceVol[TWS_SOURCE_NUM] = mainAppCt.MusicVolume;
//		}
#endif
		else
		{
			mainAppCt.gSysVol.AudioSourceVol[i] = CFG_PARA_MAX_VOLUME_NUM;		
		}
	}
	mainAppCt.gSysVol.MuteFlag = TRUE;	
	mainAppCt.muteFlagPre = FALSE;
	
	#ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN
	mainAppCt.Silence_Power_Off_Time = 0;
	#endif
}

static void SystemInit(void)
{
	int16_t i;


	DelayMsFunc = (DelayMsFunction)vTaskDelay; //提高Os条件下驱动层延时函数精度，非OS默认使用DelayMs
	DMA_ChannelAllocTableSet((uint8_t*)DmaChannelMap);

	SarADC_Init();

	//For OTG check
#if defined(CFG_FUNC_UDISK_DETECT)
#if defined(CFG_FUNC_USB_DEVICE_EN)
 	OTG_PortSetDetectMode(1,1);
#else
 	OTG_PortSetDetectMode(1,0);
#endif
#else
#if defined(CFG_FUNC_USB_DEVICE_EN)
 	OTG_PortSetDetectMode(0,1);
#else
 	OTG_PortSetDetectMode(0,0);
#endif
#endif
 	Timer_Config(TIMER2,1000,0);
 	Timer_Start(TIMER2);
 	NVIC_EnableIRQ(Timer2_IRQn);

#ifdef CFG_FUNC_BREAKPOINT_EN
 	BP_LoadInfo();
#endif

	SysVarInit();
	
#ifdef CFG_FUNC_BT_OTA_EN
	SoftFlagDeregister((~(SoftFlagUpgradeOK|SoftFlagBtOtaUpgradeOK))&SoftFlagMask);
#else
	SoftFlagDeregister((~SoftFlagUpgradeOK)&SoftFlagMask);
#endif

	CtrlVarsInit();//音频系统硬件变量初始化，系统变量初始化

#ifdef BT_TWS_SUPPORT
	TWS_Params_Init();
#endif	

#ifdef CFG_APP_HDMIIN_MODE_EN
	HDMI_CEC_DDC_Init();
#endif
	///////////////////////////////AudioCore/////////////////////////////////////////
	mainAppCt.AudioCore =  (AudioCoreContext*)&AudioCore;
	memset(mainAppCt.AudioCore, 0, sizeof(AudioCoreContext));
	for(i = 0; i < MaxNet; i++)
	{
		AudioCoreFrameSizeSet(i, CFG_PARA_SAMPLES_PER_FRAME);//默认数据帧大小
		AudioCoreMixSampleRateSet(i, CFG_PARA_SAMPLE_RATE);//默认系统采样率
	}
	mainAppCt.SampleRate = CFG_PARA_SAMPLE_RATE;
	
	////Audio Core音量配置
	SystemVolSet();
	
	for( i = 0; i < AUDIO_CORE_SOURCE_MAX_NUM; i++)
	{
	   mainAppCt.AudioCore->AudioSource[i].PreGain = 4095;//默认使用4095， 0dB
	}

	AudioCoreServiceCreate(mainAppCt.msgHandle);
	mainAppCt.AudioCoreSync = FALSE;
#ifdef CFG_FUNC_REMIND_SOUND_EN	
	RemindSoundInit();
#endif

#ifdef CFG_FUNC_ALARM_EN
	mainAppCt.AlarmFlag = FALSE;
#endif

#ifdef CFG_APP_BT_MODE_EN
	//将蓝牙任务创建移至此处,以便优先申请协议栈使用的内存空间,不影响其他的任务; 开机睡眠时，蓝牙stack再次开，避免上电马上退出。
	if(sys_parameter.bt_BackgroundType != BT_BACKGROUND_DISABLE)
		BtStackServiceStart();//蓝牙设备驱动serivce 启动失败时，目前是挂起，无同步消息。
//IRKeyInit();//clk源被改？
#endif

	DeviceServiceInit();
#ifdef BT_TWS_SUPPORT
//	mainAppCt.PauseFrame = (PCM_DATA_TYPE*)osPortMallocFromEnd(TWS_SINK_DEV_FRAME * sizeof(PCM_DATA_TYPE) * 2);//stereo
	#ifdef TWS_DAC0_OUT
		mainAppCt.DACFIFO_LEN = TWS_SINK_DEV_FIFO_SAMPLES * sizeof(PCM_DATA_TYPE) * 2;
		mainAppCt.DACFIFO = (uint32_t*)osPortMallocFromEnd(mainAppCt.DACFIFO_LEN);//DAC0 fifo
	#endif
	#ifdef TWS_DACX_OUT
		mainAppCt.DACXFIFO_LEN = TWS_SINK_DEV_FIFO_SAMPLES * sizeof(PCM_DATA_TYPE);
		mainAppCt.DACXFIFO = (uint32_t*)osPortMallocFromEnd(mainAppCt.DACXFIFO_LEN);//DACX fifo
	#endif
	#if defined(TWS_IIS0_OUT) || defined(TWS_IIS1_OUT)
		mainAppCt.I2SFIFO_LEN = TWS_SINK_DEV_FIFO_SAMPLES * sizeof(PCM_DATA_TYPE) * 2;
		mainAppCt.I2SFIFO = (uint32_t*)osPortMallocFromEnd(mainAppCt.I2SFIFO_LEN);
	#endif
#endif

#ifdef CFG_RES_AUDIO_I2S0IN_EN
	mainAppCt.I2S0_RX_FIFO = (uint32_t*)osPortMallocFromEnd(AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2);//I2S0 rx fifo
#endif
#ifdef CFG_RES_AUDIO_I2S1IN_EN
	mainAppCt.I2S1_RX_FIFO = (uint32_t*)osPortMallocFromEnd(AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2);//I2S1 rx fifo
#endif

#ifdef CFG_RES_AUDIO_I2S0OUT_EN
	mainAppCt.I2S0_TX_FIFO = (uint32_t*)osPortMallocFromEnd(AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2);//I2S0 tx fifo
#endif
#ifdef CFG_RES_AUDIO_I2S1OUT_EN
	mainAppCt.I2S1_TX_FIFO = (uint32_t*)osPortMallocFromEnd(AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2);//I2S1 tx fifo
#endif

#ifdef CFG_FUNC_DISPLAY_TASK_EN
	DisplayServiceCreate();
#endif
	APP_DBG("MainApp:run\n");

}


//接收下层service created消息，完毕后start这些servcie
static void MainAppServiceCreating(uint16_t msgId)
{
	if(msgId == MSG_AUDIO_CORE_SERVICE_CREATED)
	{
		APP_DBG("MainApp:AudioCore service created\n");
		mainAppCt.AudioCoreSync = TRUE;
	}
	
	if(mainAppCt.AudioCoreSync)
	{
		AudioCoreServiceStart();
		mainAppCt.AudioCoreSync = FALSE;
		mainAppCt.state = TaskStateReady;
	}
}

//接收下层service started，完毕后准备模式切换。
static void MainAppServiceStarting(uint16_t msgId)
{
	if(msgId == MSG_AUDIO_CORE_SERVICE_STARTED)
	{
		APP_DBG("MainApp:AudioCore service started\n");
		mainAppCt.AudioCoreSync = TRUE;
	}

	if(mainAppCt.AudioCoreSync)
	{
		mainAppCt.AudioCoreSync = FALSE;
		mainAppCt.state = TaskStateRunning;
		SysModeTaskCreat();
#ifdef	CFG_FUNC_OPEN_SLOW_DEVICE_TASK
		{
		extern	void CreatSlowDeviceTask(void);
		CreatSlowDeviceTask();
		}
#endif
	}
}

static void PublicDetect(void)
{

#if defined(BT_SNIFF_ENABLE) && defined(CFG_APP_BT_MODE_EN)
		tws_sniff_check_adda_process();//检测sniff后adda是否恢复的轮询
#endif

#ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN
		switch(GetSystemMode())
		{
			// Idle,Slave,HFP,USB Audio不省电关机
			case ModeIdle:
			case ModeTwsSlavePlay:
			case ModeBtHfPlay:
			case ModeUsbDevicePlay:
			mainAppCt.Silence_Power_Off_Time = 0;
			break;

			// BT 连上蓝牙不关机
			case ModeBtAudioPlay:
			if(btManager.btLinkState)
				mainAppCt.Silence_Power_Off_Time = 0;
			break;

			default:
			break;
		}

		mainAppCt.Silence_Power_Off_Time++;
        if(mainAppCt.Silence_Power_Off_Time >= SILENCE_POWER_OFF_DELAY_TIME)
		{
			mainAppCt.Silence_Power_Off_Time = 0;
			APP_DBG("Silence Power Off!!\n");

			MessageContext		msgSend;
#if	defined(CFG_IDLE_MODE_POWER_KEY) && (POWERKEY_MODE == POWERKEY_MODE_PUSH_BUTTON)
			msgSend.msgId = MSG_POWERDOWN;
			APP_DBG("msgSend.msgId = MSG_DEVICE_SERVICE_POWERDOWN\n");
#elif defined(CFG_SOFT_POWER_KEY_EN)
			msgSend.msgId = MSG_SOFT_POWER;
			APP_DBG("msgSend.msgId = MSG_DEVICE_SERVICE_SOFT_POWEROFF\n");
#elif defined(CFG_IDLE_MODE_DEEP_SLEEP)
			msgSend.msgId = MSG_DEEPSLEEP;
			APP_DBG("msgSend.msgId = MSG_DEVICE_SERVICE_DEEPSLEEP\n");
#else
			msgSend.msgId = MSG_POWER;
			APP_DBG("msgSend.msgId = MSG_POWER\n");
#endif
			MessageSend(GetMainMessageHandle(), &msgSend);
        }
#endif

#if FLASH_BOOT_EN
		//设备经过播放，预搜索mva包登记，可升级。拔出时取消登记。
		#ifndef FUNC_UPDATE_CONTROL
		if(SoftFlagGet(SoftFlagMvaInCard) && GetSystemMode() == ModeCardAudioPlay && (!SoftFlagGet(SoftFlagUpgradeOK)))
		{
			APP_DBG("MainApp:updata file exist in Card\n");
			#ifdef FUNC_OS_EN
			if(SDIOMutex != NULL)
			{
				osMutexLock(SDIOMutex);
			}
			#endif
			start_up_grate(SysResourceCard);
			#ifdef FUNC_OS_EN
			if(SDIOMutex != NULL)
			{
				osMutexUnlock(SDIOMutex);
			}
			#endif
		}
		else if(SoftFlagGet(SoftFlagMvaInUDisk) && GetSystemMode() == ModeUDiskAudioPlay&& (!SoftFlagGet(SoftFlagUpgradeOK)))
		{
			APP_DBG("MainApp:updata file exist in Udisk\n");
			#ifdef FUNC_OS_EN
			if(UDiskMutex != NULL)
			{
				//osMutexLock(UDiskMutex);
				while(osMutexLock_1000ms(UDiskMutex) != 1)
				{
					WDG_Feed();
				}
			}
			#endif
			start_up_grate(SysResourceUDisk);
			#ifdef FUNC_OS_EN
			if(UDiskMutex != NULL)
			{
				osMutexUnlock(UDiskMutex);
			}
			#endif
		}
		#endif


		/*uint8_t cmd = 0;
		if(UART0_RecvByte(&cmd))
		{
			if(cmd == 'y')
			{
				start_up_grate(0xFFFFFFFF);
			}
		}
		*/
#endif

}
static void PublicMsgPross(MessageContext msg)
{
	switch(msg.msgId)
	{
		case MSG_AUDIO_CORE_SERVICE_CREATED:	
			MainAppServiceCreating(msg.msgId);
			break;
		
		case MSG_AUDIO_CORE_SERVICE_STARTED:
			MainAppServiceStarting(msg.msgId);
			break;
#if FLASH_BOOT_EN
		case MSG_DEVICE_SERVICE_CARD_OUT:
			SoftFlagDeregister(SoftFlagUpgradeOK);
			SoftFlagDeregister(SoftFlagMvaInCard);//清理mva包标记
			break;
		
		case MSG_DEVICE_SERVICE_U_DISK_OUT:
			SoftFlagDeregister(SoftFlagUpgradeOK);
			SoftFlagDeregister(SoftFlagMvaInUDisk);
			break;
		
		case MSG_UPDATE:
			//if(SoftFlagGet(SoftFlagUpgradeOK))break;
			#ifdef FUNC_UPDATE_CONTROL
			APP_DBG("MainApp:UPDATE MSG\n");
			//设备经过播放，预搜索mva包登记，可升级。拔出时取消登记。
			if(SoftFlagGet(SoftFlagMvaInCard) && GetSystemMode() == ModeCardAudioPlay)
			{
				APP_DBG("MainApp:updata file exist in Card\n");
				start_up_grate(SysResourceCard);
			}
			else if(SoftFlagGet(SoftFlagMvaInUDisk) && GetSystemMode() == ModeUDiskAudioPlay)
			{
				APP_DBG("MainApp:updata file exist in Udisk\n");
				start_up_grate(SysResourceUDisk);
			}
			#endif
			break;
#endif		

#ifdef CFG_APP_IDLE_MODE_EN		
		case MSG_POWER:
		case MSG_POWERDOWN:
		case MSG_DEEPSLEEP:
#ifdef CFG_APP_HDMIIN_MODE_EN
			if(GetSystemMode() == ModeHdmiAudioPlay)
			{
				if(SoftFlagGet(SoftFlagDeepSleepMsgIsFromTV) == 0)//非电视端发来的关机
				{
					HDMI_CEC_ActivePowerOff(200);
				}
				SoftFlagDeregister(SoftFlagDeepSleepMsgIsFromTV);
			}
#endif
			if(GetSystemMode() != ModeIdle)
			{			
				if (msg.msgId == MSG_POWER){
					DBG("Main task MSG_POWER\n");
				}else if (msg.msgId == MSG_POWERDOWN){
					DBG("Main task MSG_POWERDOWN\n");
				}else if (msg.msgId == MSG_DEEPSLEEP){
					DBG("Main task MSG_DEEPSLEEP\n");
				}
				SendEnterIdleModeMsg();
			#if (defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT == ENABLE))
				if(GetSystemMode() == ModeBtHfPlay)
				{
					BtHfModeExit();				
				}
				extern void SetsBtHfModeEnterFlag(uint32_t flag);
				SetsBtHfModeEnterFlag(0);
			#endif		
			
			#ifdef BT_TWS_SUPPORT
				BtStackServiceMsgSend(MSG_BT_STACK_TWS_PAIRING_STOP);
				BtReconnectTwsStop();

				if(btManager.twsState == BT_TWS_STATE_CONNECTED)
				{
					#ifdef TWS_POWEROFF_MODE_SYNC
					if (btManager.twsRole == BT_TWS_MASTER)
					{
						void tws_send_key_msg(uint16_t msg);
						tws_send_key_msg(msg.msgId);
						btManager.twsState = BT_TWS_STATE_NONE;		
					}
					#else
					tws_link_disconnect();
					#endif
				}
			#endif

			#ifdef	CFG_IDLE_MODE_POWER_KEY
				if(msg.msgId == MSG_POWERDOWN)
				{
					SoftFlagRegister(SoftFlagIdleModeEnterPowerDown);
				}
			#endif				
			
			#ifdef CFG_IDLE_MODE_DEEP_SLEEP
				if(msg.msgId == MSG_DEEPSLEEP)
				{					
					SoftFlagRegister(SoftFlagIdleModeEnterSleep);
				}
			#endif	
			}
			break;			
#endif	

#ifdef CFG_SOFT_POWER_KEY_EN
		case MSG_SOFT_POWER:
			SoftKeyPowerOff();
			break;
#endif

#ifdef CFG_APP_BT_MODE_EN
		case MSG_BT_ENTER_DUT_MODE:
			BtEnterDutModeFunc();
			break;

#ifdef BT_SNIFF_ENABLE
#ifndef BT_TWS_SUPPORT
		case MSG_BT_SNIFF:
			BtStartEnterSniffMode();
			break;
#endif
#endif
		case MSG_BTSTACK_DEEPSLEEP:
			APP_DBG("MSG_BTSTACK_DEEPSLEEP\n");
#ifdef BT_TWS_SUPPORT
			{
				MessageContext		msgSend;
				
				msgSend.msgId		= MSG_DEEPSLEEP;
				MessageSend(GetMainMessageHandle(), &msgSend);
			}	
#endif
			break;

		case MSG_BTSTACK_BB_ERROR:
			APP_DBG("bb and bt stack reset\n");
			RF_PowerDownBySw();
			WDG_Feed();
			//reset bb and bt stack
			rwip_reset();
			BtStackServiceKill();
			WDG_Feed();
			vTaskDelay(50);
			RF_PowerDownByHw();
			WDG_Feed();
			//reset bb and bt stack
			BtStackServiceStart();

			//发起回连
			BtStackServiceMsgSend(MSG_BTSTACK_BB_ERROR_RESTART);

			//如蓝牙模式处于slave模式,则退出tws slave模式
			if(GetSystemMode() == ModeTwsSlavePlay)
			{
				MessageContext		msgSend;
				APP_DBG("Exit Tws Slave Mode\n");
				msgSend.msgId = MSG_DEVICE_SERVICE_TWS_SLAVE_DISCONNECT;
				MessageSend(GetMainMessageHandle(), &msgSend);
			}
			break;
#ifdef CFG_FUNC_BT_OTA_EN
		case MSG_BT_START_OTA:
			APP_DBG("\nMSG_BT_START_OTA\n");
			RF_PowerDownBySw();
			WDG_Feed();
			rwip_reset();
			BtStackServiceKill();
			WDG_Feed();
			vTaskDelay(50);
			RF_PowerDownByHw();
			WDG_Feed();
			start_up_grate(SysResourceBtOTA);
			break;
#endif
#if (BT_HFP_SUPPORT == ENABLE)
			case MSG_DEVICE_SERVICE_ENTER_BTHF_MODE:
			if(GetHfpState(BtCurIndex_Get()) >= BT_HFP_STATE_CONNECTED)
			{
				BtHfModeEnter();
			}
			break;
#endif
#ifdef BT_AUTO_ENTER_PLAY_MODE
		case MSG_BT_A2DP_STREAMING:
			//播放歌曲时,有模式切换需求,则在此消息中开始进行模式切换操作
			if((GetSystemMode() != ModeBtAudioPlay)&&(GetSystemMode() != ModeBtHfPlay))
			{
				MessageContext		msgSend;
				
				APP_DBG("Enter Bt Audio Play Mode...\n");
				//ResourceRegister(AppResourceBtPlay);
				
				// Send message to main app
				msgSend.msgId		= MSG_DEVICE_SERVICE_BTPLAY_IN;
				MessageSend(GetMainMessageHandle(), &msgSend);
			}
			break;
#endif
#endif
	}
	
#ifdef BT_TWS_SUPPORT
	tws_msg_process(msg.msgId);
#endif
		
}

static void MainAppTaskEntrance(void * param)
{
	MessageContext		msg;
	
	SystemInit();
	while(1)
	{
		MessageRecv(mainAppCt.msgHandle, &msg, MAIN_APP_MSG_TIMEOUT);
		PublicDetect();
		PublicMsgPross(msg);

#ifdef SOFT_WACTH_DOG_ENABLE
		big_dog_feed();
#else
		WDG_Feed();
#endif
		#ifdef AUTO_TEST_ENABLE
		extern void AutoTestMain(uint16_t test_msg);
		AutoTestMain(msg.msgId);
		#endif
		
		if(mainAppCt.state == TaskStateRunning)
		{
			DeviceServicePocess(msg.msgId);
			if(msg.msgId != MSG_NONE)
			{
				SysModeGenerate(msg.msgId);
				MessageSend(GetSysModeMsgHandle(), &msg);	
			}
			SysModeChangeTimeoutProcess();
		}
	}
}

/***************************************************************************************
 *
 * APIs
 *
 */
int32_t MainAppTaskStart(void)
{
	MainAppInit();
#ifdef CFG_FUNC_SHELL_EN
	shell_init();
	xTaskCreate(mv_shell_task, "SHELL", SHELL_TASK_STACK_SIZE, NULL, SHELL_TASK_PRIO, NULL);
#endif
	xTaskCreate(MainAppTaskEntrance, "MainApp", MAIN_APP_TASK_STACK_SIZE, NULL, MAIN_APP_TASK_PRIO, &mainAppCt.taskHandle);
	return 0;
}

MessageHandle GetMainMessageHandle(void)
{
	return mainAppCt.msgHandle;
}


uint32_t GetSystemMode(void)
{
	return mainAppCt.SysCurrentMode;
}

void SoftFlagRegister(uint32_t SoftEvent)
{
	SoftwareFlag |= SoftEvent;
	return ;
}

void SoftFlagDeregister(uint32_t SoftEvent)
{
	SoftwareFlag &= ~SoftEvent;
	return ;
}

bool SoftFlagGet(uint32_t SoftEvent)
{
	return SoftwareFlag & SoftEvent ? TRUE : FALSE;
}

void SamplesFrameUpdataMsg(void)//发现帧变化，发送消息
{
	MessageContext		msgSend;
	APP_DBG("SamplesFrameUpdataMsg\n");

	msgSend.msgId		= MSG_AUDIO_CORE_FRAME_SIZE_CHANGE;
    MessageSend(mainAppCt.msgHandle, &msgSend);
}

void EffectUpdataMsg(void)
{
	MessageContext		msgSend;
	APP_DBG("EffectUpdataMsg\n");

	msgSend.msgId		= MSG_AUDIO_CORE_EFFECT_CHANGE;
	MessageSend(mainAppCt.msgHandle, &msgSend);
}

uint32_t IsBtAudioMode(void)
{
	return (GetSysModeState(ModeBtAudioPlay) == ModeStateInit || GetSysModeState(ModeBtAudioPlay) == ModeStateRunning);
}

uint32_t IsBtHfMode(void)
{
	return (GetSysModeState(ModeBtHfPlay) == ModeStateRunning);
}

uint32_t IsBtTwsSlaveMode(void)
{
	return (GetSysModeState(ModeTwsSlavePlay) == ModeStateInit || GetSysModeState(ModeTwsSlavePlay) == ModeStateRunning);
}

uint32_t IsIdleModeReady(void)
{
	if(GetModeDefineState(ModeIdle))
	{
		if(GetSysModeState(ModeIdle) == ModeStateInit || GetSysModeState(ModeIdle) == ModeStateRunning )
			return 1;
	}
	return 0;
}


void PowerOffMessage(void)
{
	MessageContext		msgSend;

#if	defined(CFG_IDLE_MODE_POWER_KEY) && (POWERKEY_MODE == POWERKEY_MODE_PUSH_BUTTON)
	msgSend.msgId = MSG_POWERDOWN;
	APP_DBG("msgSend.msgId = MSG_DEVICE_SERVICE_POWERDOWN\n");
#elif defined(CFG_SOFT_POWER_KEY_EN)
	msgSend.msgId = MSG_SOFT_POWER;
	APP_DBG("msgSend.msgId = MSG_DEVICE_SERVICE_SOFT_POWEROFF\n");
#elif	defined(CFG_IDLE_MODE_DEEP_SLEEP) 
	msgSend.msgId = MSG_DEEPSLEEP;
	APP_DBG("msgSend.msgId = MSG_DEVICE_SERVICE_DEEPSLEEP\n");
#else
	msgSend.msgId = MSG_POWER;
	APP_DBG("msgSend.msgId = MSG_POWER\n");
#endif

	MessageSend(GetMainMessageHandle(), &msgSend);
}



void BatteryLowMessage(void)
{
	MessageContext		msgSend;

	APP_DBG("msgSend.msgId = MSG_DEVICE_SERVICE_BATTERY_LOW\n");
	msgSend.msgId = MSG_DEVICE_SERVICE_BATTERY_LOW;
	MessageSend(GetMainMessageHandle(), &msgSend);
}

void TwsSlaveModeSwitchDeal(SysModeNumber pre, SysModeNumber Cur)
{
#ifdef TWS_SLAVE_MODE_SWITCH_EN
	//null
	if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
	{
		if (pre == ModeTwsSlavePlay)
		{
			if((Cur != ModeBtAudioPlay)&&(Cur != ModeTwsSlavePlay))
			{
				BtFastPowerOff();
				BtStackServiceWaitClear();
			}
			else
			{
				BtStackServiceWaitResume();
			}
		}
	}
	else if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_DISABLE)
	{
		if(Cur != ModeBtAudioPlay)
		{
			BtPowerOff();
		}
	}
#endif	
}

