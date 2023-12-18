#ifndef __DISPLAY_TASK_H__
#define __DISPLAY_TASK_H__
#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"

void MsgSendToDisplayTask(uint16_t msgId);
void DisplayServiceCreate(void);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif //__DISPLAY_TASK_H__

