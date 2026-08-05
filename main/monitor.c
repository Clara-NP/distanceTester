#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "export_ids.h"
#include "config.h"
#include "monitor.h"
#include "sys_event.h"
#include "bus_device_manage.h"
#include "motor.h"
#include "coil.h"

#define TRACE_TAG "monitor"
#define TRACE_LEVEL T_INFO
#define TRACE_ENABLE

#include <common/trace.h>

/**
 * @brief 定义一个数据监控管理器
 * 
 */
struct dataMonitorManage{
    const char* name;
    /// @brief 运行状态
    uint32_t runState;
    /// @brief 事件队列
    QueueHandle_t event;
    /// @berif 数据保护锁
    SemaphoreHandle_t dataMutex;
    // 数据监控定时器
    sysTick_t monitorTimer;
    // 数据监控周期
    uint32_t monitorPeriod;

    // 数据监控数据
    // 电机数据上报时间
    sysTick_t motorDataReportTime;
    // 电机数据
    motorState_t motorState;
    // 线圈数据上报时间
    sysTick_t coilDataReportTime;
    // 线圈数据
    coilManageState_t coilState;
};

static dataMonitorInstance_t s_dataMonitorManage;
#define getInstance()       &s_dataMonitorManage

static void dataMonitorTask(void *pvParameters);
static void dataMonitorEventHandle(void *user, int event, int size, uint8_t *data);
static void dataMonitorUpdateInfo(void);

dataMonitorInstance_t* dataMonitorNew(const char* name)
{
    dataMonitorInstance_t *m = getInstance();

    if(name && m->name && strcmp(name, m->name) == 0) {
        return m;
    }

    osMemset(m, 0, sizeof(dataMonitorInstance_t));
    m->name = name;

    m->dataMutex = xSemaphoreCreateMutex();
    ASSERT(m->dataMutex != NULL, "data mutex create failed");

    ilog("Start data monitor success, name:%s", name ? name : "monitor");


    // 创建数据监控任务
    xTaskCreate(dataMonitorTask, "dataMonitor", 4096, NULL, CONFIG_DATA_MONITOR_TASK_PRIORITY, NULL);

    return m;

}

static void dataMonitorTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(1) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(20));
        dataMonitorUpdateInfo();
    }
}

static void dataMonitorUpdateInfo(void)
{
    dataMonitorInstance_t *m = getInstance();
    if (xSemaphoreTake(m->dataMutex, portMAX_DELAY) != pdPASS) {
        return;
    }
    if (m->coilState.updateTime > 0 && m->motorState.dataUpdateTime > 0) {
        // totalAngle 为 int32 累计计数；先转 float 再运算，避免整型中间结果溢出
        float rev = (float)m->motorState.totalAngle / 8192.0f;
        int distance = (int)(rev / 36.0f * 5.0f * 10.0f); // 0.1mm

        ilog("coil_t=%u, ch0 = %d,ch1 = %d, motor_t = %u, distance = %d (0.1 mm)", 
            (unsigned int)m->coilState.updateTime, m->coilState.data[0], m->coilState.data[1],
            (unsigned int)m->motorState.dataUpdateTime, distance);
        
        m->coilState.updateTime = 0;
    }
    xSemaphoreGive(m->dataMutex);

    return ;
}


int dataMonitorSetMotorState(dataMonitorInstance_t *m, const motorState_t *state, uint16_t timeout)
{
    if (!m || !state) {
        return RET_FAILED;
    }

    if (xSemaphoreTake(m->dataMutex, timeout / portTICK_PERIOD_MS) != pdPASS) {
        return RET_FAILED;
    }

    memcpy(&m->motorState, state, sizeof(motorState_t));
    xSemaphoreGive(m->dataMutex);

    return RET_SUCCESS;
}

int dataMonitorSetCoilState(dataMonitorInstance_t *m, const coilManageState_t *state, uint16_t timeout)
{
    if (!m || !state) {
        return RET_FAILED;
    }
    if (xSemaphoreTake(m->dataMutex, timeout / portTICK_PERIOD_MS) != pdPASS) {
        return RET_FAILED;
    }

    memcpy(&m->coilState, state, sizeof(coilManageState_t));
    xSemaphoreGive(m->dataMutex);

    return RET_SUCCESS;
}