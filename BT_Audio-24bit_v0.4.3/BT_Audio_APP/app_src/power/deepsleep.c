#include "app_config.h"
#include "powercontroller.h"
#include "deepsleep.h"
#include "gpio.h"
#include "timeout.h"
#include "audio_adc.h"
#include "dac.h"
#include "clk.h"
#include "chip_info.h"
#include "otg_device_hcd.h"
#include "rtc.h"
#include "irqn.h"
#include "debug.h"
#include "adc_key.h"
#include "delay.h"
#include "FreeRTOS.h"
#include "task.h"
#include "adc_key.h"
#include "uarts.h"
//#include "OrioleReg.h"//for test
#include "sys.h"
#include "sadc_interface.h"
#include "watchdog.h"
#include "backup.h"
#include "ir_key.h"
#include "app_message.h"
#include "reset.h"
#include "bt_stack_service.h"
#include "key.h"
#include "main_task.h"
#include "efuse.h"
#include "bt_common_api.h"
#include "bt_manager.h"
#include "bt_app_sniff.h"
#include "hdmi_in_api.h"
#ifdef CFG_IDLE_MODE_DEEP_SLEEP
HDMIInfo  			 *gHdmiCt;
void GIE_ENABLE(void);
void SystemOscConfig(void);
void SleepMainAppTask(void);
void SleepAgainConfig(void);

void WakeupMain(void);

void SleepMain(void);
void LogUartConfig(bool InitBandRate);

#define CHECK_SCAN_TIME				5000		//醒来确认有效唤醒源 扫描限时ms。
TIMER   waitCECTime;
uint8_t	waitCECTimeFlag = 0;

#define DPLL_QUICK_START_SLOPE	(*(volatile unsigned long *) 0x40026028)
//DPLL 快速启动参数获取。
void Clock_GetDPll(uint32_t* NDAC, uint32_t* OS, uint32_t* K1, uint32_t* FC);

static uint32_t sources;
#ifdef CFG_PARA_WAKEUP_SOURCE_RTC
uint32_t alarm = 0;
#endif

__attribute__((section(".driver.isr")))void WakeupInterrupt(void)
{
	sources |= Power_WakeupSourceGet();

	Power_WakeupSourceClear();
}


void SystermGPIOWakeupConfig(PWR_SYSWAKEUP_SOURCE_SEL source,PWR_WAKEUP_GPIO_SEL gpio,PWR_SYSWAKEUP_SOURCE_EDGE_SEL edge)
{
	if(gpio < 32)
	{
		GPIO_RegOneBitSet(GPIO_A_IE,   (1 << gpio));
		GPIO_RegOneBitClear(GPIO_A_OE, (1 << gpio));
		if( edge == SYSWAKEUP_SOURCE_NEGE_TRIG )
		{
			GPIO_RegOneBitSet(GPIO_A_PU, (1 << gpio));//因为芯片的GPIO有内部上下拉电阻,选择下降沿触发时要将指定的GPIO唤醒管脚配置为上拉
			GPIO_RegOneBitClear(GPIO_A_PD, (1 << gpio));
		}
		else if(edge == SYSWAKEUP_SOURCE_POSE_TRIG )
		{
			GPIO_RegOneBitClear(GPIO_A_PU, (1 << gpio));//因为芯片的GPIO有内部上下拉电阻，所以选择上升沿触发时要将指定的GPIO唤醒管脚配置为下拉
			GPIO_RegOneBitSet(GPIO_A_PD, (1 << gpio));
		}
	}
	else if(gpio < 41)
	{
		GPIO_RegOneBitSet(GPIO_B_IE,   (1 << (gpio - 32)));
		GPIO_RegOneBitClear(GPIO_B_OE, (1 << (gpio - 32)));
		if( edge == SYSWAKEUP_SOURCE_NEGE_TRIG )
		{
			GPIO_RegOneBitSet(GPIO_B_PU, (1 << (gpio - 32)));//因为芯片的GPIO有内部上下拉电阻,选择下降沿触发时要将指定的GPIO唤醒管脚配置为上拉
			GPIO_RegOneBitClear(GPIO_B_PD, (1 << (gpio - 32)));
		}
		else if( edge == SYSWAKEUP_SOURCE_POSE_TRIG )
		{
			GPIO_RegOneBitClear(GPIO_B_PU, (1 << (gpio - 32)));//因为芯片的GPIO有内部上下拉电阻，所以选择上升沿触发时要将指定的GPIO唤醒管脚配置为下拉
			GPIO_RegOneBitSet(GPIO_B_PD, (1 << (gpio - 32)));
		}
	}
	else if(gpio == 41)
	{

		BACKUP_WriteEnable();

		BACKUP_C0RegSet(BKUP_GPIO_C0_REG_IE_OFF, TRUE);
		BACKUP_C0RegSet(BKUP_GPIO_C0_REG_OE_OFF, FALSE);
		if( edge == SYSWAKEUP_SOURCE_NEGE_TRIG )
		{
			BACKUP_C0RegSet(BKUP_GPIO_C0_REG_PU_OFF, TRUE);
			BACKUP_C0RegSet(BKUP_GPIO_C0_REG_PD_OFF, FALSE);
		}
		else if( edge == SYSWAKEUP_SOURCE_POSE_TRIG )
		{
			BACKUP_C0RegSet(BKUP_GPIO_C0_REG_PU_OFF, FALSE);//因为芯片的GPIO有内部上下拉电阻，所以选择上升沿触发时要将指定的GPIO唤醒管脚配置为下拉
			BACKUP_C0RegSet(BKUP_GPIO_C0_REG_PD_OFF, TRUE);
		}
		BACKUP_WriteDisable();
	}

	Power_WakeupSourceClear();
	Power_WakeupSourceSet(source, gpio, edge);
	Power_WakeupEnable(source);

	NVIC_EnableIRQ(Wakeup_IRQn);
	NVIC_SetPriority(Wakeup_IRQn, 0);
	GIE_ENABLE();
}

