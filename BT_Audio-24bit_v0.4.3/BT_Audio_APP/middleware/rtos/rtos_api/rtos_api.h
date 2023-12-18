/*
 * RTOS_API.c
 *
 *  Created on: Aug 30, 2016
 *      Author: peter
 */
 
#ifndef __RTOS_API_H__
#define __RTOS_API_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "list.h"
//#include "locks.h"
typedef SemaphoreHandle_t osMutexId;

#define MessageHandle		QueueHandle_t
#define MSG_INVAILD			0x0000

typedef struct _MessageContext
{
	uint16_t		msgId;
}MessageContext;


/**
* @brief	Register a message queue
* @param	maxMsgQueueNumber
*	The max number of message that the queue can receive.
* @retval	MessageHandle
*/
MessageHandle MessageRegister(uint16_t maxMsgQueueNumber);
#define MessageRegister(num) 			xQueueCreate((num), sizeof(MessageContext))

/**
* @brief	Deregister a message queue
* @param	msgHandle
* @retval	NONE
*/
void MessageDeregister(MessageHandle msgHandle);
#define MessageDeregister(handle)		vQueueDelete((handle))

/**
* @brief	Clear all of message in the queue
* @param	msgHandle
*	The message queue which to be cleared
* @retval	NONE
*/
void MessageClear(MessageHandle msgHandle);
#define MessageClear(handle)			xQueueReset((handle))


/**
* @brief	Send Message
* @param	msgHandle The message queue which will receive this message
* @param	msgContext The message to send
* @retval	TRUE or FALSE
*/
bool MessageSend(MessageHandle msgHandle,  MessageContext * msgContext);
bool MessageSendFromISR(MessageHandle msgHandle,  MessageContext * msgContext);


/**
* @brief
*	Send Message extension function. 
*	It will not return until the message has been sent successful.
*	This function CAN NOT be used in interrupt functions
* @param 	msgHandle The message queue which will receive this message
* @param	msgContext The message to send
* @retval	NONE
*/
void MessageSendx(MessageHandle msgHandle,  MessageContext * msgContext);

/**
* @brief	Get a Message or Wait for a Message from a Queue.
* @param 	msgHandle The message queue handle
* @param 	millisec Timeout value or 0 in case of no time-out.
* @param	msgContext The message to send
* @retval	TRUE means received message and FALSE means timeout
*/
bool MessageRecv(MessageHandle msgHandle, MessageContext * msgContext, uint32_t millisec);

/**
* @brief	Init heap.
* @param 	NONE
* @retval	NONE
*/
void prvInitialiseHeap(void);

void osSemaphoreMutexCreate(void);

void *osPortMallocFromEnd_1(uint32_t osWantedSize);
void *osPortMalloc_1(uint16_t osWantedSize);

//#define OS_MEM_DEBUG
#ifdef OS_MEM_DEBUG
#define osPortMallocFromEnd(A) 		osPortMallocFromEnd_1(A);printf("%s %s %u\n",__FILE__,__FUNCTION__,__LINE__)
#define osPortMalloc(A) 				osPortMalloc_1(A);printf("%s %s %u\n",__FILE__,__FUNCTION__,__LINE__)
#else
#define osPortMallocFromEnd 		osPortMallocFromEnd_1
#define osPortMalloc 				osPortMalloc_1
#endif

void osPortFree( void *ospv );
void osTaskDelay(uint32_t Cnt);
int osPortRemainMem(void);
BaseType_t osMutexLockFun(osMutexId mutex_id);
BaseType_t osMutexLock_1000msFun(osMutexId mutex_id);
BaseType_t osMutexLock_1msFun(osMutexId mutex_id);
BaseType_t osMutexUnlockFun(osMutexId mutex_id);


#define osWaitForever     			0xFFFFFFFF     ///< wait forever timeout value
#define osMutexCreate()				xSemaphoreCreateMutex()

#define osMutexLock( mutex_id )		osMutexLockFun(mutex_id)//xSemaphoreTake(( mutex_id ), ( osWaitForever ))
#define osMutexLock_1000ms( mutex_id ) osMutexLock_1000msFun(mutex_id)		//xSemaphoreTake(( mutex_id ), ( 1000 ))
#define osMutexLock_1ms( mutex_id )		osMutexLock_1msFun(mutex_id)//xSemaphoreTake(( mutex_id ), ( 1 ))
#define osMutexUnlock( mutex_id )	osMutexUnlockFun(mutex_id)//xSemaphoreGive( mutex_id )


//
extern osMutexId SDIOMutex;//defined @device_service
extern osMutexId SDIOSendCommandMutex;
extern osMutexId UDiskMutex;
extern osMutexId MuteReadBlock;
extern osMutexId MuteWriteBlock;
extern osMutexId UART0Mutex;
extern osMutexId FlashMutex;
extern osMutexId UART1Mutex;
extern osMutexId AudioEffectMutex;
extern osMutexId LoadAudioParamMutex;
extern osMutexId SbcDecoderMutex;
extern osMutexId SysModeMutex;
extern osMutexId RemindMutex;

void osSemaphoreMutexUnlock(void);

/**
* @brief   Read instantaneous MCPS situation, instantaneous MCPS will be cleared to 0 after read
* @param   NONE
* @retval  (mS) Max duration of full load
*/
uint16_t InstantMcpsFull(void);

#define ENABLE_COUNT_INSTANT_MCPS

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif /*__RTOS_API_H__*/
