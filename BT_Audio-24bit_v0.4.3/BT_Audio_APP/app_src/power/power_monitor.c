///////////////////////////////////////////////////////////////////////////////
//               Mountain View Silicon Tech. Inc.
//                       All rights reserved.
//               Filename: power_management.c
///////////////////////////////////////////////////////////////////////////////
#include "type.h"
#include "app_config.h"
#include "adc.h"
#include "clk.h"
#include "gpio.h"
#include "timeout.h"
#include "adc_key.h"
#include "debug.h"
#include "sadc_interface.h"
#include "delay.h"
#include "bt_config.h"
#include <string.h>
#include "main_task.h"

#ifdef CFG_FUNC_POWER_MONITOR_EN
#include "power_monitor.h"

#define LDOIN_POWER_OFF_CYCLE       30      //低电关机检测时间 15s
#define LDOIN_LOW_WARNING_CYCLE     120     //低电1分钟播报一次
#define BAT_DET_VALUES_MAX          20      //10秒钟去抖窗口
#define BAT_DET_WIN_TIME            30      //电池电量检测窗口 15S
#define BAT_DET_WIN_COUNT           10      //10个窗口观察，150s稳定后升高电量


#define LDOIN_SAMPLE_COUNT			30      //获取LDOIN幅度时用来平均的采样次数
#define LDOIN_SAMPLE_PERIOD			50	    //获取LDOIN幅度时获取采样值的间隔(ms)
#define LDOIN_DETECT_PERIOD			500	    //上层获取电量等级间隔(ms)


//以下定义不同的电压检测事件的触发电压(单位mV)，用户根据自身系统电池的特点来配置
#define LDOIN_VOLTAGE_FULL			4150    //满电电压
#define LDOIN_VOLTAGE_9			    4100
#define LDOIN_VOLTAGE_8			    4000
#define LDOIN_VOLTAGE_7			    3900
#define LDOIN_VOLTAGE_6			    3800
#define LDOIN_VOLTAGE_5			    3740
#define LDOIN_VOLTAGE_4			    3680
#define LDOIN_VOLTAGE_3			    3600
#define LDOIN_VOLTAGE_LOW			3500    //低于此电压值提示低电电压
#define LDOIN_VOLTAGE_OFF			3350	//低于此电压值关机

#ifdef BAT_VOL_DET_LRADC
const uint16_t LRADC_GradeVol[PWR_FULL + 1] =
{
    0,                 //返回 0 -- 低电关机
    LDOIN_VOLTAGE_OFF, //返回 1 -- 低电报警
    LDOIN_VOLTAGE_LOW, //返回 2
    LDOIN_VOLTAGE_3,   //返回 3
    LDOIN_VOLTAGE_4,   //返回 4
    LDOIN_VOLTAGE_5,   //返回 5
    LDOIN_VOLTAGE_6,   //返回 6
    LDOIN_VOLTAGE_7,   //返回 7
    LDOIN_VOLTAGE_8,   //返回 8
    LDOIN_VOLTAGE_9,   //返回 9
    LDOIN_VOLTAGE_FULL,//返回 10 -- 满电电压
};
#else
const uint16_t LdoinGradeVol[PWR_FULL + 1] =
{
	0,                 //返回 0 -- 低电关机
	LDOIN_VOLTAGE_OFF, //返回 1 -- 低电报警
	LDOIN_VOLTAGE_LOW, //返回 2
	LDOIN_VOLTAGE_3,   //返回 3
	LDOIN_VOLTAGE_4,   //返回 4
	LDOIN_VOLTAGE_5,   //返回 5
	LDOIN_VOLTAGE_6,   //返回 6
	LDOIN_VOLTAGE_7,   //返回 7
	LDOIN_VOLTAGE_8,   //返回 8
	LDOIN_VOLTAGE_9,   //返回 9
	LDOIN_VOLTAGE_FULL,//返回 10 -- 满电电压
};
#endif

//用于电压检测的变量
TIMER PowerMonitorTimer;
TIMER PowerMonitorDetectTimer;