void SystermIRWakeupConfig(IR_MODE_SEL ModeSel, IR_IO_SEL GpioSel, IR_CMD_LEN_SEL CMDLenSel)
{

	Clock_BTDMClkSelect(RC_CLK32_MODE);//sniff开启时使用影响蓝牙功能呢
	Reset_FunctionReset(IR_FUNC_SEPA);
	IRKeyInit();
	IR_WakeupEnable();


	if(GpioSel == IR_GPIOB6)
	{
		GPIO_RegOneBitSet(GPIO_B_IE,   GPIO_INDEX6);
		GPIO_RegOneBitClear(GPIO_B_OE, GPIO_INDEX6);
		GPIO_RegOneBitSet(GPIO_B_IN,   GPIO_INDEX6);
		GPIO_RegOneBitClear(GPIO_B_OUT, GPIO_INDEX6);
		GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX6);
	}
	else if(GpioSel == IR_GPIOB7)
	{
		GPIO_RegOneBitSet(GPIO_B_IE,   GPIO_INDEX7);
		GPIO_RegOneBitClear(GPIO_B_OE, GPIO_INDEX7);
		GPIO_RegOneBitSet(GPIO_B_IN,   GPIO_INDEX7);
		GPIO_RegOneBitClear(GPIO_B_OUT, GPIO_INDEX7);
		GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX7);
	}
	else
	{
		GPIO_RegOneBitSet(GPIO_A_IE,   GPIO_INDEX29);
		GPIO_RegOneBitClear(GPIO_A_OE, GPIO_INDEX29);
		GPIO_RegOneBitSet(GPIO_A_IN,   GPIO_INDEX29);
		GPIO_RegOneBitClear(GPIO_A_OUT, GPIO_INDEX29);
		GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX29);
	}
	NVIC_EnableIRQ(Wakeup_IRQn);
	NVIC_SetPriority(Wakeup_IRQn, 0);
	GIE_ENABLE();

	Power_WakeupSourceClear();
	Power_WakeupSourceSet(SYSWAKEUP_SOURCE9_IR, 0, 0);
	Power_WakeupEnable(SYSWAKEUP_SOURCE9_IR);
}

void SystermIRWakeupConfig_sniff(IR_MODE_SEL ModeSel, IR_IO_SEL GpioSel, IR_CMD_LEN_SEL CMDLenSel)
{

	Reset_FunctionReset(IR_FUNC_SEPA);
	IRKeyInit();
	IR_WakeupEnable();


	if(GpioSel == IR_GPIOB6)
	{
		GPIO_RegOneBitSet(GPIO_B_IE,   GPIO_INDEX6);
		GPIO_RegOneBitClear(GPIO_B_OE, GPIO_INDEX6);
		GPIO_RegOneBitSet(GPIO_B_IN,   GPIO_INDEX6);
		GPIO_RegOneBitClear(GPIO_B_OUT, GPIO_INDEX6);
		GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX6);
	}
	else if(GpioSel == IR_GPIOB7)
	{
		GPIO_RegOneBitSet(GPIO_B_IE,   GPIO_INDEX7);
		GPIO_RegOneBitClear(GPIO_B_OE, GPIO_INDEX7);
		GPIO_RegOneBitSet(GPIO_B_IN,   GPIO_INDEX7);
		GPIO_RegOneBitClear(GPIO_B_OUT, GPIO_INDEX7);
		GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX7);
	}
	else
	{
		GPIO_RegOneBitSet(GPIO_A_IE,   GPIO_INDEX29);
		GPIO_RegOneBitClear(GPIO_A_OE, GPIO_INDEX29);
		GPIO_RegOneBitSet(GPIO_A_IN,   GPIO_INDEX29);
		GPIO_RegOneBitClear(GPIO_A_OUT, GPIO_INDEX29);
		GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX29);
	}
	NVIC_EnableIRQ(Wakeup_IRQn);
	NVIC_SetPriority(Wakeup_IRQn, 0);
	GIE_ENABLE();

	Power_WakeupSourceClear();
	Power_WakeupSourceSet(SYSWAKEUP_SOURCE9_IR, 0, 0);
	Power_WakeupEnable(SYSWAKEUP_SOURCE9_IR);
}



