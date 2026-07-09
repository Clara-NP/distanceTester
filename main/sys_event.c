#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"


#include <common/generic.h>
#include "sys_event.h"

#define TRACE_TAG "sys-event"
#define TRACE_LEVEL T_INFO
#define TRACE_ENABLE

#include <common/trace.h>

/**
 * @brief 推送事件默认等待时间，ms
 * 
 */
#ifndef CONFIG_SYS_EVENT_DEFAULT_WAIT_TIME
#define CONFIG_SYS_EVENT_DEFAULT_WAIT_TIME    20
#endif

/**
 * @brief 事件队列深度
 * 
 */
#ifndef CONFIG_SYS_EVENT_ITEM_LENGTH
#define CONFIG_SYS_EVENT_ITEM_LENGTH       32
#endif

/**
 * @brief MQTT发布队列格式
 * 
 */
typedef struct
{
    int event;
    int dataSize;
    void *data;
}sysEventQueue_t;

typedef struct
{
    /// @brief 已注册数量
    uint8_t registerNumber;
    /// @brief 名称
    const char* name[SYS_EVENT_NODE_MAX];
    /// @brief 用户指针
    void *user[SYS_EVENT_NODE_MAX];
    /// @brief 参数锁
    SemaphoreHandle_t mutex[SYS_EVENT_NODE_MAX];
    /// @brief 发布队列
    QueueHandle_t queue[SYS_EVENT_NODE_MAX];
}sysEventManage_t;

static sysEventManage_t s_sysEventManage = {0};
#define getInstance()       &s_sysEventManage

/**
 * @brief 注册电源事件（消息队列）
 * 
 * @param name 名称
 * @param user 用户指针 
 * 
 * @return QueueHandle_t 
 */
QueueHandle_t sysEventRegister(sysEventNode_t nodeId, const char* name, void *user)
{
    sysEventManage_t *m = getInstance();
    // QueueHandle_t queue;
    
    if (m->registerNumber >= SYS_EVENT_NODE_MAX) {
        return NULL;
    }

    if (m->name[nodeId] != NULL) {
        return m->queue[nodeId];
    }

    m->name[nodeId] = name ? name : "Unknow module";
    m->user[nodeId] = user;
    m->mutex[nodeId] = xSemaphoreCreateMutex();
    ASSERT((m->mutex[nodeId] != NULL), "%s create event mutex failed", m->name[nodeId]);

    m->queue[nodeId] = xQueueCreate(CONFIG_SYS_EVENT_ITEM_LENGTH, sizeof(sysEventQueue_t));
    ASSERT((m->queue[nodeId] != NULL), "%s create event queue failed", m->name[nodeId]);

    m->registerNumber++;
    return m->queue[nodeId];
}

int sysEventPost(sysEventNode_t nodeId, int event, const void *data, int size)
{
    sysEventManage_t *m = getInstance();
    QueueHandle_t queue = NULL;
    UBaseType_t queueLength;
    sysEventQueue_t eventQueue;
    int ret;

    if(!m->registerNumber || !m->queue[nodeId]|| nodeId >= SYS_EVENT_NODE_MAX) {
        wlog("Event(module:%s) register number:%d queue:%p nodeId:%d", m->name[nodeId], m->registerNumber, m->queue[nodeId], nodeId);
        return RET_NO_OBJECT;
    }

    if((size < 0 ) || (size && !data)) {
        wlog("Event(module:%s) size:%d data:%p", m->name[nodeId], size, data);
        return RET_INVALID_PARAMETER;
    }

    queue = m->queue[nodeId];
    queueLength = uxQueueMessagesWaiting(queue);
    if (queueLength >= CONFIG_SYS_EVENT_ITEM_LENGTH) {
        wlog("Event(module:%s) queue is already full, length:%ld max:%d", m->name[nodeId], queueLength, CONFIG_SYS_EVENT_ITEM_LENGTH);
        return RET_FAILED;
    }

    eventQueue.event = event;
    eventQueue.dataSize = size;
    eventQueue.data = NULL;
    if (size) {
        eventQueue.data = osMalloc(size);
        if (!eventQueue.data) {
            wlog("Event(module:%s) malloc failed", m->name[nodeId]);
            return RET_NO_MEM;
        }
        osMemcpy(eventQueue.data, data, size);
    }

    if (xSemaphoreTake(m->mutex[nodeId], CONFIG_SYS_EVENT_DEFAULT_WAIT_TIME / portTICK_PERIOD_MS) == pdFALSE) {
        wlog("Event(module:%s) mutex take failed", m->name[nodeId]);
        if (eventQueue.data) {
            osFree(eventQueue.data);
            wlog("Event(module:%s) mutex give failed", m->name[nodeId]);
        }
        return RET_FAILED;
    }

    ret = xQueueSend(queue, &eventQueue, 0);
    xSemaphoreGive(m->mutex[nodeId]);
    if (ret != pdTRUE) {
        wlog("Event(module:%s) queue send failed", m->name[nodeId]);
        if (eventQueue.data) {
            osFree(eventQueue.data);
        }
        return RET_FAILED;
    }

    // ilog("Event Post, module:%s event:%d dataSize:%d", m->name[nodeId], event, size);

    return RET_SUCCESS;
}

int sysEventPop(sysEventNode_t nodeId, void **user, int *event, void *data, int *size)
{
    sysEventManage_t *m = getInstance();
    QueueHandle_t queue = NULL;
    sysEventQueue_t eventQueue;
    UBaseType_t queueLength;
    int ret;

    if(!m->registerNumber || !m->queue[nodeId] || nodeId >= SYS_EVENT_NODE_MAX) {
        return RET_NO_OBJECT;
    }
    
    queue = m->queue[nodeId];
    queueLength = uxQueueMessagesWaiting(queue);
    if (queueLength == 0) {
        return RET_NO_DATA;
    }

    if (xSemaphoreTake(m->mutex[nodeId], CONFIG_SYS_EVENT_DEFAULT_WAIT_TIME / portTICK_PERIOD_MS) == pdFALSE) {
        wlog("Event(module:%s) mutex take failed", m->name[nodeId]);
        return RET_FAILED;
    }


    ret = xQueueReceive(queue, &eventQueue, 0);
    xSemaphoreGive(m->mutex[nodeId]);
    if (ret != pdTRUE) {
        wlog("Event(module:%s) queue receive failed", m->name[nodeId]);
        return RET_FAILED;
    }

    if(user) {
        *user = m->user[nodeId];
    }

    if(event) {
        *event = eventQueue.event;
    }

    if(size) {
        *size = eventQueue.dataSize;
    }

    if(data && eventQueue.data) {
        osMemcpy(data, eventQueue.data, eventQueue.dataSize);
    }

    //务必释放内存
    if(eventQueue.data) {
        osFree(eventQueue.data);
    }

    dlog("Event Pop, module:%s event:%d dataSize:%d", m->name[nodeId], eventQueue.event, eventQueue.dataSize);
    return RET_SUCCESS;
}

void eventQueueReceive(sysEventNode_t nodeId, eventHandle_t eventHandle)
{
    uint8_t data[CONFIG_SYS_EVENT_DATA_MAX];
    int size = 0, event;

    while (sysEventPop(nodeId, NULL, &event, data, &size) == RET_SUCCESS) {
        eventHandle(NULL, event, size, data);
    }
}