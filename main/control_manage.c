#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "export_ids.h"
#include "config.h"
#include "control_manage.h"
#include "io_module.h"
#include <gpio/gpio.h>

#define TRACE_TAG "control-manage"
#define TRACE_LEVEL T_INFO
#define TRACE_ENABLE

#include <common/trace.h>


/**
 * @berif 扩展IO模块
*/
typedef struct {
    /// EM_GPIO_ROTARY_A
    uint8_t rotaryA;

    /// EM_GPIO_ROTARY_B
    uint8_t rotaryB;

    /// EM_GPIO_ROTARY_KEY
    uint8_t rotaryKey;
    ioModuleManage_t *rotaryKeyManage;


    // /// 电源开关ID 
    // uint8_t powerSwitchId;
    // const customIoFunction_t *powerSwitch;
    // /// 紧急开关ID
    // uint8_t emergencySwitchId;
    // const customIoFunction_t *emergencySwitch;
    // /// 通用开关ID
    // uint8_t generalSwitchId;
    // const customIoFunction_t *generalSwitch;
    // /// 绿色指示灯ID
    // uint8_t greenIndicatorLampId;
    // const customIoFunction_t *greenIndicatorLamp;
    // /// @brief 电源指示灯闪烁控制实例
    // ioModuleManage_t *greenIndicatorLampManage;
    // /// 黄色指示灯ID
    // uint8_t redIndicatorLampId;
    // const customIoFunction_t *redIndicatorLamp;
    // /// @brief 电源指示灯闪烁控制实例
    // ioModuleManage_t *redIndicatorLampManage;
    // /// 接触器ID
    // uint8_t contactorId;
    // const customIoFunction_t *contactor;
    // /// 电源指示灯ID
    // uint8_t powerIndicatorLampId;
    // const customIoFunction_t *powerIndicatorLamp;
    // /// @brief 电源指示灯闪烁控制实例
    // ioModuleManage_t *powerIndicatorLampManage;

    // /// @brief 电源开关状态
    // bool powerSwitchState;
    // /// @brief 接触器开关
    // bool contactorState;
    // /// @brief 紧急开关状态
    // bool emergencySwitchState;
    // /// @brief 通用开关状态
    // bool generalSwitchState;
    // /// @brief 电源按键松手标记，用于标记长按(供电状态下)是否已经出发断开电源
    // bool powerSwitchReleaseFlag;

    // /// @brief 电源按键1秒超时时间（供电/断电）
    // sysTick_t powerSupplyExpired;
    // /// @brief 电源按键10秒超时时间（重启）
    // sysTick_t powerResetExpired;
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


static controlManage_t s_controlManage = {0};
#define getInstance()       &s_controlManage


// #define powerSwitchIsPressed()          (m->ioModule.powerSwitch->polarity == IO_DEFAULT_LOW_ACTIVE) ?  !gpioGet(m->ioModule.powerSwitchId) : gpioGet(m->ioModule.powerSwitchId)


static void controlManageTask(void *pvParameters);
static bool keyDetection(uint8_t keyId);



int controlManageInit(void)
{

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

    xTaskCreate(controlManageTask, "control-manage", 4096, NULL, CONFIG_CONTROL_MANAGE_TASK_PRIORITY, NULL);

    return RET_SUCCESS;
}

static void controlManageTask(void *pvParameters)
{
    controlManage_t *m = getInstance();
    bool keyPressed = false;

    ilog("control manage task start...");
    while (1)
    {

        /// 按键检测
        keyPressed = keyDetection(m->ioModule.rotaryKey);
        if (keyPressed) {
            // ilog("rotaryKey pressed");
            // 向电机控制发送命令
        }
        // /// 12V外部电压检测
        // externalVoltageDetection();
        // /// 充电枪插入检测
        // chargeGunDetection();
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
    //     /// 防拆状态同步
    //     tamperSynchronous();
    //     /// 系统状态指示更新
    //     systemIndicatorUpdate();
    //     /// 事件处理
    //     eventQueueReceive();
    //     /// 延迟执行重启，避免阻塞事件队列消费
    //     handlePendingReboot();
        
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
