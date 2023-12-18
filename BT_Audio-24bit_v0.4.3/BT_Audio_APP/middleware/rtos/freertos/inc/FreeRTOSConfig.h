/*
    FreeRTOS V8.0.0 - Copyright (C) 2014 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that has become a de facto standard.             *
     *                                                                       *
     *    Help yourself get started quickly and support the FreeRTOS         *
     *    project by purchasing a FreeRTOS tutorial book, reference          *
     *    manual, or both from: http://www.FreeRTOS.org/Documentation        *
     *                                                                       *
     *    Thank you!                                                         *
     *                                                                       *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    >>! NOTE: The modification to the GPL is included to allow you to distribute
    >>! a combined work that includes FreeRTOS without being obliged to provide
    >>! the source code for proprietary components outside of the FreeRTOS
    >>! kernel.

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available from the following
    link: http://www.freertos.org/a00114.html

    1 tab == 4 spaces!

    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?"                                     *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org - Documentation, books, training, latest versions,
    license and Real Time Engineers Ltd. contact details.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
    Integrity Systems to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/**
* @addtogroup FreeRTOS
* @{
* @defgroup freertosconfig freertosconfig.h
* @{
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

/* Ensure stdint is only used by the compiler, and not the assembler. */
//#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
// #include <stdint.h>
 //extern uint32_t SystemCoreClock;
//#endif

#define configUSE_PREEMPTION			1
#define configUSE_IDLE_HOOK			1
#define configUSE_TICK_HOOK			0
#define configCPU_CLOCK_HZ			( ( unsigned long ) 120000000)
#define configTICK_RATE_HZ			( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES			(  10 )
#define configMINIMAL_STACK_SIZE		( ( uint16_t ) 1024 )//( ( uint16_t ) 512 )
#define configTOTAL_HEAP_SIZE			( ( size_t ) ( 37 * 1024 ) )
#define configMAX_TASK_NAME_LEN			( 16 )
#define configUSE_TRACE_FACILITY		1
#define configUSE_16_BIT_TICKS			0
#define configIDLE_SHOULD_YIELD			1
#define configUSE_MUTEXES			1
#define configQUEUE_REGISTRY_SIZE		8
#define configCHECK_FOR_STACK_OVERFLOW	        0
#define configUSE_RECURSIVE_MUTEXES		1
#define configUSE_MALLOC_FAILED_HOOK	        0
#define configUSE_APPLICATION_TASK_TAG	        0
#define configUSE_COUNTING_SEMAPHORES	        1
#define configGENERATE_RUN_TIME_STATS	        1
#define configSUPPORT_ZOL                       1

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 		        0
#define configMAX_CO_ROUTINE_PRIORITIES        ( 2 )

/* Software timer definitions. */
#define configUSE_TIMERS			0
#define configTIMER_TASK_PRIORITY		( 2 )
#define configTIMER_QUEUE_LENGTH		10
#define configTIMER_TASK_STACK_DEPTH	        ( configMINIMAL_STACK_SIZE * 2 )

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */
#define INCLUDE_vTaskPrioritySet		1
#define INCLUDE_uxTaskPriorityGet		1
#define INCLUDE_vTaskDelete			1
#define INCLUDE_vTaskCleanUpResources	        0
#define INCLUDE_vTaskSuspend			0
#define INCLUDE_vTaskDelayUntil			1
#define INCLUDE_vTaskDelay			1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_pcTaskGetTaskName 1

#define configPRIO_BITS       		2        /* 4 priority levels */


/* The lowest interrupt priority that can be used in a call to a "set priority"
function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY			0xf

/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
PRIORITY THAN THIS! (higher priorities are lower numeric values. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY	5

/* Interrupt priorities used by the kernel port layer itself.  These are generic
to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY 		( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
//#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

//0：任务切换时关闭所有中断   1：任务切换时关闭中断优先级为1、2、3的中断  2：任务切换时关闭中断优先级为2、3的中断  3：任务切换时关闭中断优先级为3的中断
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	0

/* Normal assert() semantics without relying on the provision of an assert.h
header file. */
#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names. */
//#define vPortSVCHandler SVC_Handler
//#define xPortPendSVHandler PendSV_Handler