#ifdef CFG_PARA_WAKEUP_SOURCE_RTC
//RTC唤醒 并不会进入RTC中断
void SystermRTCWakeupConfig(uint32_t SleepSecond)
{
	//RTC_REG_TIME_UNIT start;
	RTC_ClockSrcSel(OSC_24M);
	RTC_IntDisable();
	RTC_IntFlagClear();
	RTC_WakeupDisable();

	alarm = RTC_SecGet() + SleepSecond;
	RTC_SecAlarmSet(alarm);
	RTC_WakeupEnable();
	RTC_IntEnable();

	NVIC_EnableIRQ(Wakeup_IRQn);
	NVIC_SetPriority(Wakeup_IRQn, 0);
	NVIC_EnableIRQ(Rtc_IRQn);
	NVIC_SetPriority(Rtc_IRQn, 1);
	GIE_ENABLE();

	Power_WakeupSourceClear();
	Power_WakeupSourceSet(SYSWAKEUP_SOURCE7_RTC, 0, 0);
	Power_WakeupEnable(SYSWAKEUP_SOURCE7_RTC);
}
#endif //CFG_PARA_WAKEUP_RTC

void DeepSleepIOConfig()
{
	//cansle all IO AF
	{
		int IO_cnt = 0;
		for(IO_cnt = 0;IO_cnt < 32;IO_cnt++)
		{
			if(IO_cnt == 30)
				continue;//A30是sw口
			if(IO_cnt == 31)
				continue;//A31是sw口
			if(IO_cnt == 23)
				continue;//A23是唤醒源

			GPIO_PortAModeSet(GPIOA0 << IO_cnt,0);
		}
		for(IO_cnt = 0;IO_cnt < 8;IO_cnt++)
		{
			if(IO_cnt == 0)
				continue;//B1是sw口
			if(IO_cnt == 1)
				continue;//B0是sw口

			GPIO_PortBModeSet(GPIOB0 << IO_cnt,0);
		}
	}

	SleepAgainConfig();//配置相同
}

void SystermDeepSleepConfig(void)
{
	uint32_t IO_cnt;

	for(IO_cnt = 0;IO_cnt < 32;IO_cnt++)
	{
#if (CFG_PARA_WAKEUP_GPIO_CEC != WAKEUP_GPIOA27)
		if(IO_cnt == 27)
			continue;
#endif
		GPIO_PortAModeSet(GPIOA0 << IO_cnt, 0);
	}

	for(IO_cnt = 0;IO_cnt < 8;IO_cnt++)
	{
		GPIO_PortBModeSet(GPIOB0 << IO_cnt, 0);
	}

	SleepAgainConfig();//配置相同

	SleepMain();

#ifdef CFG_PARA_WAKEUP_SOURCE_RTC
	alarm = 0;
#else

	//Clock_LOSCDisable(); //若有RTC应用则不关闭32K晶振 BKD mark sleep
//	BACKUP_32KDisable(OSC32K_SOURCE);// bkd add

#endif
}

