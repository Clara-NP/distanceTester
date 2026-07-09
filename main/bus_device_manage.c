#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "bus_device_manage.h"
#include "export_ids.h"
#include "config.h"
#include "coil.h"
#include "display.h"
#include "sys_event.h"

#define TRACE_TAG "bus-device-manage"
#define TRACE_LEVEL T_INFO
#define TRACE_ENABLE

#include <common/trace.h>

/**
 * @brief BUS设备类型
 * 
 */
enum BUS_DEVICE_TYPE
{
    /// @brief 磁感应线圈
    BUS_DEVICE_TYPE_COIL,
    /// @brief 液晶屏
    BUS_DEVICE_TYPE_DISPLAY,

    BUS_DEVICE_TYPE_ALL,
};

/**
 * @brief 设备参数
 * 
 */
typedef struct
{
    /// @brief 设备类型
    uint8_t type;
    /// @brief 设备名称
    const char *name;
    /// @brief 设备配置
    const void *config;
}busDevicesInfo_t;

static coilConfig_t coilConfig = {
    .address = 0x2A,
    .channelEna = COIL_CHANNEL_ENABLE_0 | COIL_CHANNEL_ENABLE_1,
    // .detectionPeriod = 1000,
    // .samplePeriodP0 = 100,
    // .samplePeriodP1 = 200,
    // .samplePeriodP2 = 300,
    // .pt = 1000,
    // .ct = 1000,
};

static const lcdDisplayConfig_t displayConfig = {
    .id = 0,
    .bus = EM_SPI_1,
    .dc = EM_GPIO_LCD_DC,
    .cs = -1,
    .rst = EM_GPIO_LCD_RST,
    .backlight = EM_GPIO_LCD_BL,
    .driver = LCD_DRIVER_TYPE_ST7789,
    .columns = 240,
    .rows = 240,
    .polarityReversal = 0,
};


/**
 * @brief bus总线设备
 * 
 */
static busDevicesInfo_t busDevicesInfo[] = {
    // 磁感应线圈
    {BUS_DEVICE_TYPE_COIL, "magnetic induction coil",   &coilConfig},
    /// 液晶屏
    {BUS_DEVICE_TYPE_DISPLAY, "display",   &displayConfig},
};

/**
 * @brief 设备参数
 * 
 */
typedef struct {
    /// @brief 设备信息
    const busDevicesInfo_t *info;

    /// @brief 是否已连接
    bool connected;
    /// @brief 发生故障
    bool faultOccurred;
    /// @brief 发生告警
    bool warnOccurred;

    /// @brief 上电延迟，用于首次检查是否在线
    sysTick_t powerOnDelay;

    /// @brief 运行状态
    uint32_t runState;

    /// @brief bus实例
    void *bus;

    // /// cjson对象管理
    // busDeviceInfoCjsonMsg_t deviceInfoMsg;
    // busDeviceStateCjsonMsg_t deviceStateMsg;

    // /// 同步参数管理
    // syncManage_t syncManage;

}busDevices_t;

/**
 * @brief 定义一个bus设备控制器
 * 
 */
typedef struct {
    // /// @brief 系统是否准备好
    // bool systemReady;
    // /// @brief 是否已连接到mqtt服务器
    // bool mqttConnected;

    /// @brief 系统时间
    uint64_t systemTime;

    /// @brief 事件队列
    QueueHandle_t event;
    // /// @brief 产测参数
    // factoryParameter_t factory;

    // /// @brief mqtt客户端
    // mqttClientInstance_t *mqttClient;

    // /// @brief web服务器
    // webServiceInstance_t *webService;

    /// @brief 设备参数管理
    busDevices_t device[ARRAY_SIZE(busDevicesInfo)];

// #ifdef CONFIG_CONSOLE
//     struct
//     {
//         /// @brief 是否为打印info
//         uint8_t traceInfo;
//         /// @brief 控制台打印类型
//         uint8_t traceType;
//         /// @brief 控制台定时器
//         loopTimer_t traceTimer;
//     }console;

// #endif

} busDeviceManage_t;

static busDeviceManage_t s_busDeviceManage = {0};
#define getInstance()       &s_busDeviceManage

static void busDeviceManageTask(void *pvParameters);
static void busDeviceEventHandle(void *user, int event, int size, uint8_t *data);

/**
 * @brief RS485总线设备管理任务初始化
 * 
 * @return int 
 */
int busDeviceManageInit(void)
{
    int ret;
    busDeviceManage_t *m = getInstance();
    osMemset(m, 0, sizeof(busDeviceManage_t));

    for (int i = 0; i < ARRAY_SIZE(m->device); i++) {
        m->device[i].info = &busDevicesInfo[i];
        switch (busDevicesInfo[i].type) {
            case BUS_DEVICE_TYPE_COIL:
                m->device[i].bus = coilManageNew(EM_I2C_1, busDevicesInfo[i].name, busDevicesInfo[i].config);
                break;
            case BUS_DEVICE_TYPE_DISPLAY:
                ilog("device type = %d", BUS_DEVICE_TYPE_DISPLAY);
                m->device[i].bus = displayNew(EM_SPI_1, busDevicesInfo[i].name, busDevicesInfo[i].config);
                break;
            default:
                break;
        }
    }
    // 注册消息队列
    m->event = sysEventRegister(SYS_EVENT_NODE_BUS_DEVICE, "bus_device", m);
    ASSERT(m->event != NULL, "sys event register failed");

    xTaskCreate(busDeviceManageTask, "busDeviceManageTask", 4096, NULL, CONFIG_BUS_DEVICE_TASK_PRIORITY, NULL);
    return RET_SUCCESS;
}

static void busDeviceManageTask(void *pvParameters)
{
    busDeviceManage_t *m = getInstance();
    busDevices_t *device;

    ilog("bus device manage task start...");

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // 事件处理
        eventQueueReceive(SYS_EVENT_NODE_BUS_DEVICE, busDeviceEventHandle);

        for (int i = 0; i < ARRAY_SIZE(m->device); i++) {
            device = &m->device[i];
            switch (device->info->type) {
                case BUS_DEVICE_TYPE_COIL:
                    coilManageSchedule(device->bus);
                    break;
                case BUS_DEVICE_TYPE_DISPLAY:
                    displayManageSchedule(device->bus);
                    break;
                default:
                    break;
            }
        }

    }
}

static void busDeviceEventHandle(void *user, int event, int size, uint8_t *data)
{
    busDeviceManage_t *m = getInstance();
    // ilog("bus device manage event: %d, size: %d", event, size);
    switch (event) {
        case SYS_EVENT_MOTOR_MESSAGE:
            motorMessageEvent_t *motorMessageEvent = (motorMessageEvent_t *)data;
            ilog("bus device isPressed: %d, speedLevel: %d", motorMessageEvent->isPressed, motorMessageEvent->speedLevel);
            displaySetSpeedLevel(motorMessageEvent->speedLevel);
            displaySetIsPressed(motorMessageEvent->isPressed);
            break;
        default:
            break;
    }
}