#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "export_ids.h"
#include "config.h"
#include "control_manage.h"
#include "io_module.h"
#include <gpio/gpio.h>
#include "rotary.h"
#include "sys_event_type.h"
#include "sys_event.h"

#define TRACE_TAG "control-manage"
#define TRACE_LEVEL T_INFO
#define TRACE_ENABLE

#include <common/trace.h>





/// @brief 编码器
static const rotaryConfig_t s_rotaryConfigs[] = {
    { .id = ENCODER_ROTARY, .name = "rotary", .rotaryA = EM_GPIO_ROTARY_A, .rotaryB = EM_GPIO_ROTARY_B, .address = 1},
};

/// 定义一个电机管理数据结构
typedef struct {
    // 对应句柄
    rotaryController_t *node;
    const rotaryConfig_t *config;
    // // 电流过滤器
    // struct floatFilter currentFilter;

    // motorInfo_t info;
    // motorAdminStatus_t adminStatus;
    // motorState_t state;  

    // // 电机编码器的读值
    // int32_t motorEncoderValue;
    // int encoderMultipleFactor;

    // // 状态上报定时器
    // loopTimer_t stateReportTimer;  

    // // 启动加载标志
    // bool bootChecked;
    // /// 零点位置是否有效
    // bool zeroPositionValid;
    // /// @brief 启动时编码器的位置
    // int32_t bootPosition;    
    // /// @brief 零点位置
    // int32_t zeroPosition;

    // /// 失效管理
    // motorFailureManage_t failureManage;    
}rotaryEncoder_t;

/**
 * @berif 扩展IO模块
*/
typedef struct {

    uint8_t rotaryKey;

}ioModule_t;


/**
 * @brief 定义一个旋钮管理器
 * 
 */
typedef struct
{
    /// @brief 系统是否准备好
    bool systemReady;
    /// @brief 旋钮是否已连接
    bool rotaryConnected;
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
    /// @brief 旋钮控制器句柄
    // rotaryController_t *rotary;

    // /// @brief 产测参数
    // factoryParameter_t factory;
    /// @brief 电源运行配置
    // controlConfig_t controlConfig;

    /// @brief IO模块
    ioModule_t ioModule;

    /// @brief 编码器
    rotaryEncoder_t rotarys[ARRAY_SIZE(s_rotaryConfigs)];     

    // /// @brief mqtt客户端
    // mqttClientInstance_t *mqttClient;

    // /// @brief 事件队列
    // QueueHandle_t event;

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

}controlManage_t;


// static const rotaryConfig_t s_rotaryConfig = 
// {
//     .rotaryA = EM_GPIO_ROTARY_A,
//     .rotaryB = EM_GPIO_ROTARY_B,
// };
// ENCODER_ROTARY = 0,





static controlManage_t s_controlManage = {0};
#define getInstance()       &s_controlManage


// #define powerSwitchIsPressed()          (m->ioModule.powerSwitch->polarity == IO_DEFAULT_LOW_ACTIVE) ?  !gpioGet(m->ioModule.powerSwitchId) : gpioGet(m->ioModule.powerSwitchId)


static void controlManageTask(void *pvParameters);
static bool keyDetection(uint8_t keyId);