//启用多个唤醒源时，source通道配置灵活，但不可重复。
void WakeupSourceSet(void)
{
#ifdef CFG_PARA_WAKEUP_SOURCE_RTC
	SystermRTCWakeupConfig(CFG_PARA_WAKEUP_TIME_RTC);
#endif	
#if defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_ADCKEY, CFG_PARA_WAKEUP_GPIO_ADCKEY, SYSWAKEUP_SOURCE_NEGE_TRIG);
#ifdef CFG_PARA_WAKEUP_SOURCE_POWERKEY
	SystermGPIOWakeupConfig(SYSWAKEUP_SOURCE6_POWERKEY, 42, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif
#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_IOKEY1) && defined(CFG_PARA_WAKEUP_GPIO_IOKEY1)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_IOKEY1, CFG_PARA_WAKEUP_GPIO_IOKEY1, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_IOKEY2) && defined(CFG_PARA_WAKEUP_GPIO_IOKEY2)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_IOKEY2, CFG_PARA_WAKEUP_GPIO_IOKEY2, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_IR)
	SystermIRWakeupConfig(CFG_PARA_IR_SEL, CFG_RES_IR_PIN, CFG_PARA_IR_BIT);
#endif	
}


void DeepSleeping(void)
{

	uint32_t GpioAPU_Back,GpioAPD_Back,GpioBPU_Back,GpioBPD_Back;

//	WDG_Disable();
	WDG_Feed();
	
	GpioAPU_Back = GPIO_RegGet(GPIO_A_PU);
	GpioAPD_Back = GPIO_RegGet(GPIO_A_PD);
	GpioBPU_Back = GPIO_RegGet(GPIO_B_PU);
	GpioBPD_Back = GPIO_RegGet(GPIO_B_PD);
	SystermDeepSleepConfig();
	WakeupSourceSet();
	waitCECTimeFlag = 0;
	WDG_Disable();
	Power_GotoDeepSleep();
	WDG_Enable(WDG_STEP_4S);
	while(!SystermWackupSourceCheck())
	{
		SleepAgainConfig();
		Power_WakeupDisable(0xff);
		WakeupSourceSet();
		waitCECTimeFlag = 0;
		WDG_Disable();
		Power_GotoDeepSleep();
		WDG_Enable(WDG_STEP_4S);
	}
	Power_WakeupDisable(0xff);
	//GPIO恢复上下拉
	GPIO_RegSet(GPIO_A_PU, GpioAPU_Back);
	GPIO_RegSet(GPIO_A_PD, GpioAPD_Back);
	GPIO_RegSet(GPIO_B_PU, GpioBPU_Back);
	GPIO_RegSet(GPIO_B_PD, GpioBPD_Back);
#if 1
    GPIO_PortBModeSet(GPIOB0, 0x0001); //调试口 SW恢复 方便下载
    GPIO_PortBModeSet(GPIOB1, 0x0001);
#else
	GPIO_PortAModeSet(GPIOA30, 0x0005);//调试口 SW恢复 方便下载
	GPIO_PortAModeSet(GPIOA31, 0x0004);
#endif
	WDG_Feed();
	WakeupMain();
	SysTickInit();
	WDG_Feed();
#if defined(CFG_RES_ADC_KEY_SCAN) || defined(CFG_RES_IR_KEY_SCAN) || defined(CFG_RES_CODE_KEY_USE)|| defined(CFG_ADC_LEVEL_KEY_EN) || defined(CFG_RES_IO_KEY_SCAN)
	KeyInit();//Init keys
#endif	
#ifdef BT_TWS_SUPPORT
	tws_time_init();
#endif
#ifdef CFG_FUNC_LED_REFRESH
	//默认优先级为0，旨在提高刷新速率，特别是断点记忆等写flash操作有影响刷屏，必须严格遵守所有timer6中断调用都是TCM代码，含调用的driver库代码
	//已确认GPIO_RegOneBitSet、GPIO_RegOneBitClear在TCM区，其他api请先确认。
	NVIC_SetPriority(Timer6_IRQn, 0);
 	Timer_Config(TIMER6,1000,0);
 	Timer_Start(TIMER6);
 	NVIC_EnableIRQ(Timer6_IRQn);
#endif

#ifdef CFG_FUNC_POWER_MONITOR_EN
	extern void PowerMonitorInit(void);
	PowerMonitorInit();
#endif

#if defined(CFG_FUNC_DISPLAY_EN)
    DispInit(0);
#endif
}


bool SystermWackupSourceCheck(void)
{
#ifdef CFG_RES_ADC_KEY_SCAN
	AdcKeyMsg AdcKeyVal;
#endif

#ifdef CFG_PARA_WAKEUP_SOURCE_IR
	IRKeyMsg IRKeyMsg;

#endif
	TIMER WaitScan;

#if defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY)
	SarADC_Init();
	AdcKeyInit();
#endif

//********************
	//串口IO设置
	LogUartConfig(FALSE);//此处如果重配clk波特率，较为耗时，不重配。
	SysTickInit();
	//APP_DBG("Scan:%x\n", (int)sources);
#ifdef CFG_PARA_WAKEUP_SOURCE_RTC
	if(sources & CFG_PARA_WAKEUP_SOURCE_RTC)
	{
		sources = 0;//唤醒源清零
		//APP_DBG("Alarm!%d", RTC_SecGet());
		return TRUE;
	}
	else if(alarm)//避免RTC唤醒事件错失
	{
		uint32_t NowTime;
		NowTime = RTC_SecGet();
		if(NowTime + 2 + CHECK_SCAN_TIME / 1000 > alarm)//如果存在多个唤醒源，rtc可以提前唤醒()，以避免丢失
		{
			//APP_DBG("Timer");
			sources = 0;//唤醒源清零
			alarm = 0;
			return TRUE;
		}
	}
#endif

	TimeOutSet(&WaitScan, CHECK_SCAN_TIME);
	while(!IsTimeOut(&WaitScan)
		)
	{
		WDG_Feed();
#ifdef CFG_PARA_WAKEUP_SOURCE_IR
		if(sources & CFG_PARA_WAKEUP_SOURCE_IR)
		{
			IRKeyMsg = IRKeyScan();			
			if(IRKeyMsg.index != IR_KEY_NONE && IRKeyMsg.type != IR_KEY_UNKOWN_TYPE)
			{
				//APP_DBG("IRID:%d,type:%d\n", IRKeyMsg.index, IRKeyMsg.type);
				SetIrKeyValue(IRKeyMsg.type,IRKeyMsg.index);
				if((GetGlobalKeyValue() == MSG_DEEPSLEEP)||(GetGlobalKeyValue() == MSG_BT_SNIFF))
				{
					sources = 0;
					ClrGlobalKeyValue();
					return TRUE;
				}
				ClrGlobalKeyValue();
			}
		}
#endif
#ifdef CFG_PARA_WAKEUP_SOURCE_POWERKEY
		if(sources & SYSWAKEUP_SOURCE6_POWERKEY)
		{
			sources = 0;
			return TRUE;
		}
#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY) && defined(CFG_RES_ADC_KEY_SCAN)
		if(sources & (CFG_PARA_WAKEUP_SOURCE_ADCKEY))
		{
			AdcKeyVal = AdcKeyScan();
			if(AdcKeyVal.index != ADC_CHANNEL_EMPTY && AdcKeyVal.type != ADC_KEY_UNKOWN_TYPE)
			{
				//APP_DBG("KeyID:%d,type:%d\n", AdcKeyVal.index, AdcKeyVal.type);
				SetAdcKeyValue(AdcKeyVal.type,AdcKeyVal.index);
				if((GetGlobalKeyValue() == MSG_DEEPSLEEP)||(GetGlobalKeyValue() == MSG_BT_SNIFF))
				{
					sources = 0;
					ClrGlobalKeyValue();
					return TRUE;
				}
				ClrGlobalKeyValue();
			}
		}
#endif
#ifdef CFG_RES_IO_KEY_SCAN
#ifdef CFG_PARA_WAKEUP_SOURCE_IOKEY1
		if(sources & CFG_PARA_WAKEUP_SOURCE_IOKEY1)
		{
			sources = 0;
			return TRUE;
		}
#endif
#ifdef CFG_PARA_WAKEUP_SOURCE_IOKEY2
		if(sources & CFG_PARA_WAKEUP_SOURCE_IOKEY2)
		{
			sources = 0;
			return TRUE;
		}
#endif
#endif

	}
	sources = 0;
//	SysTickDeInit();
	return FALSE;
}


void SleepAgainConfig(void)
{
#ifndef	BT_SNIFF_ENABLE
	GPIO_RegSet(GPIO_A_IE,0x00000000);
	GPIO_RegSet(GPIO_A_OE,0x00000000);
	GPIO_RegSet(GPIO_A_OUTDS,0x00000000);//bkd GPIO_A_REG_OUTDS
	GPIO_RegSet(GPIO_A_PD,0xffffffff);
	GPIO_RegSet(GPIO_A_PU,0x00000000);//此时的flash的CS必须拉高0x00400000
	GPIO_RegSet(GPIO_A_ANA_EN,0x00000000);
	GPIO_RegSet(GPIO_A_PULLDOWN0,0x00000000);//bkd
	GPIO_RegSet(GPIO_A_PULLDOWN1,0x00000000);//bkd

	GPIO_RegSet(GPIO_B_IE,0x00);
	GPIO_RegSet(GPIO_B_OE,0x00); 
	GPIO_RegSet(GPIO_B_OUTDS,0x00); // bkd mark GPIO_B_REG_OUTDS
	GPIO_RegSet(GPIO_B_PD,0xff);//B2、B3下拉，B4,B5高阻 0x1cc
	GPIO_RegSet(GPIO_B_PU,0x00);//B0、B1上拉 0x03
	GPIO_RegSet(GPIO_B_ANA_EN,0x00);
	GPIO_RegSet(GPIO_B_PULLDOWN,0x00);//bkd mark GPIO_B_REG_PULLDOWN

#else

	GPIO_RegSet(GPIO_A_IE,0x00000000 | (BIT(23)));
	GPIO_RegSet(GPIO_A_OE,0x00000000);
	GPIO_RegSet(GPIO_A_OUTDS,0x00000000);//bkd GPIO_A_REG_OUTDS
	GPIO_RegSet(GPIO_A_PD,0xffffffff & (~ BIT(23)));
	GPIO_RegSet(GPIO_A_PU,0x00000000 | (BIT(23)));//此时的flash的CS必须拉高0x00400000
	GPIO_RegSet(GPIO_A_ANA_EN,0x00000000);
	GPIO_RegSet(GPIO_A_PULLDOWN0,0x00000000);//bkd
	GPIO_RegSet(GPIO_A_PULLDOWN1,0x00000000);//bkd

	GPIO_RegSet(GPIO_B_IE,0x00);
	GPIO_RegSet(GPIO_B_OE,0x00);
	GPIO_RegSet(GPIO_B_OUTDS,0x00); // bkd mark GPIO_B_REG_OUTDS
	GPIO_RegSet(GPIO_B_PD,0xff);//B2、B3下拉，B4,B5高阻 0x1cc
	GPIO_RegSet(GPIO_B_PU,0x00);//B0、B1上拉 0x03
	GPIO_RegSet(GPIO_B_ANA_EN,0x00);
	GPIO_RegSet(GPIO_B_PULLDOWN,0x00);//bkd mark GPIO_B_REG_PULLDOWN
#endif
}

#endif//CFG_FUNC_DEEPSLEEP_EN



#ifdef BT_SNIFF_ENABLE

//IR退出sniff轮询
void IrWakeupProcess(void)
{
	uint32_t Cmd = 0;
	uint8_t val = 0;

	//printf("IrWakeupProcess\n");
	if(IR_CommandFlagGet())
	{
		Cmd = IR_CommandDataGet();
		val = IRKeyIndexGet_BT(Cmd);

		SetIrKeyValue((uint8_t)2,(uint16_t)val);
		//APP_DBG("cmd:0x%lx,0x%d,%x\n",Cmd,GetIrKeyValue(),val);
		if(GetIrKeyValue() == MSG_BT_SNIFF)
		{
			extern void BtSniffExit_process(void);
#ifdef CFG_IDLE_MODE_DEEP_SLEEP
			sources = 0;
#endif
			ClrGlobalKeyValue();
			IR_Disable();
			BtSniffExit_process();
		}
		IR_IntFlagClear();
		IR_CommandFlagClear();
	}
}

uint8_t GetDebugPrintPort(void);
void UartClkChange(CLK_MODE clk_change)//蓝牙唤醒配置入口
{
	//串口时钟切换
#ifdef FUNC_OS_EN
	if(GetDebugPrintPort())
	{
		osMutexLock(UART1Mutex);
	}
	else
	{
		osMutexLock(UART0Mutex);
	}
#endif
	Clock_UARTClkSelect(clk_change);//先切换log clk。避免后续慢速处理
	LogUartConfig(TRUE);
#ifdef FUNC_OS_EN
	if(GetDebugPrintPort())
	{
		osMutexUnlock(UART1Mutex);;
	}
	else
	{
		osMutexUnlock(UART0Mutex);;
	}

#endif
}

uint8_t sniffiocnt = 0;
uint8_t sniff_wakeup_check()
{
//	bool CecRest = FALSE;

//	if(!GPIO_RegOneBitGet(GPIO_A_IN,GPIOA23))
//	{//唤醒流程，按键大于两个sniff周期就唤醒。如果用时间判断可能导致sniff功能不正常
//
//		if(sniffiocnt > 2)//按键超过两个周期就退出。
//		{//退出sniff。
//			sniffiocnt = 0;
//			extern void BtSniffExit_process(void);
//			BtSniffExit_process();
//
//			return 1;
//		}
//		else
//		{
//			sniffiocnt++;
//
//			return 1;
//		}
//
//	}

#if defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY) && defined(CFG_RES_ADC_KEY_SCAN)
		if(sources & (CFG_PARA_WAKEUP_SOURCE_ADCKEY))
		{
			AdcKeyMsg AdcKeyVal;

			SarADC_Init();
			AdcKeyInit();

			sources = 0;
			//如果没出现RELEASED就在while里面
			do
			{
				AdcKeyVal = AdcKeyScan();

				if(AdcKeyVal.index != ADC_CHANNEL_EMPTY && AdcKeyVal.type != ADC_KEY_UNKOWN_TYPE)
				{
//					APP_DBG("KeyID:%d,type:%d\n", AdcKeyVal.index, AdcKeyVal.type);
					SetAdcKeyValue(AdcKeyVal.type,AdcKeyVal.index);
					if((GetGlobalKeyValue() == MSG_DEEPSLEEP)||(GetGlobalKeyValue() == MSG_BT_SNIFF))
					{
						ClrGlobalKeyValue();
						extern void BtSniffExit_process(void);
						BtSniffExit_process();
						return 1;
					}
					ClrGlobalKeyValue();
				}
			}while((AdcKeyVal.type != ADC_KEY_RELEASED) && (AdcKeyVal.type != ADC_KEY_LONG_RELEASED));
			return 1;
		}
#endif

#ifdef CFG_PARA_WAKEUP_SOURCE_POWERKEY
		if(sources & SYSWAKEUP_SOURCE6_POWERKEY)
		{
			APP_DBG("POWER_Key!\n");
			sources = 0;
			extern void BtSniffExit_process(void);
			BtSniffExit_process();
			return 1;
		}
#endif

#if defined(CFG_PARA_WAKEUP_SOURCE_IR)
	if(sources & CFG_PARA_WAKEUP_SOURCE_IR)
	{
		sources = 0;
		IrWakeupProcess();
		return 1;
	}
#endif

	return 0;

}

void BtDeepSleepForUsr(void)//蓝牙休眠配置入口，目前没做处理
{
	//快速启动pll有点问题
//	uint32_t SLOPE, NDAC, OS, K1, FC;
//	uint32_t GpioAPU_Back,GpioAPD_Back,GpioBPU_Back,GpioBPD_Back;
//
//	Clock_GetDPll(&NDAC, &OS, &K1, &FC);
//	SLOPE = DPLL_QUICK_START_SLOPE;//获取快速启动，参数。

//	GPIO_PortAModeSet(GPIOA30,0);		  //去掉Sw复用。调试口
//	GPIO_PortAModeSet(GPIOA31,0);

	UartClkChange(RC_CLK_MODE);
	GIE_DISABLE();
	DeepSleepIOConfig();
	Power_DeepSleepLDO12ConfigTest(0,5,0);//进入deepsleep时电压降为1V0
	SysTickDeInit();

	NVIC_EnableIRQ(Wakeup_IRQn);
	NVIC_SetPriority(Wakeup_IRQn, 0);
	Power_WakeupSourceClear();
	Power_WakeupSourceSet(SYSWAKEUP_SOURCE13_BT, 0, 0);//设置蓝牙为唤醒源，无需IO所以随便填了0
	Power_WakeupEnable(SYSWAKEUP_SOURCE13_BT);

#if defined(CFG_PARA_WAKEUP_SOURCE_IR)
	SystermIRWakeupConfig_sniff(CFG_PARA_IR_SEL, CFG_RES_IR_PIN, CFG_PARA_IR_BIT);
#endif

	NVIC_DisableForDeepsleep();//关闭所有中断，只开唤醒中断

	Clock_DeepSleepSysClkSelect(RC_CLK_MODE, FSHC_RC_CLK_MODE, TRUE);

	Clock_PllClose();
	Clock_LOSCDisable();
//	Clock_HOSCDisable();
#ifdef CFG_IDLE_MODE_DEEP_SLEEP
	sources = 0;
	waitCECTimeFlag = 0;
//	GIE_ENABLE();//Power_GotoDeepSleep里面有开时钟动作
#endif
	Power_GotoDeepSleep();

	GIE_DISABLE();
//	Clock_PllQuicklock(288000, K1, OS, NDAC, FC, SLOPE);
#ifdef BT_TWS_SUPPORT
    Clock_PllLock(320000);
#else
    Clock_PllLock(288000);
#endif
	Clock_DeepSleepSysClkSelect(PLL_CLK_MODE,FSHC_PLL_CLK_MODE,FALSE);

	NVIC_EnableForDeepsleep();//中断恢复

//	GIE_ENABLE();

	SysTickInit();//开始OS 打开全局时钟

	UartClkChange(PLL_CLK_MODE);

//	GPIO_PortAModeSet(GPIOA30, 0x0005);//调试口 SW恢复 方便下载
//	GPIO_PortAModeSet(GPIOA31, 0x0004);

//	extern uint8_t OtgPortLinkState;
//	Timer_Config(TIMER2,1000,0);
//	Timer_Start(TIMER2);
//	NVIC_EnableIRQ(Timer2_IRQn);
//	OtgPortLinkState = 0;


}

uint8_t sniff_wakeup_flag = 0;//本次休眠唤醒是否触发sniff标志，为保系统稳定，此标志可确保系统不受stack影响
							  //0sniff流程没触发   1sniff已经触发
void sniff_wakeup_set(uint8_t set)
{
	sniff_wakeup_flag = set;
}
uint8_t sniff_wakeup_get(void)
{
	return sniff_wakeup_flag;
}

uint8_t sniff_lmp_sync_flag = 0;//sniff的lmp同步命令已经发送过标志，防止用于UI多次发送lmp导致异常,此标志可确保协议栈不受系统影响
								//0表示未发送可接受，1表示已经有命令请等待。
void sniff_lmpsend_set(uint8_t set)
{
	sniff_lmp_sync_flag = set;
}
uint8_t sniff_lmpsend_get()
{
	return sniff_lmp_sync_flag;
}

void tws_stop_callback()//进入sniff时库里会调用
{
	SysDeepsleepStandbyStatus();
	printf("SysDeepsleepStandbyStatus\n");
}

void tws_sniff_check_adda_process()
{
//	if(GetBtManager()->twsRole == BT_TWS_SLAVE)
	{

//		if(BtSniffADDAReadyGet() == 3)
//		{
//			printf("wakeup all ready\n");
//			if(sniff_lmpsend_get() == 1)
//			{
//				//##__退出sniff，标志位恢复__##
//				sniff_lmpsend_set(0);
//				printf("sniff_lmpsend_set(0)\n");
//				//##____________________##
//			}
//			BtSniffADDAReadySet(0);//清空ADDA准备标志
//			tws_link_status_set(1);
//			printf("tws_link_status_set(1)\n");
//		}

		//链路层断开后，发现sniff有标志没清。
		if((sniff_wakeup_get() /*|| sniff_lmpsend_get()*/)
#ifdef BT_TWS_SUPPORT
				&& (GetBtManager()->twsState != BT_TWS_STATE_CONNECTED)
#endif
				)
		{
			//断开后的sniff状态恢复
			sniff_wakeup_set(0);
			//sniff_lmpsend_set(0);

		}
	}
}

TIMER   sniffrerequsettimer;
#define RESEND_SCAN_TIME				2000		//醒来确认有效唤醒源 扫描限时ms。
void DeepSleeping_BT(void)
{
	uint32_t GpioAPU_Back,GpioAPD_Back,GpioBPU_Back,GpioBPD_Back;

	//Efuse_ReadDataDisable();
//	SysDeepsleepStart();

	BtStartEnterSniffMode();
	TimeOutSet(&sniffrerequsettimer, RESEND_SCAN_TIME);
	while((Bt_sniff_sniff_start_state_get() == 0) ||
			(Bt_sniff_sleep_state_get() == 0))
	{

		if(IsTimeOut(&sniffrerequsettimer))
		{
			APP_DBG("LMP sniff state ERR!!!\r\n");
			BtStartEnterSniffMode();
#ifdef BT_TWS_SUPPORT
			//断开连接后，跳出等待siff req，然后进入低功耗扫描
			if(GetBtManager()->twsState != BT_TWS_STATE_CONNECTED)
			{
				Bt_sniff_sniff_start();
				break;
			}
#endif
			TimeOutSet(&sniffrerequsettimer, RESEND_SCAN_TIME);
		}

		vTaskDelay(2);
	}


	GpioAPU_Back = GPIO_RegGet(GPIO_A_PU);//保存上下拉
	GpioAPD_Back = GPIO_RegGet(GPIO_A_PD);
	GpioBPU_Back = GPIO_RegGet(GPIO_B_PU);
	GpioBPD_Back = GPIO_RegGet(GPIO_B_PD);

#ifdef BT_TWS_SUPPORT
	if(tws_get_role() == BT_TWS_MASTER)
		BTSetAccessMode(BtAccessModeNotAccessible);
	
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
		DisableAdvertising();
#endif
#endif

#ifdef CFG_PARA_WAKEUP_SOURCE_POWERKEY
	SystermGPIOWakeupConfig(SYSWAKEUP_SOURCE6_POWERKEY, 42, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif

#if defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_ADCKEY, CFG_PARA_WAKEUP_GPIO_ADCKEY, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif

#ifdef CFG_IDLE_MODE_DEEP_SLEEP
	sources = 0;//休眠前清除所有唤醒中断。
	waitCECTimeFlag = 0;
#endif
	BTSniffSet();//准备进入sniff

	while(Bt_sniff_sniff_start_state_get())//没退出sniff消息，进入sniff休眠轮询。
	{
		vTaskDelay(1);
		if(Bt_sniff_sleep_state_get())
		{
			Bt_sniff_sleep_exit();
#ifdef BT_TWS_SUPPORT
			if(GetBtManager()->twsRole == BT_TWS_MASTER)
			{
				//此项目从机无UI，所以注释掉了从机唤醒的逻辑
				if(sniff_wakeup_check())// 如果出现唤醒标志次周期不睡，并且函数内部可以跳出sniff
				{
					continue;
				}
			}
#endif
			BtDeepSleepForUsr();
		}
	}

#ifdef BT_TWS_SUPPORT
	if(tws_get_role() == BT_TWS_MASTER)
	{
		if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
		{
			if(IsBtAudioMode())
			{
				BtExitSniffReconnectFlagSet();
			}
		}
		else
			BtExitSniffReconnectFlagSet();
		BTSetAccessMode(BtAccessModeGeneralAccessible);
	}
#endif

	GPIO_RegSet(GPIO_A_PU, GpioAPU_Back);
	GPIO_RegSet(GPIO_A_PD, GpioAPD_Back);
	GPIO_RegSet(GPIO_B_PU, GpioBPU_Back);
	GPIO_RegSet(GPIO_B_PD, GpioBPD_Back);

	Efuse_ReadDataEnable();

#ifdef BT_TWS_SUPPORT
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
	ble_advertisement_data_update();
#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
	BleScanParamConfig_Default();
#endif
#endif
	UartClkChange(APLL_CLK_MODE);


#ifdef CFG_FUNC_LED_REFRESH
	//默认优先级为0，旨在提高刷新速率，特别是断点记忆等写flash操作有影响刷屏，必须严格遵守所有timer6中断调用都是TCM代码，含调用的driver库代码
	//已确认GPIO_RegOneBitSet、GPIO_RegOneBitClear在TCM区，其他api请先确认。
	NVIC_SetPriority(Timer6_IRQn, 0);
 	Timer_Config(TIMER6,1000,0);
 	Timer_Start(TIMER6);
 	NVIC_EnableIRQ(Timer6_IRQn);

 	//此行代码仅仅用于延时，配合Timer中断处理函数，客户一定要做修改调整
 	//GPIO_RegOneBitSet(GPIO_A_OE, GPIO_INDEX2);//only test，user must modify
#endif

#if defined(CFG_FUNC_DISPLAY_EN)
    DispInit(0);
#endif
}

#else
void tws_stop_callback()//进入sniff时库里会调用
{

}

#endif //BT_SNIFF_ENABLE

