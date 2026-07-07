#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "i2c/i2c.h"
#include "coil.h"

#define TRACE_TAG "coil-manage"
#define TRACE_LEVEL T_DEBUG
#define TRACE_ENABLE

#include <common/trace.h>

#define LDC1614_DEVICE_ID_ADDR 0x7F
// LDC1614 寄存器地址
#define REG_DATA_CH0_MSB       0x00
#define REG_DATA_CH0_LSB       0x01
#define REG_DATA_CH1_MSB       0x02
#define REG_DATA_CH1_LSB       0x03
#define REG_DATA_CH2_MSB       0x04
#define REG_DATA_CH2_LSB       0x05
#define REG_DATA_CH3_MSB       0x06
#define REG_DATA_CH3_LSB       0x07
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
    /// 状态
    coilManageState_t state;
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

static int ldc1614_configure(coilManage_t *coil);
static uint16_t ldc1614_read_register(coilManage_t *coil, uint8_t reg);
static void ldc1614_debug(coilManage_t *coil);
static uint32_t read_channel_data(coilManage_t *coil, uint8_t msb_addr);


// const uint16_t ldc1614Reg[] = {0x08, 0x09, 0x0A, 0x0B, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21};
// const uint16_t ldc1614RegData[] = {0xffff, 0xffff, 0xffff, 0xffff, 0x0400, 0x0400, 0x0400, 0x0400, 0x1001, 0x1001, 0x1001, 0x1001,  }


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

    uint8_t data[2] = {0};
    int ret = 0;
    ret = i2cReadByteData(m->bus, m->config->address, LDC1614_DEVICE_ID_ADDR, data, 2);
    if (ret != RET_SUCCESS) {
        elog("LDC1614 device ID read failed, bus=%d, address=0x%x, reg=0x%x", m->bus, m->config->address, LDC1614_DEVICE_ID_ADDR);
        osFree(m);
        return NULL;
    } 

    dlog("LDC1614 device ID read success, bus=%d, address=0x%x, reg=0x%x, data=%x %x", m->bus, m->config->address, LDC1614_DEVICE_ID_ADDR, data[0], data[1]);
    if (data[0] != 0x30 || data[1] != 0x55) {
        elog("LDC1614 device ID read failed, bus=%d, address=0x%x, reg=0x%x, data=%x %x", m->bus, m->config->address, LDC1614_DEVICE_ID_ADDR, data[0], data[1]);
        osFree(m);
        return NULL;
    }

    ilog("coilManageNew success");
    return m;
}

static int ldc1614_write_register(coilManage_t *coil, uint8_t reg, uint16_t data)
{
    int ret = 0;
    ret = i2cWriteByteData(coil->bus, coil->config->address, reg, (uint8_t *)&data, 2);
    if (ret != RET_SUCCESS) {
        elog("LDC1614 write register failed, bus=%d, address=0x%x, reg=0x%x", coil->bus, coil->config->address, reg);
        return RET_FAILED;
    }
    return RET_SUCCESS;
}

