#ifndef __FLASH_BOOT_H__
#define __FLASH_BOOT_H__

#include "app_config.h"
#include "flash_table.h"
/*
版本说明：当前为V3.0.6版本
日期：2021年11月10日
*/
#define FLASH_BOOT_EN      1

//TX PIN
#define BOOT_UART_TX_OFF	0//关闭串口
#define BOOT_UART_TX_A0		1
#define BOOT_UART_TX_A1		2
#define BOOT_UART_TX_A6		3
#define BOOT_UART_TX_A10	4
#define BOOT_UART_TX_A19	5
#define BOOT_UART_TX_A25	6
#ifdef CFG_UART_TX_PORT
	#if CFG_UART_TX_PORT == 0
		#define BOOT_UART_TX_PIN	BOOT_UART_TX_A6
	#elif CFG_UART_TX_PORT == 1
		#define BOOT_UART_TX_PIN	BOOT_UART_TX_A10
	#elif CFG_UART_TX_PORT == 2
		#define BOOT_UART_TX_PIN	BOOT_UART_TX_A25
	#elif CFG_UART_TX_PORT == 3
		#define BOOT_UART_TX_PIN	BOOT_UART_TX_A0	
	#elif CFG_UART_TX_PORT == 4
		#define BOOT_UART_TX_PIN	BOOT_UART_TX_A1 
	#else
		#define BOOT_UART_TX_PIN	BOOT_UART_TX_A6
	#endif
#else
	#define BOOT_UART_TX_PIN	BOOT_UART_TX_OFF
#endif

//波特率配置
#define BOOT_UART_BAUD_RATE_9600	0
#define BOOT_UART_BAUD_RATE_115200	1
#define BOOT_UART_BAUD_RATE_256000	2
#define BOOT_UART_BAUD_RATE_512000	3
#define BOOT_UART_BAUD_RATE_1000000	4
#define BOOT_UART_BAUD_RATE_1500000	5
#define BOOT_UART_BAUD_RATE_3000000	6
#ifdef CFG_UART_BANDRATE
	#if CFG_UART_BANDRATE == 9600
		#define BOOT_UART_BAUD_RATE		BOOT_UART_BAUD_RATE_9600
	#elif CFG_UART_BANDRATE == 115200
		#define BOOT_UART_BAUD_RATE		BOOT_UART_BAUD_RATE_115200
	#elif CFG_UART_BANDRATE == 256000
		#define BOOT_UART_BAUD_RATE		BOOT_UART_BAUD_RATE_256000	
	#elif CFG_UART_BANDRATE == 512000
		#define BOOT_UART_BAUD_RATE		BOOT_UART_BAUD_RATE_512000	
	#elif CFG_UART_BANDRATE == 1000000
		#define BOOT_UART_BAUD_RATE		BOOT_UART_BAUD_RATE_1000000	
	#elif CFG_UART_BANDRATE == 1500000
		#define BOOT_UART_BAUD_RATE		BOOT_UART_BAUD_RATE_1500000	
	#elif CFG_UART_BANDRATE == 3000000
		#define BOOT_UART_BAUD_RATE		BOOT_UART_BAUD_RATE_3000000	
	#else
		#define BOOT_UART_BAUD_RATE		BOOT_UART_BAUD_RATE_512000
	#endif
#else
	#define BOOT_UART_BAUD_RATE		BOOT_UART_BAUD_RATE_512000
#endif

#define BOOT_UART_CONFIG	((BOOT_UART_BAUD_RATE<<4)+BOOT_UART_TX_PIN)


/*  JUDGEMENT_STANDARD说明
 * 分高4bit与低4bit：
 *   高4bit：
 *      为F则code按版本号升级
 *      为5则按code的CRC进行升级
 *   低4bit:
 *     为F则在升级code需要用到多大空间即擦除多大空间
 *     为5时则标识升级code前全部擦除芯片数据，即擦除“全片”（即除开flash的0地址开始flashboot占用空间不擦除以及最后8K不擦除）
 * 例如：0x5F 则为比较CODE CRC判断是否需要升级；升级时仅部分擦除，不进行全擦除
 */
#define JUDGEMENT_STANDARD		0x55

///升级接口定义
#define	SD_OFF				0x00
#define SD_A15A16A17		0x1
#define SD_A20A21A22		0x2
#if CFG_RES_CARD_GPIO == SDIO_A15_A16_A17
#define SD_PORT				SD_A15A16A17
#else
#define SD_PORT				SD_A20A21A22
#endif

#define UDisk_OFF			0x00
#define UDisk_ON			0x4

#define PCTOOL_OFF			0x00
#define PCTOOL_ON			0x08

#define	BTTOOL_OFF			0X00
#define BTTOOL_ON			0X10

#ifdef CFG_FUNC_BT_OTA_EN
#define UP_PORT				(BTTOOL_OFF + PCTOOL_ON + UDisk_ON + SD_PORT + BTTOOL_ON)
#else
#define UP_PORT				(BTTOOL_OFF + PCTOOL_ON + UDisk_ON + SD_PORT)//根据应用情况决定打开那些升级接口
#endif

#if FLASH_BOOT_EN
extern const unsigned char flash_data[];
#endif

#define USER_CODE_RUN_START		0 	//无升级请求直接运行客户代码
#define UPDAT_OK				1 	//有升级请求，升级成功
#define NEEDLESS_UPDAT			2	//有升级请求，但无需升级

#endif


#define BOOT_UART_CONFIG			((BOOT_UART_BAUD_RATE<<4)+BOOT_UART_TX_PIN)


#if FLASH_BOOT_EN
extern const unsigned char flash_data[];
#endif


