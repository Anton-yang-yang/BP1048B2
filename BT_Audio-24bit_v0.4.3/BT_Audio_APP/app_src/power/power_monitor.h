//  maintainer: 
#ifndef __POWER_MONITOR_H__
#define __POWER_MONITOR_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

// 电池电压等级数，0 ~ 10，0表示显示低电关机，1表示低电需提示
typedef enum _PWR_LEVEL
{
	PWR_LEVEL_0 = 0,
	PWR_LEVEL_1,
	PWR_LEVEL_2,
	PWR_LEVEL_3,
	PWR_LEVEL_4,
	PWR_LEVEL_5,
	PWR_LEVEL_6,
	PWR_LEVEL_7,
	PWR_LEVEL_8,
	PWR_LEVEL_9,
	PWR_FULL,      //max level
} PWR_LEVEL;

/*
**********************************************************
*					函数声明
**********************************************************
*/
//电能监视初始化
//实现系统启动过程中的低电压检测处理，以及配置充电设备接入检测IO等
void PowerMonitorInit(void);

//获取当前电池电量,仅用于上传电量给手机端
//return: level(0-9)
PWR_LEVEL PowerLevelGet(void);

//电池电压采样
void PowerVoltageSampling(void);

void BatteryScan(void);

void PowerMonitorDisp(void);

extern void SetBtHfpBatteryLevel(PWR_LEVEL level, uint8_t flag);


#ifdef  __cplusplus
}
#endif//__cplusplus

#endif
