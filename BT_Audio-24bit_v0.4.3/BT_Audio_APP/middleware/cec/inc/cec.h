/**
 **************************************************************************************
 * @file    cec.h
 * @brief   cec
 *
 * @author  Cecilia Wang
 * @version V1.3.3
 *
 * $Created: 2019-6-25 11:40:00$
 *
 * @Copyright (C) 2019, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __CEC_H__
#define __CEC_H__

#include "dma.h"
#include "gpio.h"
#include "spdif.h"
#include "pwm.h"
#include "pwc.h"
#include "irqn.h"

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

typedef enum __CEC_IO_TYPE
{
	IO_TYPE_A = 0,//GPIOA
	IO_TYPE_B,    //GPIOB

} CEC_IO_TYPE;

typedef enum __CEC_WORK_STATUS
{
	CEC_IS_IDLE      = 0,
	CEC_IS_WORKING   = 1,
	CEC_IS_INACTIVE  = 2,

} CEC_WORK_STATUS;

typedef struct __CECMsg
{
    uint8_t 		Addr;              //地址
    uint8_t 		Operand[20];       //操作数, 最大参数长度可达14bytes，改长度为16
    uint8_t 		Len;               //操作数长度
    uint8_t         EndFlg;

} CECMsg;

typedef struct __MSGInfo
{
	uint8_t 		msg_status;
	uint8_t 		msg_bit_cnt;;
	uint8_t     	msg_data;
	uint8_t     	eom;
	uint8_t     	ack;

	CECMsg      	msg_queue[10];
	uint8_t			msg_queue_pos;

} MSGInfo;

typedef struct __CECInfo
{
	MSGInfo			msg_info;

	uint16_t		cec_send_flag;
	uint16_t    	cec_send_arbitration_flag;
	uint16_t    	cec_get_arbitration_flag;
	uint16_t        cec_arc_flag;
	uint32_t		cec_send_cnt;
	uint32_t		cec_get_cnt;
	uint32_t        cec_arc_cnt;

	uint8_t			physical_addr[2];
	uint8_t         vendor_id[3];
	uint8_t			data_pre[2];

	uint16_t        cec_send_arc_flag;
	uint16_t        cec_send_arc_cnt;
	uint16_t		cec_send_byte_cnt;

} CECInfo;


typedef struct __CECInitTypeDef
{
	uint8_t         pwm_io;
	uint8_t         pwm_io_pin;
	uint8_t         pwm_timer_id;
	PWM_StructInit  pwm_param;
	uint8_t         dma_tx_id;
	DMA_CONFIG      dma_tx_param;
	uint16_t       *pwm_tx_fifo;
	uint16_t		duty100;
	uint16_t		duty0;

	uint8_t         pwc_io;
	uint8_t         pwc_io_pin;
	uint8_t         pwc_timer_id;
	PWC_StructInit  pwc_param;
	uint8_t         dma_rx_id;
	DMA_CONFIG      dma_rx_param;

	CEC_IO_TYPE		cec_io_type;
	uint32_t	    cec_io_index;

	IRQn_Type       arbitration_timer_irq;
	uint32_t        arbitration_timer_id;

	uint32_t        hpd_status_io;
	uint32_t        hpd_status_io_pin;

	uint8_t			cec_check_flag;

	CECInfo         cec_info;

} CECInitTypeDef;

/**
 * @brief  HDMI CEC Init
 * @param  CECInitTypeDef *ct
 * @return None
 */
void HDMI_CEC_Init(CECInitTypeDef *ct);

/**
 * @brief Set HDMI HPD active level,and should be placed after HDMI_CEC_Init()
 * @param IsHighLevel: 1-High Level Active 0:Low Level Active
 * @return None
 */
void HDMI_CEC_HPDActiveLevelSet(uint8_t IsHighLevel);

/**
 * @brief  Get cec line current status
 * @param  isHPDActive: hpd line status
 * @return CEC_IS_IDLE / CEC_IS_WORKING / CEC_IS_INACTIVE
 */
CEC_WORK_STATUS HDMI_CEC_StatusGet(uint8_t isHPDActive);

/**
 * @brief  Judging CEC line whether to work or not
 * @param  None
 * @return CEC_IS_IDLE / CEC_IS_WORKING 
 */
CEC_WORK_STATUS HDMI_CEC_IsWorking(void);

/**
 * @brief  Judging CEC ready to deepsleep and Using internal HPD parameter.
 * @param  Timeout_Ms: Timeout
 * @return 0/1
 */
bool HDMI_CEC_IsReadytoDeepSleep(uint8_t Timeout_Ms);

/**
 * @brief  Judging CEC ready to deepsleep and Using external HPD parameter.
 * @param  Timeout_Ms: Timeout
 * @return 0/1
 */
bool HDMI_CEC_IsReadytoDeepSleepwithExHPD(uint8_t Timeout_Ms, uint8_t isHPDActive);

/**
 * @brief  Get CEC messages.
 * @param  operandValue: address pointer for operand code.
 * @return length of operandValue
 */
uint8_t HDMI_CEC_MessageDataGet(uint8_t *operandValue);

/**
 * @brief  Get tv maufacturer messages.
 * @param  in_ddc: input buf address, length = 128 Bytes.
 * @param  out_buf: output buf address, length = 5 Bytes;
 * @return length of operandValue
 */
void HDMI_DDC_TVManufacturerGet(uint8_t in_ddc[128], uint8_t out_buf[5]);

/**
 * @brief  HDMI CEC DeInit
 * @param  Timeout_Ms: Timeout.
 * @return None
 */
void HDMI_CEC_DeInit(uint8_t Timeout_Ms);

/***********Here is the CEC interaction information ***********************/
void HDMI_CEC_RoutingInfo(void);
void HDMI_CEC_AllocateLogicalAddress(void);
void HDMI_CEC_ReportPhysicalAddress(void);
void HDMI_CEC_ReportOSDName(void);
//osdname_len: 1~14bytes
//pwm_tx_fifo需要的大小为: (90 + 480 + 480 * 1 + 480 * osdname_len)bytes
void HDMI_CEC_ReportOSDNameWithParam(uint8_t *osdname_buf, uint8_t osdname_len);
void HDMI_CEC_SetSystemAudioModeOn(void);
void HDMI_CEC_SetSystemAudioModeoff(void);
void HDMI_CEC_DeviceVendorID(void);
void HDMI_CEC_DeviceVendorIDWithParam(uint8_t buf[3]);
bool HDMI_CEC_SystemAudioModeStatus(uint8_t Vol);
void HDMI_CEC_SystemPowerOn(void);
void HDMI_CEC_SystemPowerOff(void);
void HDMI_CEC_Version(void);
void HDMI_CEC_FeatureAbort(uint8_t code, uint8_t reason);
void HDMI_CEC_AudioFormatCode(void);
void HDMI_CEC_SystemAudioModeStatusOn(void);
void HDMI_CEC_SystemAudioModeStatusOff(void);
void HDMI_CEC_RequestActiveSource(void);
void HDMI_CEC_InitiateARC(void);
void HDMI_CEC_TerminationARC(void);
//param_len: 0~14bytes;
//pwm_tx_fifo需要的大小为: (90 + 480 + 480 * 1 + 480 * param_len)bytes
void HDMI_CEC_UserDefined(uint8_t logical_addr, uint8_t opcode_value, uint8_t* param_buf, uint8_t param_len);

void HDMI_Arbitration_Time_Process(uint8_t get_time, uint8_t send_time);

const unsigned char *GetLibVersionCEC(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __CEC_H__
