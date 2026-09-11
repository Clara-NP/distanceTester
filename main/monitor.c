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
#include "coil_ch1_distance_lut.h"

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
static int coilDistanceFromCh1(uint32_t ch1, uint8_t *quality);

/**
 * @brief NO.2 LUT（ch1 降序）线性插值，返回 0.1 mm；quality 取两端较差档
 */
static int coilDistanceFromCh1(uint32_t ch1, uint8_t *quality)
{
    const coil_dist_lut_entry_t *lut = kCoilDistLut;
    int n = COIL_DIST_LUT_LEN;
    int lo;
    int hi;
    uint32_t span;
    uint32_t off;
    int dist;
    uint8_t q;

    if (n < 1) {
        if (quality) {
            *quality = 3;
        }
        return 0;
    }

    if (ch1 >= lut[0].ch1) {
        if (quality) {
            *quality = lut[0].quality;
        }
        return (int)lut[0].dist_01mm;
    }
    if (ch1 <= lut[n - 1].ch1) {
        if (quality) {
            *quality = lut[n - 1].quality;
        }
        return (int)lut[n - 1].dist_01mm;
    }

    lo = 0;
    hi = n - 1;
    while (lo < hi) {
        int mid = (lo + hi + 1) / 2;
        if (lut[mid].ch1 >= ch1) {
            lo = mid;
        } else {
            hi = mid - 1;
        }
    }
    hi = lo + 1;
    if (hi >= n) {
        if (quality) {
            *quality = lut[lo].quality;
        }
        return (int)lut[lo].dist_01mm;
    }

    q = lut[lo].quality;
    if (lut[hi].quality > q) {
        q = lut[hi].quality;
    }
    if (quality) {
        *quality = q;
    }

    span = lut[lo].ch1 - lut[hi].ch1;
    if (span == 0) {
        return (int)lut[lo].dist_01mm;
    }
    off = lut[lo].ch1 - ch1;
    dist = (int)lut[lo].dist_01mm
        + (int)(((int32_t)lut[hi].dist_01mm - (int32_t)lut[lo].dist_01mm) * (int32_t)off / (int32_t)span);
    return dist;
}

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
    if (m->coilState.valid && m->coilState.updateTime > 0 && m->motorState.dataUpdateTime > 0) {
        // totalAngle 为 int32 累计计数；先转 float 再运算，避免整型中间结果溢出
        float rev = (float)m->motorState.totalAngle / 8192.0f;
        int motorDistance = (int)(rev / 36.0f * 5.0f * 10.0f); // 0.1mm
        int testDistance = coilDistanceFromCh1(m->coilState.data[1], NULL);
        int realDistance = motorDistance >= 0 ? motorDistance : -motorDistance;

        ilog("ch1=%u test_d=%d real_d=%d (0.1 mm)",
            (unsigned int)m->coilState.data[1],
            testDistance,
            realDistance);

        m->coilState.updateTime = 0;
        m->coilState.valid = false;
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