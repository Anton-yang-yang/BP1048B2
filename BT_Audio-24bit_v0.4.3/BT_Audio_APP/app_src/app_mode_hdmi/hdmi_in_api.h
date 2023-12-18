/**
 **************************************************************************************
 * @file    hdmi_in_api.h
 * @brief   hdmi in
 *
 * @author  Cecilia Wang
 * @version V1.0.0
 *
 * $Created: 2018-1-5 11:40:00$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __HDMI_IN_API_H__
#define __HDMI_IN_API_H__

#include "dma.h"
#include "gpio.h"
#include "spdif.h"
#include "pwm.h"
#include "pwc.h"
#include "mcu_circular_buf.h"
#include "cec.h"
#include "timeout.h"

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#define HDMI_COMPATIBLE_TV_NUM   12

typedef enum __TV_TYPE
{
	TV_DEFAULT  		= 0,//default
	TV_SAMSUNG_1670		= 1,//Samsung
	TV_HISENSE_3000	    = 2,//Hisense
	TV_SONY_03EF        = 3,//Sony
	TV_SONY_0537        = 4,//Sony-kd-55x8588g
	TV_POLARIOD_010B    = 5,//Polariod
	TV_LETV_0300		= 6,//Letv
	TV_SONY_047C		= 7,//Sony
	TV_XIAOMI_XMD       = 8,//xiao mi
	TV_SONY_04A2		= 9,//Sony
	TV_SAMSUNG_170F     = 10,//xiao mi
	TV_TCL_2009         = 11,//TCL

} TV_TYPE;

typedef void (*HDMICECScanFunc)(uint8_t isWorkStatus);
typedef struct __HDMI_TV_Info
{
	TV_TYPE  		tv_type;           //类型
	char     		tv_vendor[5];      //vendor
	uint32_t		tv_reaction_time;  //上电后的反应时间
	HDMICECScanFunc HDMI_CEC_SCAN_Func;//cec scan function
	uint8_t		    SendArbitrTime;    //紧接着发送第二次,单位：ms
	uint8_t		    GetArbitrTime;     //回复数据,单位：ms

} HDMI_TV_Info;

typedef enum __HDMI_REPORT_STATUS
{
	HDMI_INIT_STATUS		  = 0,
	HDMI_ALLOCATE_ADDR        = 1,
	HDMI_REPORT_PHYSICAL_ADDR = 2,
	HDMI_REQUEST_ACTIVE_SRC   = 3,
    HDMI_SET_AUDIO_MODE       = 4,

} HDMI_REPORT_STATUS;

typedef enum __HDMI_HPD_STATUS
{
	HDMI_HPD_NOT_CONNECTED_STATUS = 0,//未连接
	HDMI_HPD_CONNECTED_STATUS     = 1,//连接

} HDMI_HPD_STATUS;

typedef struct __HDMIInfo
{
	uint8_t			hdmiHPDCheck;
	uint8_t         hdmiReportStatus;
	int8_t          hdmi_poweron_flag;//-1:开始有在线进入不在线， 0:不在线中， 1：开始由不在线-->在线
	uint8_t         hdmi_audiomute_flag;
	uint8_t			hdmi_arc_flag;
	uint8_t			hdmiReportARCStatus;
	uint16_t        hdmiBusyTime;

	uint8_t         hdmiGiveReportStatus;
	uint8_t			hdmiActiveReportVolUpDownflag;
	uint8_t			hdmiActiveReportMuteflag;
	uint8_t			hdmiActiveReportMuteStatus;
	uint8_t			hdmiActiveReportMuteAfterInit;//这个标志，防止有些隔离不好的板级因串扰原因反复unlock/lock，从而反复汇报。

	uint8_t         edid_buf[256];

	HDMI_TV_Info	hdmi_tv_inf;
	TIMER			time_reaction;
	TIMER			time_waitTime;
	uint8_t         hdmiReTransCnt;

} HDMIInfo;



void 	 HDMI_CEC_DDC_Init(void);
//isWorkStatus: 0: deepsleep模式下扫描， 1： 正常工作时扫描
void 	 HDMI_CEC_Scan(uint8_t isWorkStatus);
void 	 HDMI_CEC_DDC_DeInit(void);

void 	 HDMI_ARC_Init(uint16_t *DMAFifoAddr, uint32_t DMAFifoSize, MCU_CIRCULAR_CONTEXT *ct);
uint8_t  HDMI_ARC_LockStatusGet(void);
uint16_t HDMI_ARC_DataLenGet(void);
uint16_t HDMI_ARC_DataGet(void *pcm_out, uint16_t MaxSize);
void     HDMI_ARC_DeInit(void);
//0: not ready, 1: ready
bool 	 HDMI_ARC_IsReady(void);
void     HDMI_ARC_OnOff(uint8_t ArcOnFlag);
void 	 HDMI_CEC_ActivePowerOff(uint32_t timeout_value);

//HPD
void HDMI_HPD_CHECK_IO_INIT(void);
HDMI_HPD_STATUS HDMI_HPD_StatusGet(void);
bool HDMI_HPD_INTFlagGet(void);
void HDMI_HPD_INTFlagClear(void);

//
void     HDMI_CEC_Default_Scan(uint8_t isWorkStatus);
void 	 HDMI_CEC_UserDefined_Scan(uint8_t isWorkStatus);
bool IsHDMISourceMute(void);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __HDMI_IN_MODE_H__
