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
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        dataMonitorUpdateInfo();
    }
}

static void dataMonitorUpdateInfo(void)
{
    dataMonitorInstance_t *m = getInstance();
    if (xSemaphoreTake(m->dataMutex, portMAX_DELAY) != pdPASS) {
        return;
    }

    ilog("telemetry t=%u coil_ready=%d coil_conn=%d coil_t=%u ch0=%u ch1=%u ch2=%u ch3=%u motor_t=%u rpm=%d",
                                                                                    (unsigned int)upTime(),
                                                                                    m->coilState.ready,
                                                                                    m->coilState.connected,
                                                                                    (unsigned int)m->coilState.updateTime,
                                                                                    (unsigned int)m->coilState.data[0],
                                                                                    (unsigned int)m->coilState.data[1],
                                                                                    (unsigned int)m->coilState.data[2],
                                                                                    (unsigned int)m->coilState.data[3],
                                                                                    (unsigned int)m->motorState.actualSpeedUpdateTime,
                                                                                    m->motorState.actualSpeed);
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