/* IMPORTANT: This define MUST be commented when used with STM32Cube firmware,
              to prevent overwriting SysTick_Handler defined within STM32Cube HAL */
/* #define xPortSysTickHandler SysTick_Handler */

#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()
#define portGET_RUN_TIME_COUNTER_VALUE()	xTickCount

#define INCLUDE_xSemaphoreGetMutexHolder 1

#define traceTASK_DELETE DeleteTaskMallocMem

#define use_MCPS_ANALYSIS
#ifdef use_MCPS_ANALYSIS
#define traceTASK_SWITCHED_IN()  trace_TASK_SWITCHED_IN()
#define traceTASK_SWITCHED_OUT() trace_TASK_SWITCHED_OUT()
#endif

#define configHSP_ENABLE 1
#if (configHSP_ENABLE==1)
/*Set the bytes of stack's offset in TCB. Unit: 1 byte*/
#define StackOffset_TCB			48 /*The offset of pxCurrentTCB->pxStack in TCB structure*/

/*Set the offset of top address of stack in TCB. Unit: 1 byte*/
#define EndStackOffset_TCB		52 + configMAX_TASK_NAME_LEN /*The offset of pxCurrentTCB->pxEndOfStack in TCB structure*/
#endif

#endif /* FREERTOS_CONFIG_H */


#if 0
/*
    FreeRTOS V8.0.0 - Copyright (C) 2014 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that has become a de facto standard.             *
     *                                                                       *
     *    Help yourself get started quickly and support the FreeRTOS         *
     *    project by purchasing a FreeRTOS tutorial book, reference          *
     *    manual, or both from: http://www.FreeRTOS.org/Documentation        *
     *                                                                       *
     *    Thank you!                                                         *
     *                                                                       *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    >>! NOTE: The modification to the GPL is included to allow you to distribute
    >>! a combined work that includes FreeRTOS without being obliged to provide
    >>! the source code for proprietary components outside of the FreeRTOS
    >>! kernel.

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available from the following
    link: http://www.freertos.org/a00114.html

    1 tab == 4 spaces!

    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?"                                     *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org - Documentation, books, training, latest versions,
    license and Real Time Engineers Ltd. contact details.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
    Integrity Systems to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE. 
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

#ifdef CONFIG_PLAT_AE210P

	#define configUSE_PREEMPTION				1
	#define configUSE_IDLE_HOOK				0
	#define configUSE_TICK_HOOK				0
	#define configCPU_CLOCK_HZ				( ( unsigned long ) 47923200 )
	#define configTICK_RATE_HZ				( ( portTickType ) 100 )
	#define configMAX_PRIORITIES				20 
	#define configMINIMAL_STACK_SIZE			( ( unsigned short ) 4*1024 )
	
		#ifdef CONFIG_SMALL_HEAP
		/* This small heap is for WAVBMP demo in 16MB */
			#define configTOTAL_HEAP_SIZE				( ( size_t ) 512*1024 )
		#else
			#define configTOTAL_HEAP_SIZE				( ( size_t ) 32*1024 )
		#endif

	#define configMAX_TASK_NAME_LEN				( 16 )
	#define configUSE_TRACE_FACILITY			1
	#define configUSE_16_BIT_TICKS				0
	#define configIDLE_SHOULD_YIELD				1
	
	/* Co-routine definitions. */
	#define configUSE_CO_ROUTINES 				0
	#define configMAX_CO_ROUTINE_PRIORITIES 		( 2 )
	
	/* Set the following definitions to 1 to include the API function, or zero
	to exclude the API function. */
	
	#define INCLUDE_vTaskPrioritySet			1
	#define INCLUDE_uxTaskPriorityGet			1
	#define INCLUDE_vTaskDelete				1
	#define INCLUDE_vTaskCleanUpResources			0
	#define INCLUDE_vTaskSuspend				1
	#define INCLUDE_vTaskDelayUntil				1
	#define INCLUDE_vTaskDelay				1
	#define INCLUDE_xTaskGetCurrentTaskHandle		1
	
	/* IPC Semaphore */
	#define configUSE_COUNTING_SEMAPHORES			1
	
	/* For debug */
	#define configCHECK_FOR_STACK_OVERFLOW			0
	#define configUSE_MALLOC_FAILED_HOOK			0
	
	/* For OS aware debugging */
	#define configQUEUE_REGISTRY_SIZE			10
	

