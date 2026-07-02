#ifndef __SYS_EVENT_H__
#define __SYS_EVENT_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include <common/generic.h>
#include "sys_event_type.h"




/**
 * @brief 注册系统事件（消息队列）
 * 
 * @param name 名称
 * @param user 用户指针 
 * 
 * @return QueueHandle_t 
 */
QueueHandle_t sysEventRegister(sysEventNode_t nodeId, const char* name, void *user);


int sysEventPost(sysEventNode_t nodeId, int event, const void *data, int size);

int sysEventPop(sysEventNode_t nodeId, void **user, int *event, void *data, int *size);
#endif