uint16_t LdoinSampleCnt;            //LDOIN_SAMPLE_COUNT;
uint16_t LdoinLevelAverage;		    //当前LDOIN电压平均值
uint16_t bat_low_conuter;           //低电报警计时器
uint16_t bat_low_power_off_conuter; //低电关机计时器

uint16_t bat_value_win_min[BAT_DET_WIN_COUNT]; //每15秒一个窗口统计最小值
uint16_t bat_values[BAT_DET_VALUES_MAX];
uint16_t LdoinSampleVal[LDOIN_SAMPLE_COUNT];

uint16_t bat_value_index;
uint16_t bat_value_cur_win_min;  //当前窗口最小电量值
uint16_t bat_value_win_tick;     //检测窗口计时
uint16_t bat_value_win_count;    //已经统计窗口数
uint16_t bat_value_counter;      //电池电压降低计时器

static PWR_LEVEL PowerLevel = PWR_FULL;  //当前系统电量值

#ifdef	CFG_FUNC_OPTION_CHARGER_DETECT
//硬件检测PC 或充电器连接状态
//使能内部下拉。无PC或充电器连接时，检测口为低电平，有时检测口为高电平
bool IsInCharge(void)
{
	//设为输入，无上下拉
	GPIO_PortAModeSet(CHARGE_DETECT_GPIO, 0x0);
	GPIO_RegOneBitSet(CHARGE_DETECT_PORT_PU, CHARGE_DETECT_GPIO);
	GPIO_RegOneBitClear(CHARGE_DETECT_PORT_PD, CHARGE_DETECT_GPIO);
	GPIO_RegOneBitClear(CHARGE_DETECT_PORT_OE, CHARGE_DETECT_GPIO);
	GPIO_RegOneBitSet(CHARGE_DETECT_PORT_IE, CHARGE_DETECT_GPIO);
	WaitUs(2);
	if(GPIO_RegOneBitGet(CHARGE_DETECT_PORT_IN,CHARGE_DETECT_GPIO))
	{
		return TRUE;
	}   	

	return FALSE;
}
#endif

//变量初始化
void battery_detect_var_init(void)
{
	uint8_t i;
	
	bat_value_index = 0;
	bat_value_cur_win_min = PWR_FULL;
	bat_value_win_tick = 0;
	bat_value_win_count = 0;
	bat_value_counter = 0;

	LdoinSampleCnt = 0;
	LdoinLevelAverage = 0;
	bat_low_conuter = 0;
	bat_low_power_off_conuter = 0;

	TimeOutSet(&PowerMonitorTimer, 0);	
	TimeOutSet(&PowerMonitorDetectTimer, 0);	

	for(i=0; i<BAT_DET_VALUES_MAX; i++)
	{
		bat_values[i] = PWR_FULL;
	}
}


//电池电压采样
void PowerVoltageSampling(void)
{
	uint16_t bat_vol;
	
	if(IsTimeOut(&PowerMonitorTimer))
	{
		TimeOutSet(&PowerMonitorTimer, LDOIN_SAMPLE_PERIOD);

		#ifdef BAT_VOL_DET_LRADC
		bat_vol = ADC_SingleModeDataGet(BAT_VOL_LRADC_CHANNEL_PORT);
		#else
		bat_vol = SarADC_LDOINVolGet();
		#endif
		
		//采样值小于1V则认为当前检测有误，返回满电
		if(bat_vol < 1000)
		{
			bat_vol = LDOIN_VOLTAGE_FULL;
		}
		
		if(LdoinSampleCnt >= LDOIN_SAMPLE_COUNT)
		{
			LdoinSampleCnt = 0;
		}
		
		//电池电压采样
		LdoinSampleVal[LdoinSampleCnt] = bat_vol;
		LdoinSampleCnt++;
	}
}