static int ldc1614_configure(coilManage_t *coil)
{
    int ret = 0;
    int i = 0;
    uint16_t config = 0;

    config = 0xffff;
    for(i = 0; i < 4; i++) {
        ldc1614_write_register(coil, 0x08+i, config);
    }

    config = 0x0;
    for (i = 0; i < 4; i++) {
        ldc1614_write_register(coil, 0x0C+i, config);
    }

    // config = 0x0400;
    config = 0x0004;
    for (i = 0; i < 4; i++) {
        ldc1614_write_register(coil, 0x10+i, config);
    }

    // config = 0x1001;
    config = 0x0110;
    for (i = 0; i < 4; i++) {
        ldc1614_write_register(coil, 0x14+i, config);
    }

    // config = 0x1e01;
    config = 0x011e;
    ldc1614_write_register(coil, 0x1A, config);

    // config = 0x820c;
    config = 0x0c82;
    ldc1614_write_register(coil, 0x1B, config);

    config = 0x0000;
    ldc1614_write_register(coil, 0x1c, config);

    // config = 0x8c40;
    config = 0x408c;
    ldc1614_write_register(coil, 0x1E, config);
    ldc1614_write_register(coil, 0x1F, config);
    vTaskDelay(pdMS_TO_TICKS(10));  // 等待芯片稳定

    // // config = 0x8800;
    // config = 0x0088;
    // ldc1614_write_register(coil, 0x20, config);
    // ldc1614_write_register(coil, 0x21, config);
    // vTaskDelay(pdMS_TO_TICKS(10));  // 等待芯片稳定

    return RET_SUCCESS;









    // // 1. 唤醒芯片 (配置寄存器)
    // // CONFIG 寄存器 (0x1A)
    // // BIT15: 软件复位 (1=复位)
    // // BIT13: 关断模式 (0=唤醒, 1=关断)
    // // BIT11-10: 转换模式 (01=单次, 10=连续)
    // uint16_t config = 0x0000;
    // config |= (0 << 13);   // 唤醒 (0=唤醒)
    // config |= (2 << 10);   // 连续转换模式 (10)
    // config |= (1 << 0);    // 使能传感器 (ACTIVE_CH0)

    // config = 0x1e01;
    // ret = i2cWriteByteData(coil->bus, coil->config->address, 0x1A, (uint8_t *)&config, 2);
    // if (ret != RET_SUCCESS) {
    //     elog("LDC1614 configure failed, bus=%d, address=0x%x, reg=0x%x", coil->bus, coil->config->address, 0x1A);
    //     return RET_FAILED;
    // }
    // // ldc1614_write_register(0x1A, config);
    
    // // 2. 设置 MUX_CONFIG (0x1B)
    // // BIT0-2: 选择通道序列
    // // uint16_t muxConfig = 0x0001;
    // uint16_t muxConfig = 0x820c;
    // ret = i2cWriteByteData(coil->bus, coil->config->address, 0x1B, (uint8_t *)&muxConfig, 2);
    // if (ret != RET_SUCCESS) {
    //     elog("LDC1614 configure failed, bus=%d, address=0x%x, reg=0x%x", coil->bus, coil->config->address, 0x1B);
    //     return RET_FAILED;
    // }
    // // ldc1614_write_register(0x1B, 0x0001);  // 只使能通道0
    
    // // 3. 设置 RCOUNT (参考计数) - 决定转换时间
    // // RCOUNT_CH0 (0x08)
    // // uint16_t rcount = 0x0BB8;
    // uint16_t rcount = 0xFFFF;
    // ret = i2cWriteByteData(coil->bus, coil->config->address, 0x08, (uint8_t *)&rcount, 2);
    // if (ret != RET_SUCCESS) {
    //     elog("LDC1614 configure failed, bus=%d, address=0x%x, reg=0x%x", coil->bus, coil->config->address, 0x08);
    //     return RET_FAILED;
    // }
    // // ldc1614_write_register(0x08, 0x0BB8);  // 3000 计数 (典型值)
    
    // // 4. 设置 SETTLECOUNT (稳定计数)
    // // SETTLECOUNT_CH0 (0x10)
    // // uint16_t settlecount = 0x0064;
    // uint8_t settlecount = 0x400;
    // ret = i2cWriteByteData(coil->bus, coil->config->address, 0x10, (uint8_t *)&settlecount, 2);
    // if (ret != RET_SUCCESS) {
    //     elog("LDC1614 configure failed, bus=%d, address=0x%x, reg=0x%x", coil->bus, coil->config->address, 0x10);
    //     return RET_FAILED;
    // }
    // // ldc1614_write_register(0x10, 0x0064);  // 100 计数 (典型值)
    
    // // 5. 设置驱动电流
    // // DRIVE_CURRENT_CH0 (0x1E)
    // // uint16_t driveCurrent = 0x8000;
    // uint16_t driveCurrent = 0x8c40;
    // ret = i2cWriteByteData(coil->bus, coil->config->address, 0x1E, (uint8_t *)&driveCurrent, 2);
    // if (ret != RET_SUCCESS) {
    //     elog("LDC1614 configure failed, bus=%d, address=0x%x, reg=0x%x", coil->bus, coil->config->address, 0x1E);
    //     return RET_FAILED;
    // }
    
    vTaskDelay(pdMS_TO_TICKS(10));  // 等待芯片稳定
    return RET_SUCCESS;
}




static uint16_t ldc1614_read_register(coilManage_t *coil, uint8_t reg)
{
    uint8_t readBuffer[2] = {0};
    int ret = 0;
    ret = i2cReadByteData(coil->bus, coil->config->address, reg, readBuffer, 2);
    if (ret != RET_SUCCESS) {
        elog("LDC1614 read register failed, bus=%d, address=0x%x, reg=0x%x", coil->bus, coil->config->address, reg);
        return 0;
    }
    return (readBuffer[0] << 8) | readBuffer[1];
}

/**
 * @brief 调试函数：读取并打印所有关键寄存器
 */
