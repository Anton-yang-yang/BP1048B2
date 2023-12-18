/**
 **************************************************************************************
 * @file    ir_key.h
 * @brief   ir key
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2018-3-15 14:03:00$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef __IR_KEY_H__
#define __IR_KEY_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus
#include "type.h"
#include "app_config.h"

#define 	IR_KEY_NONE               0xFF
#define		IR_KEY_POWER				0 //必须与ir_key.c 键值表位置配合

#define	IR_KEY_TABLE	0xED,/*POWER*/		0xE5,/*MODE*/		0xE1,/*MUTE*/	\
							0xFE,/*PLAY/PAUSE*/	0xFD,/*PRE*/		0xFC,/*NEXT*/	\
							0xFB,/*EQ*/			0xFA,/*VOL-*/		0xF9,/*VOL+*/	\
							0xF8,/*0*/			0xF7,/*REPEAT*/		0xF6,/*SCN*/	\
							0xF5,/*1*/			0xE4,/*2*/			0xE0,/*3*/		\
							0xF3,/*4*/			0xF2,/*5*/			0xF1,/*6*/		\
							0xFF,/*7*/			0xF0,/*8*/			0xE6/*9*/


#if defined(CFG_RES_IR_KEY_SCAN) || defined(CFG_PARA_WAKEUP_SOURCE_IR)
	#define	CFG_RES_IR_KEY_USE
	#define CFG_RES_IR_NUMBERKEY //数字键操作功能
	#define CFG_PARA_IR_SEL					IR_MODE_NEC
	#define CFG_PARA_IR_BIT					IR_NEC_32BITS
	#define IR_MANU_ID						0x7F80//0xFF00//不同遥控器会不同,键值表ir_key.c\gIrVal[]
#endif


typedef enum _IRKeyType
{
	IR_KEY_UNKOWN_TYPE = 0,
	IR_KEY_PRESSED,
	IR_KEY_RELEASED,
	IR_KEY_LONG_PRESSED,
	IR_KEY_LONG_PRESS_HOLD,
	IR_KEY_LONG_RELEASED,
}IRKeyType;

typedef struct _IRKeyMsg
{
    uint16_t index;
    uint16_t type;
}IRKeyMsg;

uint8_t IRKeyIndexGet(void);
uint8_t IRKeyIndexGet_BT(uint32_t IrKeyVal);//为BT sniff IR唤醒加入键值查询
IRKeyMsg IRKeyScan(void);
int32_t IRKeyInit(void);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif //__IR_KEY_H__