//获取电量等级
uint16_t GetPowerVoltage(void)
{
    uint32_t bat_val;
    int16_t i;

    bat_val = 0;
    for (i = 0; i < LDOIN_SAMPLE_COUNT; i++)
    {
        bat_val += LdoinSampleVal[i];
    }	

	LdoinLevelAverage = (uint16_t)(bat_val / LDOIN_SAMPLE_COUNT);
	APP_DBG("LDOin 5V Volt: %lu\n", (uint32_t)LdoinLevelAverage);

	if (LdoinLevelAverage > LDOIN_VOLTAGE_FULL)
	{
		return PWR_FULL;
	}
	else
	{
	    for (i = PWR_FULL; i >= 0; i--)
	    {
	    	#ifdef BAT_VOL_DET_LRADC
	        if (LdoinLevelAverage >= LRADC_GradeVol[i])
			#else
			if (LdoinLevelAverage >= LdoinGradeVol[i])
			#endif
	        {
	            return i;
	        }
	    }
	}

	return 0;
}


//防抖处理
PWR_LEVEL ShakeEliminationProcessing(void)
{
	uint16_t i;
	uint16_t bat_value;
	uint16_t last_bat_value;
	uint16_t tmp_bat_value_min;
	uint16_t tmp_bat_value_min_wins;
	uint16_t bat_value_min_cn;
	bool bat_det_stable_flag;
	
	last_bat_value = PowerLevel;
	
	bat_value = GetPowerVoltage();
	if (bat_value > PWR_FULL)
	{
		bat_value = PWR_FULL;
	}

	//电量为0则快速降低
	if(bat_value == 0)
	{
		if (PowerLevel > 0)
		{
			PowerLevel--;
		}
		DBG("bat_value down: %d -> %d\n", last_bat_value, PowerLevel);
		bat_value_counter = 0;
	}
	else
	{
        bat_values[bat_value_index] = bat_value;
        bat_value_index++;
        if (bat_value_index >= BAT_DET_VALUES_MAX)	
        {
			bat_value_index = 0;
		}
		
        tmp_bat_value_min = PWR_FULL;
		//找最近的最小电量值
        for (i = 0; i < BAT_DET_VALUES_MAX; i++)
        {
            if (bat_values[i] < tmp_bat_value_min)
            {
                tmp_bat_value_min = bat_values[i];
            }
        }		

        //统计窗口
        if (tmp_bat_value_min < bat_value_cur_win_min)
        {
            bat_value_cur_win_min = tmp_bat_value_min;
        }		

        bat_value_win_tick++;
        if (bat_value_win_tick >= BAT_DET_WIN_TIME)
        {
        	//统计10个窗口观测
    		if (bat_value_win_count >= BAT_DET_WIN_COUNT)
			{
				bat_value_win_count = 0;
			}
            if (bat_value_win_count < BAT_DET_WIN_COUNT)
            {
                bat_value_win_min[bat_value_win_count] = bat_value_cur_win_min;
                bat_value_win_count++;
            }
			
            bat_value_win_tick = 0;
			bat_value_cur_win_min = PWR_FULL;
        }

		//电压下降
		if (tmp_bat_value_min < PowerLevel)
		{
            bat_det_stable_flag = TRUE;
			bat_value_min_cn = 0;
            for (i = 0; i < BAT_DET_VALUES_MAX; i++)
            {
            	//最近20次电量小于系统电量次数
            	if (PowerLevel > bat_values[i])
            	{
					bat_value_min_cn++;
				}
				//偏差超过1，电量不稳定
                if ((bat_values[i] - tmp_bat_value_min) > 1)
                {
                    bat_det_stable_flag = FALSE;
                }
            }		

			bat_value_counter++;
			//如果电压很稳定，且最近20次有10次电压小于系统电量，则马上降低电压
			if ((bat_det_stable_flag == TRUE)&&(bat_value_min_cn >= 10))
			{
				bat_value_counter = 0;
				if (PowerLevel > tmp_bat_value_min)
				{
					PowerLevel--;
				}
				DBG("bat_value down1: %d -> %d\n", last_bat_value, PowerLevel);
			}
			else if (bat_value_counter >= BAT_DET_WIN_TIME) //如果电压不稳定，则需要持续15秒钟才允许调低电量，防止抖动
			{
				if (PowerLevel > tmp_bat_value_min)
				{
					PowerLevel--;
					DBG("bat_value down2: %d -> %d\n", last_bat_value, PowerLevel);
				}

				bat_value_counter = 0;
			}
		}
		else if (tmp_bat_value_min > PowerLevel)
		{
			//需要持续120s才允许升高电量，防止抖动
			if ((bat_value_win_tick == 0)&&(bat_value_win_count == BAT_DET_WIN_COUNT))
			{
                tmp_bat_value_min_wins = PWR_FULL;
                for (i = 0; i < BAT_DET_WIN_COUNT; i++)
                {
                    if (bat_value_win_min[i] < tmp_bat_value_min_wins)
                    {
                        tmp_bat_value_min_wins = bat_value_win_min[i];
                    }
                }	
				if (tmp_bat_value_min_wins > PowerLevel)
				{
					PowerLevel++;
                    DBG("bat_value up: %d -> %d\n", last_bat_value, PowerLevel);
				}
			}
		}
	}

	return PowerLevel;
}


