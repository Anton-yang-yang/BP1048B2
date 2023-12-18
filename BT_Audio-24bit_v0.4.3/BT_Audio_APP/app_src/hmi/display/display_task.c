#include <string.h>
#include "type.h"
#include "app_config.h"
#include "rtos_api.h"
#include "app_message.h"
#include "debug.h"
#include "display_task.h"
#ifdef CFG_FUNC_DISPLAY_EN
#include "display.h"
#endif

#ifdef CFG_FUNC_DISPLAY_TASK_EN

static MessageHandle DisplayTaskMsgHandle = NULL;

#define DISPLAY_TASK_STACK_SIZE 		512
#define DISPLAY_TASK_PRIO 				3
#define DISPLAY_MESSAGE_QUEUE_NUM 		20


static void DisplayTaskEntrance(void * param)
{
	MessageContext		msg;

	DisplayTaskMsgHandle = MessageRegister(DISPLAY_MESSAGE_QUEUE_NUM);

#ifdef CFG_FUNC_DISPLAY_EN
	DispInit(0);
#endif

	while(1)
	{
		MessageRecv(DisplayTaskMsgHandle, &msg, 100);

#ifdef CFG_FUNC_DISPLAY_EN
		//display
		Display(msg.msgId);
#endif

	}
}

void DisplayServiceCreate(void)
{	
	if(xTaskCreate(DisplayTaskEntrance,
			"DisplayTask",
			DISPLAY_TASK_STACK_SIZE,
			NULL, DISPLAY_TASK_PRIO, NULL) != pdTRUE)
	{
		APP_DBG("Display Task create fail!\n");
	}
}

void MsgSendToDisplayTask(uint16_t msgId)
{
	MessageContext msgSend;

	msgSend.msgId = msgId;
	MessageSend(DisplayTaskMsgHandle, &msgSend);
}

#endif
	