int controlManageInit(void)
{
    int ret = RET_FAILED;
    controlManage_t *m = getInstance();
    osMemset(m, 0, sizeof(controlManage_t));


    // m->ioModule.rotaryKeyManage = ioModuleNew(m->ioModule.rotaryKey, "key", false, 1);
    // ASSERT(m->ioModule.rotaryKeyManage != NULL, "rotary key module new failed");

    m->ioModule.rotaryKey = EM_GPIO_ROTARY_KEY;
    // m->ioModule.rotaryA = EM_GPIO_ROTARY_A;
    // m->ioModule.rotaryB = EM_GPIO_ROTARY_B;
    // m->ioModule.rotaryKey = EM_GPIO_ROTARY_KEY;
    // m->ioModule.rotaryKeyManage = ioModuleNew(m->ioModule.rotaryKey, "key", false, 1);
    // ASSERT(m->ioModule.rotaryKeyManage != NULL, "rotary key module new failed");

    // m->ioModule.rotaryController = rotaryNew(EM_GPIO_ROTARY_A, EM_GPIO_ROTARY_B, "rotary", &m->ioModule.rotaryConfig);
    // ASSERT(m->ioModule.rotaryController != NULL, "rotary controller new failed");


    // // 创建编码器节点
    // for (int i = 0; i < ARRAY_SIZE(s_rotaryConfigs); i ++){
    //     ret = rotaryEncoderNodeInit(&m->rotarys[i], &s_rotaryConfigs[i]);
    //     ASSERT(ret == RET_SUCCESS, "Init rotary node failed");
    // }
    ret = rotaryEncoderInit( NULL);
    ASSERT(ret == RET_SUCCESS, "rotary encoder init failed");


    
    xTaskCreate(controlManageTask, "control-manage", 4096, NULL, CONFIG_CONTROL_MANAGE_TASK_PRIORITY, NULL);

    return RET_SUCCESS;
}




// static int rotaryEncoderNodeInit(rotaryEncoder_t *rotary, const rotaryConfig_t *config)
// {
//     osMemset(rotary, 0, sizeof(*rotary));
//     rotary->node = rotaryNew(config->address);
//     if (rotary->node == NULL) {
//         return RET_NO_MEM;
//     }

//     rotary->config = config;
    
//     // 配置节点
//     return rotaryEncoderInit(rotary->node);
// }



static void controlManageTask(void *pvParameters)
{
    controlManage_t *m = getInstance();
    bool keyPressed = false;

    ilog("control manage task start...");
    static int lastCount = 0;
    int count = 0;

    rotaryEncoderEvent_t rotaryEventMsg;
    
    while (1)
    {
        memset(&rotaryEventMsg, 0, sizeof(rotaryEncoderEvent_t));
        /// 按键检测
        keyPressed = keyDetection(m->ioModule.rotaryKey);
        if (keyPressed) {
            rotaryEventMsg.isVaild = true;
            rotaryEventMsg.isPressed = true;
            ilog("rotaryKey pressed");
            // 向电机控制发送命令
        }
        count = rotaryEncoderRead(NULL);
        if (count != lastCount) {
            lastCount = count;
            rotaryEventMsg.isVaild = true;
            rotaryEventMsg.count = lastCount;
            // ilog("rotary count=%d", count);
        }

        // ilog("rotaryEventMsg.isVaild: %d", rotaryEventMsg.isVaild);
        if(rotaryEventMsg.isVaild) {
            sysEventPost(SYS_EVENT_NODE_MOTOR, SYS_EVENT_ROTARY_MESSAGE, &rotaryEventMsg, sizeof(rotaryEncoderEvent_t));
            // ilog("rotaryEventMsg sent");
        } else {
            // ilog("rotaryEventMsg not sent");
        }

        // /// 状态刷新
        // stateUpdate();
        // /// mqtt状态同步
        // mqttSynchronous();
        /// io模块管理（闪烁）
        // ioModuleSchedule(m->ioModule.greenIndicatorLampManage);
        // ioModuleSchedule(m->ioModule.redIndicatorLampManage);
        // ioModuleSchedule(m->ioModule.powerIndicatorLampManage);
    //     /// 告警状态刷新
    //     alertManageSchedule();

    //     /// 事件处理
    //     eventQueueReceive();

    //     //控制台打印不需要锁
    // #ifdef CONFIG_CONSOLE
    //     if(m->console.traceTimer.enable && loopTimerExpired(&m->console.traceTimer, upTime()))
    //     {
    //         dumpState();
    //     }
    // #endif

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static bool keyDetection(uint8_t keyId)
{
    int level = gpioGet(keyId);

    if (level == 0) {
        while (gpioGet(keyId) == 0) {
            vTaskDelay(5);
        }
        ilog("rotaryKey pressed");
        return true;
    }
    return false;
}
