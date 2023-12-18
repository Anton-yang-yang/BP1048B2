/**
 **************************************************************************************
 * @file    communication.h
 * @brief
 *
 * @author  Cecilia Wang
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#include <stdint.h>
#include "ctrlvars.h"


#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus


void HIDUsb_Tx(uint8_t *buf,uint16_t len);
void HIDUsb_Rx(uint8_t *buf,uint16_t len);


#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__COMMUNICATION_H__
