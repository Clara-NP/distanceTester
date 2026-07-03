#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "i2c/i2c.h"
#include "coil.h"

#define TRACE_TAG "coil-manage"
#define TRACE_LEVEL T_INFO
#define TRACE_ENABLE

#include <common/trace.h>

/**
 * @brief 定义一个磁感应线圈管理器
 * 
 */
struct coilManage
{
    /// 逻辑ID
    //uint8_t id;
    /// UART ID
    uint8_t bus;
    // /// 超时计数
    // uint8_t timeoutCount;
    // /// 空调类型
    // uint8_t type;
    // /// 协议是否已经确认
    // uint8_t typeConfirm;
    /// 配置
    const coilConfig_t *config;
    // /// 状态
    // acManageState_t state;
    // /// 空调信息
    // acManageInfo_t info;

    // /// 下次探测时间
    // sysTick_t expiredDetection;
    // /// 离线时间
    // sysTick_t expiredOffline;
    // /// 一次性读出25个寄存器的数据，所以这里只取p0优先级
    // sysTick_t expiredP0;
    // /// 运行时间1秒超时
    // sysTick_t expiredSecond;

    // /// @brief 运行配置
    // acRunConfig_t runConfig;
};


/**
 * @brief 创建一个磁感应线圈管理器
 * 
 * @param bus 
 * @param name 
 * @return coilManage_t * 
 */
coilManage_t *coilManageNew(uint8_t bus, const char *name, const coilConfig_t *config)
{
    coilManage_t *m;
    m = (coilManage_t *)osMalloc(sizeof(coilManage_t));
    if (!m) {
        wlog("coilManageNew failed");
        return NULL;
    }
    osMemset(m, 0, sizeof(coilManage_t));
    m->bus = bus;
    m->config = config;
    // osMemcpy(m->name, name, strlen(name));
    // m->name[strlen(name)] = '\0';

    
    ilog("coilManageNew success");
    return m;
}

/**
 * @brief coil Schedule
 * 
 * @param coil 
 */
void coilManageSchedule(coilManage_t *coil)
{
    int ret;
    int channel = 1;
    int msbReg = 0x0 + (channel * 2);
    int lsbReg = 0x1 + (channel * 2);
    uint8_t msbHigh;
    uint8_t msbLow;
    int data;


    // int addr = 0x2A;
    // ret = i2cWriteByte(0, addr, 0x7E);
    // if (ret != RET_SUCCESS) {
    //     wlog("2A  coil read failed, ret=%d", ret);
    //     // return;
    // } else {
    //     ilog("2A  coil read success");
    // }

    // addr = 0x2B;
    // ret = i2cWriteByte(0, addr, 0x7E);
    // if (ret != RET_SUCCESS) {
    //     wlog("2B  coil read failed, ret=%d", ret);
    //     // return;
    // } else {
    //     ilog("2B  coil read success");
    // }
    // for (int i = 0; i < 0xFF; i++) {
    //     ret = i2cWriteByte(0, i, 0x7E);
    //     if (ret != RET_SUCCESS) {
    //         wlog("coil write failed, addr=%d, ret=%d", i, ret);
    //         continue;
    //     } else {
    //         ilog("coil write success, addr=%d", i);
    //     }

    //     vTaskDelay(10 / portTICK_PERIOD_MS);
    // }



    // ret = i2cWriteByte(coil->bus, coil->config->address, msbReg);
    // // ret = i2cWriteByte(coil->bus, coil->config->address, lsbReg);
    // // ret = i2cReadByte(coil->bus, coil->config->address, &data);
    // if (ret != RET_SUCCESS)
    // {
    //     wlog("coil write failed, ret=%d", ret);
    //     return;// ret;
    // }
    // ilog("coil write success");


    // ret = i2cReadByte(coil->bus, coil->config->address, &msbHigh);
    // if (ret != RET_SUCCESS)
    // {
    //     wlog("coil read failed, ret=%d", ret);
    //     return;
    // }
    // ilog("coil read success");

    // ret = i2cReadByte(coil->bus, coil->config->address, &msbLow);
    // if (ret != RET_SUCCESS)
    // {
    //     wlog("coil read failed, ret=%d", ret);
    //     return;
    // }
    // ilog("coil read success");
}