//电池电量
void BatteryScan(void)
{
	if(IsTimeOut(&PowerMonitorDetectTimer))
	{
		TimeOutSet(&PowerMonitorDetectTimer, LDOIN_DETECT_PERIOD);	
		
		//获取消除抖动后的电量等级
		PowerLevel = ShakeEliminationProcessing();
		//DBG("PowerLevel = %d\n", PowerLevel);
		
#ifdef CFG_FUNC_OPTION_CHARGER_DETECT
		//充电设备已经接入，就不执行下面这段低电压检测和处理过程
		if(IsInCharge()) 
		{
			bat_low_conuter = 0;
			bat_low_power_off_conuter = 0;
		}
		else
#endif
		{
			if (PowerLevel == PWR_LEVEL_0)
			{
				bat_low_power_off_conuter++;
				//关机检测15s
				if (bat_low_power_off_conuter == LDOIN_POWER_OFF_CYCLE)
				{
					//先报低电提示音
					BatteryLowMessage();
				}
				else if (bat_low_power_off_conuter >= (LDOIN_POWER_OFF_CYCLE+6))
				{
					bat_low_power_off_conuter = 0;
					//延时3s再关机
					PowerOffMessage();
				}
			}
			else if (PowerLevel == PWR_LEVEL_1)
			{
				bat_low_conuter++;
				//低电报警
				if (bat_low_conuter > LDOIN_LOW_WARNING_CYCLE)
				{
					bat_low_conuter = 0;
					BatteryLowMessage();
				}
			}
			else
			{
				bat_low_conuter = 0;
				bat_low_power_off_conuter = 0;
			}
		}

		//上传电池电量到手机端
#ifdef CFG_APP_BT_MODE_EN
#if (BT_HFP_SUPPORT == ENABLE)
		SetBtHfpBatteryLevel(PowerLevelGet(), 0);
#endif
#endif		
		//电量显示
		PowerMonitorDisp();
	}
}

//电能监视初始化
//实现系统启动过程中的低电压检测处理，以及初始化充电设备接入检测IO等
void PowerMonitorInit(void)
{
	uint32_t bat_vol;
	uint8_t i;

	i = 0;
	bat_vol = 0;
	battery_detect_var_init();

	//init
	#ifdef BAT_VOL_DET_LRADC
	GPIO_RegOneBitSet(BAT_VOL_LRADC_CHANNEL_ANA_EN, BAT_VOL_LRADC_CHANNEL_ANA_MASK);
	#endif

	//系统启动过程中的低电压检测
	//开机时电压检测，如果小于开机电压，不进入设备检测和播放流程，直接关机
	//检测过程为时50ms，并对系统电量进行初始化
	while(i < 10)
	{
		#ifdef BAT_VOL_DET_LRADC
		bat_vol += ADC_SingleModeDataGet(BAT_VOL_LRADC_CHANNEL_PORT);
		#else
		bat_vol += SarADC_LDOINVolGet();
		#endif
		i++;
		vTaskDelay(5);
	}

	LdoinLevelAverage = (uint16_t)(bat_vol/10);
	for(i=0; i<LDOIN_SAMPLE_COUNT; i++)
	{
		LdoinSampleVal[i] = LdoinLevelAverage;
	}

	PowerLevel = GetPowerVoltage();
#ifdef CFG_FUNC_OPTION_CHARGER_DETECT
	if(!IsInCharge()) //如果系统启动时，充电设备已经接入，则不提示低电与关机
#endif
	{
		if (PowerLevel == PWR_LEVEL_1)
		{
			//开机低电，5s后报警
			bat_low_conuter = LDOIN_LOW_WARNING_CYCLE - 10;
		}
		else if (PowerLevel == PWR_LEVEL_0)
		{
			//开机低电，马上关机
			bat_low_power_off_conuter = LDOIN_POWER_OFF_CYCLE - 1;
		}
	}

	//电量显示
	PowerMonitorDisp();
}