static void ldc1614_debug(coilManage_t *coil)
{
    ilog("========== LDC1614 Debug ==========");
    for (int i = 0x08; i < 0x22; i++) {
        uint16_t data = ldc1614_read_register(coil, i);
        ilog("REG 0x%02X: 0x%04X", i, data);
    }

    // 1. 检查设备 ID
    uint16_t id = ldc1614_read_register(coil, 0x7F);
    ilog("Device ID:     0x%04X %s", id, (id == 0x3055) ? "[OK]" : "[ERROR]");

    // 2. 读取配置寄存器
    uint16_t config = ldc1614_read_register(coil, 0x1A);
    ilog("CONFIG:        0x%04X", config);
    ilog("  - Power mode: %s", (config & (1 << 13)) ? "SHUTDOWN" : "ACTIVE [OK]");
    ilog("  - Conv mode:  %s", ((config >> 10) & 0x03) == 2 ? "Continuous [OK]" : "Other");

    // // 3. 读取通道0数据 (先读MSB触发锁存，再读LSB)
    // uint32_t ch0_data = read_channel_data(coil, 0x00);
    // uint16_t msb = (ch0_data >> 16) & 0xFFFF;
    // uint16_t lsb = ch0_data & 0xFFFF;
    // ilog("CH0 MSB:       0x%04X", msb);
    // ilog("  - Error flags: 0x%X %s", (msb >> 12) & 0xF, ((msb >> 12) & 0xF) == 0 ? "[OK]" : "[WARN]");
    // ilog("  - Data high:   0x%03X", msb & 0x0FFF);
    // ilog("CH0 LSB:       0x%04X", lsb);
    // ilog("CH0 Full 28-bit: 0x%08X", (unsigned int)ch0_data);

    // 6. 检查其他关键寄存器
    uint16_t rcount = ldc1614_read_register(coil, 0x08);
    ilog("RCOUNT_CH0:    0x%04X", rcount);

    uint16_t settle = ldc1614_read_register(coil, 0x10);
    ilog("SETTLECOUNT:   0x%04X", settle);

    uint16_t drive = ldc1614_read_register(coil, 0x1E);
    ilog("DRIVE_CURRENT: 0x%04X", drive);

    ilog("====================================");
}



/**
 * @brief coil Schedule
 * 
 * @param coil 
 */
void coilManageSchedule(coilManage_t *coil)
{
    int ret = 0;
    if (!coil->state.ready) {
        ret = ldc1614_configure(coil);
        if (ret != RET_SUCCESS) {
            elog("LDC1614 configure failed, bus=%d, address=0x%x", coil->bus, coil->config->address);
            return;
        }
        coil->state.ready = true;
        ilog("LDC1614 configure success, bus=%d, address=0x%x", coil->bus, coil->config->address);
        ldc1614_debug(coil);
    }

    // 单次I2C事务读取MSB+LSB，保证同一时刻的完整数据快照
    uint32_t ch0data = read_channel_data(coil, REG_DATA_CH0_MSB);
    ilog("CH0 Data: 0x%08X (%u)", (unsigned int)ch0data, (unsigned int)ch0data);
    // 单次I2C事务读取MSB+LSB，保证同一时刻的完整数据快照
    uint32_t ch1data = read_channel_data(coil, REG_DATA_CH1_MSB);
    ilog("CH1 Data: 0x%08X (%u)", (unsigned int)ch1data, (unsigned int)ch1data);
    // // 单次I2C事务读取MSB+LSB，保证同一时刻的完整数据快照
    // uint32_t ch2data = read_channel_data(coil, REG_DATA_CH2_MSB);
    // ilog("CH2 Data: 0x%08X (%u)", (unsigned int)ch2data, (unsigned int)ch2data);
    // // 单次I2C事务读取MSB+LSB，保证同一时刻的完整数据快照
    // uint32_t ch3data = read_channel_data(coil, REG_DATA_CH3_MSB);
    // ilog("CH3 Data: 0x%08X (%u)", (unsigned int)ch3data, (unsigned int)ch3data);
}


/**
 * @brief 读取通道28位数据 (两次独立I2C读：先读MSB触发锁存，再读LSB)
 * @param coil 线圈管理器
 * @param msb_addr MSB寄存器地址 (LSB地址为 msb_addr+1)
 * @return 28位数据，读取失败返回0
 *
 * @note LDC1614 要求先发起一次独立的I2C读操作读取DATAx_MSB以触发
 *       数据更新和锁存，随后立即发起第二次独立的I2C读操作读取DATAx_LSB，
 *       才能拿到同一次转换的完整数据。不能用单次突发读。
 */
static uint32_t read_channel_data(coilManage_t *coil, uint8_t msb_addr)
{
    // 第一次I2C读：读MSB寄存器，触发芯片内部锁存LSB
    uint16_t msb = ldc1614_read_register(coil, msb_addr);
    // 紧接着第二次I2C读：读LSB寄存器（此时锁存的是与MSB同一次转换的数据）
    uint16_t lsb = ldc1614_read_register(coil, msb_addr + 1);

    if (msb & 0xF000) {
        elog("Error flags: 0x%04X", msb & 0xF000);
    }

    uint32_t result = ((uint32_t)(msb & 0x0FFF) << 16) | lsb;
    return result;
}