#else


#define configUSE_PREEMPTION				1
#define configUSE_IDLE_HOOK				0
#define configUSE_TICK_HOOK				0
#define configCPU_CLOCK_HZ				( ( unsigned long ) 47923200 )
#define configTICK_RATE_HZ				( ( portTickType ) 100 )
//#define configMAX_PRIORITIES				( ( unsigned portBASE_TYPE ) 20 )
#define configMAX_PRIORITIES				20

#ifdef CONFIG_OSC_SUPPORT
#define configMINIMAL_STACK_SIZE                        ( ( unsigned short ) 256 )
#else
#define configMINIMAL_STACK_SIZE                        ( ( unsigned short ) 512 )
#endif  // #ifdef CONFIG_OSC_SUPPORT
#ifdef CONFIG_LATENCY_HEAP
#define configTOTAL_HEAP_SIZE                           ( ( size_t ) 28*1024 )
#else
#ifdef CONFIG_PLAT_AG101P_16MB
#ifdef CONFIG_SMALL_HEAP
/* This small heap is for WAVBMP demo in 16MB */
#define configTOTAL_HEAP_SIZE				( ( size_t ) 512*1024 )
#else
#ifdef CONFIG_OSC_SUPPORT
#define configTOTAL_HEAP_SIZE                           ( ( size_t ) 8*1024 )
#else
#define configTOTAL_HEAP_SIZE                           ( ( size_t ) 4*1024*1024 )
#endif  // #ifdef CONFIG_OSC_SUPPORT
#endif
#else
#ifdef CONFIG_SMALL_HEAP
/* This small heap is for NDS_AUDIO demo */
#define configTOTAL_HEAP_SIZE				( ( size_t ) 512*1024 )
#else
#define configTOTAL_HEAP_SIZE				( ( size_t ) 40*1024*1024 )
#endif
#endif
#endif
#define configMAX_TASK_NAME_LEN				( 16 )
#define configUSE_TRACE_FACILITY			1
#define configUSE_16_BIT_TICKS				0
#define configIDLE_SHOULD_YIELD				1

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 				0
#define configMAX_CO_ROUTINE_PRIORITIES 		( 2 )


/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet			1
#define INCLUDE_uxTaskPriorityGet			1
#define INCLUDE_vTaskDelete				1
#define INCLUDE_vTaskCleanUpResources			0
#define INCLUDE_vTaskSuspend				1
#define INCLUDE_vTaskDelayUntil				1
#define INCLUDE_vTaskDelay				1
#define INCLUDE_xTaskGetCurrentTaskHandle		1

/* IPC Semaphore */
#define configUSE_COUNTING_SEMAPHORES			1

/* For debug */
#define configCHECK_FOR_STACK_OVERFLOW			0
#define configUSE_MALLOC_FAILED_HOOK			0

/* For OS aware debugging */
#define configQUEUE_REGISTRY_SIZE			10
#endif /* AE210P or AG101P */

/* For EventGroups */
#define configUSE_TIMERS				0	/* EventGroup need SW Timer daemon Task */
#define configTIMER_TASK_PRIORITY			2	/* Daemon task priority */
#define configTIMER_QUEUE_LENGTH			5	/* Command Queue of daemon task */
#define configTIMER_TASK_STACK_DEPTH			256	/* Daemon stack size */
#define INCLUDE_eTaskGetState                           1	/* EventGroup demo program need */
#define INCLUDE_xTimerPendFunctionCall                  1	/* EventGroup "FromISR" API need */


/* Arch Optimization Option */
//#define configUSE_PORT_OPTIMISED_TASK_SELECTION		1

/* Tickless idle/low power functionality support. */
//#define configUSE_TICKLESS_IDLE				1

#endif /* FREERTOS_CONFIG_H */
#endif
