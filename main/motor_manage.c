
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "export_ids.h"
#include "config.h"
#include "motor_manage.h"
#include "motor.h"
#include "sys_event.h"

#define TRACE_TAG "motor-manage"
#define TRACE_LEVEL T_INFO
#define TRACE_ENABLE

#include <common/trace.h>


/**
 * @brief 定义一个电机管理器
 * 
 */
typedef struct
{
    /// @brief 系统是否准备好
    bool systemReady;
    /// @brief 电机是否已连接
    bool motorConnected;
    // /// @brief 是否已连接到mqtt服务器
    // bool mqttConnected;
    /// @brief 发生故障
    bool faultOccurred;
    /// @brief 发生告警
    bool warnOccurred;

    /// @brief 系统时间
    uint64_t systemTime;

    /// @brief 上电延迟，用于首次检查是否在线
    sysTick_t powerOnDelay;

    /// @brief 运行状态
    uint32_t runState;

    /// @brief 参数锁
    // SemaphoreHandle_t mutex;

    /// @brief motor控制器句柄
    motorController_t *motor;

    // /// @brief 产测参数
    // factoryParameter_t factory;
    /// @brief 电源运行配置
    motorConfig_t motorConfig;

    // /// @brief mqtt客户端
    // mqttClientInstance_t *mqttClient;

    /// @brief 事件队列
    QueueHandle_t event;

    // /// @brief web服务器
    // webServiceInstance_t *webService;

    // /// cjson对象管理
    // powerInfoCjsonMsg_t deviceInfoMsg;
    // powerStateCjsonMsg_t deviceStateMsg;

    /// 同步参数管理
    // syncManage_t syncManage;

// #ifdef CONFIG_CONSOLE
//     struct
//     {
//         /// @brief 是否为打印info
//         uint8_t traceInfo;
//         /// @brief 控制台定时器
//         loopTimer_t traceTimer;
//     }console;

// #endif

}motorManage_t;

static motorManage_t s_motorManage = {0};
#define getInstance()       &s_motorManage

static void motorManageTask(void *pvParameters);
static void eventQueueReceive(void);
static void motorEventHandle(void *user, int event, int size, uint8_t *data);

int motorManageInit(void)
{
    // int ret;
    motorManage_t *m = getInstance();
    osMemset(m, 0, sizeof(motorManage_t));

    // 配置上电延时时间 
    // RESET_EXPIRED_TIME(m->powerOnDelay, CONFIG_POWER_DEFAULT_OFFLINE_TIME);

    m->motor = motorNew(EM_CAN_MOTOR, "motor", &m->motorConfig);
    ASSERT(m->motor != NULL, "motor new failed");

    // 注册消息队列
    m->event = sysEventRegister(SYS_EVENT_NODE_MOTOR, "motor", m);
    ASSERT(m->event != NULL, "sys event register failed");

    //创建电源管理任务
    xTaskCreate(motorManageTask, "motor", 4096, NULL, CONFIG_MOTOR_MANAGE_TASK_PRIORITY, NULL);

    return RET_SUCCESS;
}

static void motorManageTask(void *pvParameters)
{
    motorManage_t *m = getInstance();

    ilog("motor manage task start...");
    while (1)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        // 事件处理
        eventQueueReceive();

        // 等待系统起来 这里是在事件中处理的  暂时不使用
        // if(!m->systemReady) {
        //     continue;
        // }

        // 电机调度
        motorSchedule(m->motor);

        // // 状态刷新
        // stateUpdate();

        // // mqtt状态同步
        // mqttSynchronous();
        
    //     //控制台打印不需要锁
    // #ifdef CONFIG_CONSOLE
    //     if(m->console.traceTimer.enable && loopTimerExpired(&m->console.traceTimer, upTime())) {
    //         m->console.traceInfo ? dumpInfo() : dumpState();
    //     }
    // #endif
    }
}

static void eventQueueReceive(void)
{
    motorManage_t *m = getInstance();
    //void *user;
    uint8_t data[CONFIG_SYS_EVENT_DATA_MAX];
    int size = 0, event;

    while (sysEventPop(SYS_EVENT_NODE_MOTOR, NULL, &event, data, &size) == RET_SUCCESS) {
        motorEventHandle(NULL, event, size, data);
    }
}


static void motorEventHandle(void *user, int event, int size, uint8_t *data)
{
    motorManage_t *m = getInstance();
    // ilog("motor manage event: %d, size: %d", event, size);
    switch (event) {
        case SYS_EVENT_ROTARY_MESSAGE:
            int speedLevel = 0;
            rotaryEncoderEvent_t *rotaryEvent = (rotaryEncoderEvent_t *)data;
            // ilog("rotaryEvent: %p", rotaryEvent);
            // ilog("rotaryEvent->isVaild: %d", rotaryEvent->isVaild);
            // ilog("rotaryEvent->isPressed: %d", rotaryEvent->isPressed);
            // ilog("rotaryEvent->count: %d", rotaryEvent->count);
            // ilog("motor output current: %d", m->motorConfig.outputCurrentSet);
            // ilog("rotaryEvent->count: %d", rotaryEvent->count);
            if (rotaryEvent->isVaild) {
                // if (rotaryEvent->isPressed) {
                //     ilog("change motor enable");
                //     m->motorConfig.enable = !m->motorConfig.enable;
                // }

                speedLevel = rotaryEvent->count;
                if (speedLevel > 50) {
                    speedLevel = 50;
                } else if (speedLevel < -50) {
                    speedLevel = -50;
                }
                speedLevel = speedLevel / 5;
                speedLevel = speedLevel + 10;
                motorSetConfig(m->motor, rotaryEvent->isPressed, speedLevel);
                // ilog("speedLevel: %d", speedLevel);
                // if (m->motorConfig.outputCurrentSet != speedLevel) {
                //     m->motorConfig.outputCurrentSet = speedLevel;
                //     ilog("motor output current set: %d", m->motorConfig.outputCurrentSet);
                // }
            }
            break;
        default:
            break;
    }
}