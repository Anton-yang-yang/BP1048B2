/*
 * retarget.c
 *
 *  Created on: Mar 8, 2017
 *      Author: peter
 */

#include <stdio.h>
#include "uarts_interface.h"
#include "type.h"
#include "remap.h"
#ifdef CFG_APP_CONFIG
#include "app_config.h"
#endif
uint8_t DebugPrintPort = UART_PORT0;

#include "uarts.h"
#include "sw_uart.h"

typedef void (*uartfun)(uint8_t);

#ifndef CFG_FUNC_STRING_CONVERT_EN
	#ifndef USE_DBG_CODE
	extern uartfun sendbyte;
	#else
	uartfun sendbyte;
	__attribute__((used))
	void nds_write(const unsigned char *buf, int size)
	{
		int i;
		for (i = 0; i < size; i++)
		{
			sendbyte(buf[i]);
		}
	}
	#endif
#else
	uartfun sendbyte;
	__attribute__((used))
	void nds_write(const unsigned char *buf, int size)
	{
		int i;
		for (i = 0; i < size; i++)
		{
			sendbyte(buf[i]);
		}
	}


#endif


#include "rtos_api.h"
#include "mcu_circular_buf.h"
uint8_t IsSwUartActedAsUARTFlag = 0;

MCU_CIRCULAR_CONTEXT log_CircularBuf;
uint8_t log_buf[4096];
volatile uint8_t uart_switch = 0;
static uint16_t sQueuePrintfLock = 0;
void UartPrintf(uint8_t c)
{
#ifdef CFG_FUNC_DEBUG_EN
#ifdef CFG_USE_SW_UART
	if(IsSwUartActedAsUARTFlag)
    {
        if((unsigned char)c == '\n')
        {
            const char lfca[2] = "\r\n";
            SwUartSend((unsigned char*)lfca, 2);
        }
        else
        {
            SwUartSend((unsigned char*)&c, 1);
        }
    }
	else
#endif
	if(uart_switch==0)
	{
		if(DebugPrintPort == UART_PORT0)
		{
			osMutexLock(UART0Mutex);
			UART0_SendByte(c);
			osMutexUnlock(UART0Mutex);
		}
		else if(DebugPrintPort == UART_PORT1)
		{
			osMutexLock(UART1Mutex);
			UART1_SendByte(c);
			osMutexUnlock(UART1Mutex);
		}
		else
		{
			//uartfun = (uartfun)SwUartSend;
		}
	}
	else
	{
		if(sQueuePrintfLock)return;
		sQueuePrintfLock = 1;
		{
			uint8_t buf[1];
			buf[0] = c;
			if(MCUCircular_GetSpaceLen(&log_CircularBuf)>=1)
			{
				MCUCircular_PutData_Printf(&log_CircularBuf,buf,1);
				
			}
		}
		sQueuePrintfLock = 0;

	}
#endif
}
void QueuePrintUnLock(void)
{
	sQueuePrintfLock = 0;
}
void EnableSwUartAsUART(uint8_t EnableFlag)
{
	IsSwUartActedAsUARTFlag = EnableFlag;
	sendbyte = UartPrintf;
}

void uart_log_out(void)
{
	int i;
#ifdef CFG_FUNC_DEBUG_EN	
	if(IsSwUartActedAsUARTFlag || uart_switch == 0)
		return;
	if(DebugPrintPort == UART_PORT0)
	{
		uint32_t reg_status = *(uint32_t*)0x40005014;
		for(i=0;i<4;i++)
		{
			if(reg_status&0x100)//tx_data_int
			{
				*(uint8_t*)0x4000501C &= ~4;//tx_int_clr
			}
			if(reg_status&0x200)//tx_mcu_allow
			{
				if(MCUCircular_GetDataLen(&log_CircularBuf))
				{
					uint8_t tab[1];
					MCUCircular_GetData_Printf(&log_CircularBuf,tab,1);
					*(uint32_t*)0x40005018 = tab[0];
				}
			}
			if(reg_status&0x100)//tx_data_int
			{
				*(uint8_t*)0x4000501C &= ~4;//tx_int_clr
			}
		}
	}
	else
	{
		uint32_t reg_status = *(uint32_t*)0x40006014;
		for(i=0;i<4;i++)
		{
			if(reg_status&0x100)//tx_data_int
			{
				*(uint8_t*)0x4000601C &= ~4;//tx_int_clr
			}
			if(reg_status&0x200)//tx_mcu_allow
			{
				if(MCUCircular_GetDataLen(&log_CircularBuf))
				{
					uint8_t tab[1];
					MCUCircular_GetData_Printf(&log_CircularBuf,tab,1);
					*(uint32_t*)0x40006018 = tab[0];
				}
			}
			if(reg_status&0x100)//tx_data_int
			{
				*(uint8_t*)0x4000601C &= ~4;//tx_int_clr
			}
		}
	}
#endif	
}

void PrintfAllInBuffer(void)
{
	while(MCUCircular_GetDataLen(&log_CircularBuf))
	{
		uint8_t data;
		MCUCircular_GetData_Printf(&log_CircularBuf,&data,1);
		if(DebugPrintPort == UART_PORT0)
		{
			UART0_SendByte(data);
		}
		else if(DebugPrintPort == UART_PORT1)
		{
			UART1_SendByte(data);
		}
	}

}
int DbgUartInit(int Which, unsigned int BaudRate, unsigned char DatumBits, unsigned char Parity, unsigned char StopBits)
{
#ifdef CFG_FUNC_DEBUG_EN
	IsSwUartActedAsUARTFlag = 0;
	DebugPrintPort = Which;
	if(DebugPrintPort == UART_PORT0)
	{
		sendbyte = UartPrintf;
		UART0_Init(BaudRate, DatumBits,  Parity,  StopBits);
	}
	else if(DebugPrintPort == UART_PORT1)
	{
		sendbyte = UartPrintf;
		UART1_Init(BaudRate, DatumBits,  Parity,  StopBits);
	}
	else
	{
		//uartfun = (uartfun)SwUartSend;
	}
	MCUCircular_Config(&log_CircularBuf,log_buf,sizeof(log_buf));
#else
	sendbyte = UartPrintf;
#endif
	return 0;
}

