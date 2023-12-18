#ifndef _SYS_PARAM_H__
#define _SYS_PARAM_H__

#include "sys_param_typedef.h"

//注意事项:
//1.下面所有参数默认上电后从flash中读取,需要在\app_src\system_config\parameter.ini中修改。
//2.如需要修改SDK蓝牙名称,在\app_src\system_config\parameter.ini中,直接修改bt_LocalDeviceName
//3.如需使用固定的名称,请移步到bt_app_func.c中LoadBtConfigurationParams函数内修改获取方式
//4.此处的BT_NAME是系统保留的蓝牙名
//5.BLE的名称修改在ble广播数据中体现(ble_app_func.c)
#define BT_NAME						"BP10_BT"

//trim范围:0x07~0x1d -- 针对于老芯片
#define BT_TRIM_ECO0				0x16 
//trim范围:0x00~0x1f
#define BT_TRIM						0x14 

/* Rf Tx Power Range 
{   level  dbm
	[23] = 8,
	[22] = 6,
	[21] = 4,
	[20] = 2,
	[19] = 0,
	[18] = -2,
	[17] = -4,
	[16] = -6,
	[15] = -8,
	[14] = -10,
	[13] = -13,
	[12] = -15,
	[11] = -17,
	[10] = -19,
	[9]  = -21,
	[8]  = -23,
	[7]  = -25,
	[6]  = -27,
	[5]  = -29,
	[4]  = -31,
	[3]  = -33,
	[2]  = -35,
	[1]  = -37,
	[0]  = -39,
}
*/
//蓝牙正常工作时发射功率
#define BT_TX_POWER_LEVEL			22//19 //Tony 20221109 for delay
//蓝牙回连发射功率
#define BT_PAGE_TX_POWER_LEVEL		16 

//bt 铃声设置
//0 -> 不支持来电铃声
//1 -> 来电报号和铃声
//2 -> 使用手机铃声，若没有则播本地铃声
//3 -> 强制使用本地铃声
enum
{
	USE_NULL_RING = 0,
	USE_NUMBER_REMIND_RING = 1,
	USE_LOCAL_AND_PHONE_RING = 2,
	USE_ONLY_LOCAL_RING = 3,
};

//BT 后台设置
//0 -> BT后台不能连接手机
//1 -> BT后台可以连接手机
//2 -> 无后台
enum
{
	BT_BACKGROUND_FAST_POWER_ON_OFF = 0,
	BT_BACKGROUND_POWER_ON = 1,
	BT_BACKGROUND_DISABLE = 2,
};

#define BT_NAME_SIZE				sizeof(((SYS_PARAMETER *)0)->bt_LocalDeviceName)
#define BLE_NAME_SIZE				sizeof(((SYS_PARAMETER *)0)->ble_LocalDeviceName)

extern SYS_PARAMETER sys_parameter;
extern void sys_parameter_init(void);

#endif


