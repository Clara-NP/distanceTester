#ifndef __SYS_EVENT_TYPE_H__
#define __SYS_EVENT_TYPE_H__

#include <common/generic.h>

/**
 * @brief 定义事件数据的最大值
 * 
 */
#ifndef CONFIG_SYS_EVENT_DATA_MAX
#define CONFIG_SYS_EVENT_DATA_MAX     512
#endif


typedef enum {
    SYS_EVENT_NODE_MOTOR = 0,
    SYS_EVENT_NODE_ROTARY,
    SYS_EVENT_NODE_MAX,
}sysEventNode_t;

typedef enum {
    // MOTOR事件
    SYS_EVENT_MOTOR_ONLINE = 0,
    
    // ROTARY事件
    SYS_EVENT_ROTARY_MESSAGE = 100,



    SYS_EVENT_TYPE_MAX,
}sysEventType_t;



typedef struct {
    bool isVaild;
    bool isPressed;
    int count;
}rotaryEncoderEvent_t;


#endif