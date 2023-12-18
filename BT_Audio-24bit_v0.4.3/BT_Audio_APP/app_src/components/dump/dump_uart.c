/**
 **************************************************************************************
 * @file    main_task.c
 * @brief   Program Entry
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2016-6-29 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include <string.h>
#include "type.h"
#include "app_config.h"
#include "timeout.h"
#include "rtos_api.h"
#include "app_message.h"
#include "debug.h"
#include "clk.h"
#include "main_task.h"
#include "timer.h"
#include "otg_detect.h"
#include "ctrlvars.h"
#include "communication.h"
#include "otg_device_hcd.h"
#include "otg_device_standard_request.h"
#include "ff.h"
#include "mvstdio.h"
#include "blue_aec.h"
#include "otg_host_hcd.h"
#include "dma.h"
#include "rom.h"
#include "uarts.h"
#include "uarts_interface.h"

#ifdef CFG_DUMP_DEBUG_EN

#define ONE_BLOCK_WRITE 512

MemHandle aec_debug_fifo;
static uint8_t aec_debug_raw_buf[4096*2];
uint8_t aec_temp_buf[ONE_BLOCK_WRITE];
int16_t aec_temp_buf1[256*2];
uint8_t hfp_disconnect = 0;

uint8_t gDumpProcessStop = 0;//dump stop flag

static const uint8_t DmaChannelMap[29] = {
	255,//PERIPHERAL_ID_SPIS_RX = 0,	//0
	255,//PERIPHERAL_ID_SPIS_TX,		//1
	255,//PERIPHERAL_ID_TIMER3,			//2
	4,//PERIPHERAL_ID_SDIO_RX,			//3
	4,//PERIPHERAL_ID_SDIO_TX,			//4
	255,//PERIPHERAL_ID_UART0_RX,		//5
	255,//PERIPHERAL_ID_TIMER1,			//6
	255,//PERIPHERAL_ID_TIMER2,			//7
	255,//PERIPHERAL_ID_SDPIF_RX,		//8 SPDIF_RX /TX same chanell
	255,//PERIPHERAL_ID_SDPIF_TX,		//8 SPDIF_RX /TX same chanell
	255,//PERIPHERAL_ID_SPIM_RX,		//9
	255,//PERIPHERAL_ID_SPIM_TX,		//10

#if (defined(CFG_DUMP_DEBUG_EN)&&(CFG_DUMP_UART_TX_PORT_GROUP == 0))
	7,//PERIPHERAL_ID_UART0_TX, 	//11
#else
	255,//PERIPHERAL_ID_UART0_TX,		//11
#endif
	
	255,//PERIPHERAL_ID_UART1_RX,		//12
	
#if (defined(CFG_DUMP_DEBUG_EN)&&(CFG_DUMP_UART_TX_PORT_GROUP == 1))
	7,//PERIPHERAL_ID_UART1_TX,			//13
#else
	255,//PERIPHERAL_ID_UART1_TX,		//13
#endif

	255,//PERIPHERAL_ID_TIMER4,			//14
	255,//PERIPHERAL_ID_TIMER5,			//15
	255,//PERIPHERAL_ID_TIMER6,			//16
	0,//PERIPHERAL_ID_AUDIO_ADC0_RX,	//17
	1,//PERIPHERAL_ID_AUDIO_ADC1_RX,	//18
	2,//PERIPHERAL_ID_AUDIO_DAC0_TX,	//19
	3,//PERIPHERAL_ID_AUDIO_DAC1_TX,	//20
	255,//PERIPHERAL_ID_I2S0_RX,		//21
#if	(defined(CFG_RES_AUDIO_I2SOUT_EN )&&(CFG_RES_I2S == 0))
	7,//PERIPHERAL_ID_I2S0_TX,			//22
#else
	255,//PERIPHERAL_ID_I2S0_TX,		//22
#endif
	255,//PERIPHERAL_ID_I2S1_RX,		//23
#if	(defined(CFG_RES_AUDIO_I2SOUT_EN )&&(CFG_RES_I2S == 1))
	7,	//PERIPHERAL_ID_I2S1_TX,		//24
#else
	255,//PERIPHERAL_ID_I2S1_TX,		//24
#endif
	255,//PERIPHERAL_ID_PPWM,			//25
	255,//PERIPHERAL_ID_ADC,     		//26
	255,//PERIPHERAL_ID_SOFTWARE,		//27
};

void sbc_store_process(uint8_t *sbc, uint16_t Len)
{
	int i;
	static uint8_t cnn = 0;
	DBG(".");
	if(hfp_disconnect != 2)
		return;
	for(i=0; i<Len; i++)
	{
		aec_temp_buf1[i] = sbc[i];
	}
	if(mv_mremain(&aec_debug_fifo) >= Len)
	{
		mv_mwrite(aec_temp_buf1, 1, Len, &aec_debug_fifo);
	}
	else
	{
		if(cnn == 0)
		{
			APP_DBG("\n-F-\n");
			cnn = 100;
		}
		else
		{
			cnn--;
		}

	}
}
void sco_store_process(uint8_t *sco, uint16_t Len)
{
	int i;
	static uint8_t cnn = 0;
	DBG(".");
	if(hfp_disconnect != 2)
		return;
	for(i=0; i<Len; i++)
	{
		aec_temp_buf1[i] = sco[i];
	}
	if(mv_mremain(&aec_debug_fifo) >= Len)
	{
		mv_mwrite(aec_temp_buf1, 1, Len, &aec_debug_fifo);
	}
	else
	{
		if(cnn == 0)
		{
			APP_DBG("\n-F-\n");
			cnn = 100;
		}
		else
		{
			cnn--;
		}

	}
}
void aec_store_process(uint8_t *aec, uint16_t Len)
{
	int i;
	static uint8_t cnn = 0;
	// DBG(".");
	if(hfp_disconnect != 2)
		return;
	if(mv_mremain(&aec_debug_fifo) >= Len)
	{
		mv_mwrite(aec, 1, Len, &aec_debug_fifo);
	}
	else
	{
		if(cnn == 0)
		{
			APP_DBG("\n-F-\n");
			cnn = 100;
		}
		else
		{
			cnn--;
		}

	}
}

uint8_t DumpBuf[4096];
void DumpUartConfig(bool InitBandRate)
{
	switch (CFG_DUMP_UART_TX_PORT)
	{
	case 0:
		GPIO_PortAModeSet(GPIOA6, 5);//Tx,A6:uart0_txd_0
		if(InitBandRate)
		{
			UARTS_Init(0, CFG_DUMP_UART_BANDRATE, 8, 0, 1);
		}
		DMA_ChannelAllocTableSet(DmaChannelMap);//载入DmaChannelMap——目的是设置UART RX和UART TX分别使用哪一路DMA
		UARTS_DMA_TxInit(0,DumpBuf,4096,4096/2,NULL);
		break;
		
	case 1:
		GPIO_PortAModeSet(GPIOA10, 3);//Tx,A10:uart1_txd_0
		if(InitBandRate)
		{
			UARTS_Init(1, CFG_DUMP_UART_BANDRATE, 8, 0, 1);
		}
		DMA_ChannelAllocTableSet(DmaChannelMap);//载入DmaChannelMap——目的是设置UART RX和UART TX分别使用哪一路DMA
		UARTS_DMA_TxInit(1,DumpBuf,4096,4096/2,NULL);
		break;
		
	case 2:
		GPIO_PortAModeSet(GPIOA25, 6);	//Tx, A25:uart1_txd_0
		if(InitBandRate)
		{
			UARTS_Init(1, CFG_DUMP_UART_BANDRATE, 8, 0, 1);
		}
		DMA_ChannelAllocTableSet(DmaChannelMap);//载入DmaChannelMap——目的是设置UART RX和UART TX分别使用哪一路DMA
		UARTS_DMA_TxInit(1,DumpBuf,4096,4096/2,NULL);
		break;
		
	case 3:
		GPIO_PortAModeSet(GPIOA0, 8);	//Tx, A0:1000:uart0_txd_2(o)
		if(InitBandRate)
		{
			UARTS_Init(0, CFG_DUMP_UART_BANDRATE, 8, 0, 1);
		}
		DMA_ChannelAllocTableSet(DmaChannelMap);//载入DmaChannelMap——目的是设置UART RX和UART TX分别使用哪一路DMA
		UARTS_DMA_TxInit(0,DumpBuf,4096,4096/2,NULL);
		break;
		
	case 4:
		GPIO_PortAModeSet(GPIOA1, 0x0c);	//Tx, A1:1100:uart0_txd_0(o)
		if(InitBandRate)
		{
			UARTS_Init(0, CFG_DUMP_UART_BANDRATE, 8, 0, 1);
		}
		DMA_ChannelAllocTableSet(DmaChannelMap);//载入DmaChannelMap——目的是设置UART RX和UART TX分别使用哪一路DMA
		UARTS_DMA_TxInit(0,DumpBuf,4096,4096/2,NULL);
		break;
		
	case 5:
		GPIO_PortAModeSet(GPIOA19, 3);//Tx,A19:uart1_txd_1
		if(InitBandRate)
		{
			UARTS_Init(1, CFG_DUMP_UART_BANDRATE, 8, 0, 1);
		}
		DMA_ChannelAllocTableSet(DmaChannelMap);//载入DmaChannelMap——目的是设置UART RX和UART TX分别使用哪一路DMA
		UARTS_DMA_TxInit(1,DumpBuf,4096,4096/2,NULL);
		break;
		
	default:
		break;
	}
}

void dumpUartSend(uint8_t *buf,uint16_t buflen)
{
	uint8_t Hend[6];
	uint16_t Crc;
	static uint8_t frameid;
	TIMER DumpTimer;

	//dump 异常，停止发送
	if(gDumpProcessStop)
		return;
	
	TimeOutSet(&DumpTimer, 1000);
#if (CFG_DUMP_UART_TX_PORT_GROUP == 1)
	while(DMA_CircularSpaceLenGet(PERIPHERAL_ID_UART1_TX) < buflen+8){
		if(IsTimeOut(&DumpTimer))
		{
			uint32_t id;
			id = DMA_ChannelNumGet(PERIPHERAL_ID_UART1_TX);
			APP_DBG("DUMP_ERR PERIPHERAL_ID_UART1_TX ID %ld\r\n",id);    //注意查看DMA通道是否在其他位置被重新设置了。DMA_ChannelAllocTableSet
			return;
		}
	}
	Hend[0] = 0xA5;
	Hend[1] = 0xA5;
	Hend[2] = 0xA5;
	Hend[3] = frameid++;
	Hend[4] = buflen>>8;
	Hend[5] = buflen;
	Crc = ROM_CRC16(buf,buflen,0);
//	printf("Crc 0x%x\r\n",Crc);
	DMA_CircularDataPut(PERIPHERAL_ID_UART1_TX, Hend, 6);
	DMA_CircularDataPut(PERIPHERAL_ID_UART1_TX, buf, buflen);
	DMA_CircularDataPut(PERIPHERAL_ID_UART1_TX, &Crc, 2);
#else
	while(DMA_CircularSpaceLenGet(PERIPHERAL_ID_UART0_TX) < buflen+8){
		if(IsTimeOut(&DumpTimer))
		{
			uint32_t id;
			id = DMA_ChannelNumGet(PERIPHERAL_ID_UART0_TX);
			APP_DBG("DUMP_ERR PERIPHERAL_ID_UART0_TX ID %ld\r\n",id);    //注意查看DMA通道是否在其他位置被重新设置了。DMA_ChannelAllocTableSet
			return;
		}
	}
	Hend[0] = 0xA5;
	Hend[1] = 0xA5;
	Hend[2] = 0xA5;
	Hend[3] = frameid++;
	Hend[4] = buflen>>8;
	Hend[5] = buflen;
	Crc = ROM_CRC16(buf,buflen,0);
//	printf("Crc 0x%x\r\n",Crc);
	DMA_CircularDataPut(PERIPHERAL_ID_UART0_TX, Hend, 6);
	DMA_CircularDataPut(PERIPHERAL_ID_UART0_TX, buf, buflen);
	DMA_CircularDataPut(PERIPHERAL_ID_UART0_TX, &Crc, 2);
#endif
}

void aec_data_dump_task(void * param)
{
	mv_mopen(&aec_debug_fifo, aec_debug_raw_buf, sizeof(aec_debug_raw_buf)-4, NULL);
	hfp_disconnect = 2;
	while(1)
	{
		if(hfp_disconnect == 2)
		{
			if(mv_msize(&aec_debug_fifo) >= ONE_BLOCK_WRITE)
			{
				// DBG("*");
				mv_mread(aec_temp_buf, 1, ONE_BLOCK_WRITE, &aec_debug_fifo);
				dumpUartSend(aec_temp_buf,ONE_BLOCK_WRITE);
			}
		}
		vTaskDelay(1);
	}
}

#endif //end of CFG_DUMP_DEBUG_EN



