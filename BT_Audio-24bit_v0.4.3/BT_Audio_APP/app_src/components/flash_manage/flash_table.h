#ifndef __FLASH_TABLE_H__
#define __FLASH_TABLE_H__
#include "type.h"

// 8  --> 8Mb		
// 4/8/16/32/64/128
#if defined(FUNC_OS_DEBUG) || defined(CFG_FUNC_STRING_CONVERT_EN)   //如需中文字库，flash容量需要选择16M
		#define FLASH_CAPACTITY_TYPE	16	//系统调试模式下,flash容量选择16Mb
#else
	#if defined (CFG_CHIP_BP1048B1) || defined (CFG_CHIP_MT166) ||  defined (CFG_CHIP_BP1032A1)
		#define FLASH_CAPACTITY_TYPE	8	//flash默认容量 8Mb
	#else
		#define FLASH_CAPACTITY_TYPE	16	//flash默认容量 8Mb
	#endif
#endif

#define FLASH_CAPACTITY_ADDR	0xD4 

#define FLASH_DEFAULT_CAPACTITY	((FLASH_CAPACTITY_TYPE*1024*1024)/8)	
//flash data map
//采用倒序的方式(4KB对齐)
//仅供参考，实际的分布情况由\tools\merge_script\merge.ini文件决定
typedef struct _FLASH_DATA_MAP_
{
	//末尾8KB参量化区域(兼容旧SDK做的保留)
	uint8_t	reserve_1		[1024 * 4];		//size:4KB
	uint8_t	reserve_2		[1024 * 4];		//size:4KB

	uint8_t	flash_table		[1024 * 4];		//size:4KB	索引表
	uint8_t	sys_parameter	[1024 * 4];		//size:4KB	8MB SDK参量化数据
	uint8_t	bt_data			[1024 * 8];		//size:8KB  bt数据 配对记录等
	uint8_t	bp_data			[1024 * 8];		//size:8KB	breakpoint数据
	uint8_t	effect_data		[1024 * 16];	//size:16KB 音效数据
	uint8_t	remind			[1024 * 152];	//size:152KB 提示音
}FLASH_DATA_MAP;

#define GET_MEMBER_SIZE(member)		sizeof(((FLASH_DATA_MAP *)0)->member)
#define GET_MEMBER_OFFSET(member)	(uint32_t)((((FLASH_DATA_MAP *)0)->member) + GET_MEMBER_SIZE(member))

#define BT_CONFIG_OFFSET			GET_MEMBER_OFFSET(reserve_1)
#define USER_CONFIG_OFFSET			GET_MEMBER_OFFSET(reserve_2)
#define FLASH_TABLE_OFFSET			GET_MEMBER_OFFSET(flash_table)
#define BT_DATA_OFFSET     			GET_MEMBER_OFFSET(bt_data)
#define BP_DATA_OFFSET     			GET_MEMBER_OFFSET(bp_data)
#define EFFECT_DATA_OFFSET     		GET_MEMBER_OFFSET(effect_data)
#define REMIND_DATA_OFFSET			GET_MEMBER_OFFSET(remind)
#define SYS_PARA_OFFSET				GET_MEMBER_OFFSET(sys_parameter)

#define	SDK_DATA_SIZE				sizeof(FLASH_DATA_MAP)	//8MB SDK数据区大小
#define BP_DATA_FLASH_SIZE			GET_MEMBER_SIZE(bp_data)
#define FLASH_TABLE_SIZE			GET_MEMBER_SIZE(flash_table)

void flash_table_init(void);
uint32_t get_remind_addr(void);
uint32_t get_effect_data_addr(void);
uint32_t get_bp_data_addr(void);
uint32_t get_bt_data_addr(void);
uint32_t get_user_config_addr(void);
uint32_t get_bt_config_addr(void);
uint32_t get_sys_parameter_addr(void);
bool flash_table_is_valid(void);

#define REMIND_FLASH_STORE_BASE				(get_remind_addr()) //系统flash空间分配决定
#define REMIND_FLASH_STORE_OVERFLOW			(get_bp_data_addr())


/////////////////////////////////////////////////////////////////
//下面flash的定义是无效的，只是暂时解决编译错误
#define	CODE_ADDR				0x0
#define CONST_DATA_ADDR    		FLASH_DEFAULT_CAPACTITY
#define AUDIO_EFFECT_ADDR  		FLASH_DEFAULT_CAPACTITY		
#define FLASHFS_ADDR			FLASH_DEFAULT_CAPACTITY		
#define USER_DATA_ADDR     		FLASH_DEFAULT_CAPACTITY	
#define BP_DATA_ADDR     		FLASH_DEFAULT_CAPACTITY

#endif