//获取当前电池电量,仅用于上传电量给手机端
//return: level(0-9)
PWR_LEVEL PowerLevelGet(void)
{
	PWR_LEVEL res = 0;

	//对应手机端0% ~ 100%电量，上传手机时自减一级电量
	if (PowerLevel > 0)
	{
		res = PowerLevel - 1;
	}
	return res;
}

//电量显示处理
void PowerMonitorDisp(void)
{
#if (defined(FUNC_SEG_LED_EN) || defined(FUNC_SEG_LCD_EN) ||defined(FUNC_TM1628_LED_EN))
	static bool IsToShow = FALSE;

#ifdef CFG_FUNC_OPTION_CHARGER_DETECT
	static uint8_t  ShowStep = 0;
	if(IsInCharge())      //充电器已经接入的处理
	{
		switch(ShowStep)
		{
			case 0:
			DispIcon(ICON_BAT1, FALSE);
			DispIcon(ICON_BAT2, FALSE);
			DispIcon(ICON_BAT3, FALSE);
			break;
			case 1:
			DispIcon(ICON_BAT1, TRUE);
			DispIcon(ICON_BAT2, FALSE);
			DispIcon(ICON_BAT3, FALSE);
			break;
			case 2:
			DispIcon(ICON_BAT1, TRUE);
			DispIcon(ICON_BAT2, TRUE);
			DispIcon(ICON_BAT3, FALSE);
			break;
			case 3:
			DispIcon(ICON_BAT1, TRUE);
			DispIcon(ICON_BAT2, TRUE);
			DispIcon(ICON_BAT3, TRUE);
			break;
		}		
		
		ShowStep++;
		if(ShowStep > 3)
		{
			ShowStep = 0;
		}
	}
	else
#endif
	{
		switch(PowerLevel)
		{
			case PWR_FULL
			case PWR_LEVEL_9:
			DispIcon(ICON_BATFUL, TRUE);
			DispIcon(ICON_BATHAF, FALSE);
			//可以显示满格电量，请添加显示代码			
			break;

			case PWR_LEVEL_8:
			case PWR_LEVEL_7:
			case PWR_LEVEL_6:
			DispIcon(ICON_BATFUL, FALSE);
			DispIcon(ICON_BATHAF, TRUE);
			//可以显示3格电量，请添加显示代码			
			break;

			case PWR_LEVEL_5:
			case PWR_LEVEL_4:
			DispIcon(ICON_BATFUL, FALSE);
			DispIcon(ICON_BATHAF, TRUE);
			//可以显示2格电量，请添加显示代码	
			break;

			case PWR_LEVEL_3:
			case PWR_LEVEL_2:
			DispIcon(ICON_BATFUL, FALSE);
			DispIcon(ICON_BATHAF, TRUE);
			//可以显示1格电量，请添加显示代码			
			break;

			case PWR_LEVEL_1:
			case PWR_LEVEL_0:
			DispIcon(ICON_BATFUL, FALSE);
			if(IsToShow)
			{
				DispIcon(ICON_BATHAF, TRUE);
			}
			else
			{
				DispIcon(ICON_BATHAF, FALSE);
			}
			IsToShow = !IsToShow;
			//可以显示0格电量，请添加显示代码	
			break;
			
			default:
			break;
		}
	}
#endif
}

#endif
