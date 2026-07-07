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


#define LDC1614_CHANNEL_0   0
#define LDC1614_CHANNEL_1   1
#define LDC1614_CHANNEL_2   2
#define LDC1614_CHANNEL_3   3

// LDC1614 寄存器地址
// MSB [15:12]   
//              15- ERR_UR0  欠量程错误   传感器频率过低（测量值低于最小可测范围）
//              14- ERR_OR0 超量程错误   传感器频率过高（测量值超出最大可测范围）
//              13- ERR_WD0 看门狗超时   传感器未能正常起振或振荡中断
//              12- ERR_AE0 幅度错误     传感器振荡幅度未稳定在 1.2V~1.8V 范围内
//                                      该位如果经常为1, 说明 DRIVE_CURRENT 或 SETTLECOUNT 配置有问题。
//     [11:0] - 高12位数据
#define LDC1614_CHANNEL_MSB_REG(x)                  (0x00 + x*2)   //0 2 4 6
#define LDC1614_CHANNEL_LSB_REG(x)                  (0x01 + x*2)   //1 3 5 7

// 转换时长  控制转换时长（精度）
// 0x00 - 0x04  resv
// 0x05 - 0xFFFF  为有效值， 转换时间：tC0 = (RCOUNT0 × 16) / fREF0
#define LDC1614_CHANNEL_RCOUNT_REG(x)               (0x08 + x) // 8 9 a b

// 用于减去固定的频率偏移，可以让你在关心的测量范围内获得更高的有效分辨率
// ƒOFFSET0=(OFFSET0/(2^16))×ƒREF0
#define LDC1614_CHANNEL_OFFSET_REG(x)               (0x0C + x) // c d e f

// 通道建立时间  控制建立时长（起振稳定）
// 0x0000 或 0x0001 → tS0 = 32 / fREF0
// tS0 = (SETTLECOUNT0 × 16) / fREF0
#define LDC1614_CHANNEL_SETTLECOUNT_REG(x)          (0x10 + x) //10 11 12 13
/**
LDC1614_CHANNEL_CLOCK_DIVIDERS_REG
[15:12] FIN_DIVIDER0 - 0- 传感器输入分频：fIN0 = fSENSOR0 / FIN_DIVIDER0
[11:10] RESERVED
[9:0]  FREF_DIVIDER0   fREF0 = fCLK / FREF_DIVIDER0
*/
#define LDC1614_CHANNEL_CLOCK_DIVIDERS_REG(x)       (0x14 + x)  // 14 15 16 17
#define LDC1614_CHANNEL_DRIVE_CURRENT_REG(x)        (0x1E + x) // 1e 1f 20 21

/**
[15:14]   错误通道指示：00→通道0，01→通道1，10→通道2，11→通道3
[13]      任意通道发生欠量程错误（汇总标志）
[12]      任意通道发生超量程错误（汇总标志）
[11]      任意通道发生看门狗超时（汇总标志）
[10]      幅度过高错误（汇总标志）
[9]       幅度过低错误（汇总标志）
[8]       零交叉错误（传感器未能产生有效过零信号）
[7]       保留 
[6]       数据就绪（Data Ready）：当有新转换数据时置1，读取后清零
[5:4]       保留
[3]       通道0有未读的新转换数据
[2]       通道1有未读的新转换数据
[1]       通道2有未读的新转换数据
[0]       通道3有未读的新转换数据
*/
#define LDC1614_STATUS_REG                          0x18
/**
[15:5]   RESV
[4]      使能幅度错误报告到 INTB 引脚和状态寄存器
[3]      使能看门狗超时错误报告
[2]      使能量程错误报告
[1]      使能欠量程错误报告
[0]      使能数据就绪中断（转换完成时 INTB 引脚输出低脉冲）
*/
#define LDC1614_ERROR_CONFIG_REG                    0x19

/**
LDC1614_CONFIG_REG
[15:14]  ACTIVE_CHAN - 0/1/2/3    当LDC1614_MUX_CONFIG_REG.rr_sequence为0时，ACTIVE_CHAN有效
[13]     SLEEP_MODE_EN  - 1 - 休眠模式使能：1=SLEEP，0=ACTIVE
[12]    RP_OVERRIDE_EN
[11]    传感器激活模式   - 1  - 0-电流激活  1- 低功耗模式
[10]    AUTO_AMP_DIS   0  自动传感器幅度校准使能：0=使能 1=禁止
[9]     REF_CLK_SRC  - 0  - 内部振荡器使能：0=使用内部振荡器 1=使用外部 CLKIN
[8]    RESV
[7]    INTB_DIS           0-当有数据时 INTB引脚拉低 1-INTB引脚保持高电平
[6]    HIGH_CURRENT_DRV  是否需要使用高电流驱动所有通道（>1.5mA）
[5:0]  RESV
*/
#define LDC1614_CONFIG_REG                          0x1A
/**
LDC1614_MUX_CONFIG_REG
[15] 自动扫描使能 0-自动扫描单通道(config.active_chan) 1-自动扫描多通道 (mux_config.rr_sequence)
[14:13] 自动扫描配置(rr_sequence)
[12:3] RESV
[2:0] 去毛刺过滤器带宽  1-1Mhz/8-3.3Mhz/9-10Mhz/11-33Mhz
*/
#define LDC1614_MUX_CONFIG_REG                      0x1B
// 写入1触发软件复位，所有寄存器恢复默认值。读取时始终返回0
#define LDC1614_RESET_DEV_REG                       0x1C
// 固定为0x5449（'TI'），可用于I2C通信自检
#define LDC1614_MANUFACTURER_ID_REG                 0x7E
// 固定为0x3055，可用于确认芯片型号是LDC1614
#define LDC1614_DEVICE_ID_REG                       0x7F

#define LDC1614_CONFIG_REG_DEFAULT                  0x011e     // 0x1e01 内部振荡器使能，正常转换模式
#define LDC1614_CHANNEL_RCOUNT_DEFAULT              0xFFFF
#define LDC1614_CHANNEL_OFFSET_DEFAULT              0x0000
#define LDC1614_CHANNEL_SETTLECOUNT_DEFAULT         0x0004
#define LDC1614_CHANNEL_CLOCK_DIVIDERS_DEFAULT      0x0110
#define LDC1614_CHANNEL_DRIVE_CURRENT_DEFAULT       0x0088
#define LDC1614_RESET_DEV_REG_DEFAULT               0x0000
#define LDC1614_MUX_CONFIG_REG_DEFAULT              0x0c82 // 自动扫描多通道，3.3Mhz带宽


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
    /// 超时计数
    uint8_t timeoutCount;
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

static int ldc1614Config(coilManage_t *coil);
static void ldc1614_debug(coilManage_t *coil);
static int ldc1614WriteRegister16(coilManage_t *coil, uint8_t reg, uint16_t data);
static int ldc1614ReadChannelData(coilManage_t *coil, uint8_t channel);
static int ldc1614ReadRegister16(coilManage_t *coil, uint8_t reg, uint16_t *data);

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

    ilog("coilManageNew success");
    return m;
}

static int ldc1614Config(coilManage_t *coil)
{
    uint16_t device_id = 0;
    int ret;
    ret = ldc1614ReadRegister16(coil, LDC1614_DEVICE_ID_REG, &device_id);
    if (ret != RET_SUCCESS) {
        // elog("LDC1614 device ID read failed, bus=%d, address=0x%x, reg=0x%x", coil->bus, coil->config->address, LDC1614_DEVICE_ID_REG);
        return RET_FAILED;
    }

    for(int i = 0; i < 4; i++) {
        ret = ldc1614WriteRegister16(coil, LDC1614_CHANNEL_RCOUNT_REG(i), LDC1614_CHANNEL_RCOUNT_DEFAULT);
        if (ret != RET_SUCCESS) {
            return RET_FAILED;
        }
        ret = ldc1614WriteRegister16(coil, LDC1614_CHANNEL_OFFSET_REG(i), LDC1614_CHANNEL_OFFSET_DEFAULT);
        if (ret != RET_SUCCESS) {
            return RET_FAILED;
        }
        ret = ldc1614WriteRegister16(coil, LDC1614_CHANNEL_SETTLECOUNT_REG(i), LDC1614_CHANNEL_SETTLECOUNT_DEFAULT);
        if (ret != RET_SUCCESS) {
            return RET_FAILED;
        }
        ret = ldc1614WriteRegister16(coil, LDC1614_CHANNEL_CLOCK_DIVIDERS_REG(i), LDC1614_CHANNEL_CLOCK_DIVIDERS_DEFAULT);
        if (ret != RET_SUCCESS) {
            return RET_FAILED;
        }
        ret = ldc1614WriteRegister16(coil, LDC1614_CHANNEL_DRIVE_CURRENT_REG(i), LDC1614_CHANNEL_DRIVE_CURRENT_DEFAULT);
        if (ret != RET_SUCCESS) {
            return RET_FAILED;
        }
    }

    ret = ldc1614WriteRegister16(coil, LDC1614_CONFIG_REG, LDC1614_CONFIG_REG_DEFAULT);
    if (ret != RET_SUCCESS) {
        return RET_FAILED;
    }
    ret = ldc1614WriteRegister16(coil, LDC1614_MUX_CONFIG_REG, LDC1614_MUX_CONFIG_REG_DEFAULT);
    if (ret != RET_SUCCESS) {
        return RET_FAILED;
    }
    ret = ldc1614WriteRegister16(coil, LDC1614_RESET_DEV_REG, LDC1614_RESET_DEV_REG_DEFAULT);
    if (ret != RET_SUCCESS) {
        return RET_FAILED;
    }

    vTaskDelay(pdMS_TO_TICKS(10));  // 等待芯片稳定
    return RET_SUCCESS;
}



static int ldc1614ReadRegister16(coilManage_t *coil, uint8_t reg, uint16_t *data)
{
    uint8_t readBuffer[2] = {0};
    int ret = 0;
    ret = i2cReadByteData(coil->bus, coil->config->address, reg, readBuffer, 2);
    if (ret != RET_SUCCESS) {
        elog("read register[0x%02X] failed", reg);
        return RET_FAILED;
    }
    *data = (readBuffer[0] << 8) | readBuffer[1];
    return RET_SUCCESS;
}
/**
 * @brief 读取通道28位数据 (两次独立I2C读：先读MSB触发锁存，再读LSB)
 * @param coil 线圈管理器
 * @param channel 通道号
 * @param data 数据
 * @return RET_SUCCESS 成功
 * @return RET_FAILED 失败
 */
static int ldc1614ReadChannelData(coilManage_t *coil, uint8_t channel)
{
    uint16_t dataMsb = 0;
    uint16_t dataLsb = 0;
    uint16_t errorMask = 0;
    int ret;
    ret = ldc1614ReadRegister16(coil, LDC1614_CHANNEL_MSB_REG(channel), &dataMsb);
    if (ret != RET_SUCCESS) {
        elog("read channle[%d] msb data failed", channel);
        return RET_FAILED;
    }
    errorMask = dataMsb & 0xF000;
    if (errorMask) {
        errorMask = errorMask >> 12;
        errorMask = errorMask & 0x000F;
        errorMask = errorMask << (channel *4);
        coil->state.runStateMask |= errorMask;
        elog("channle[%d] error flags: 0x%04X", channel, errorMask);
        return RET_FAILED;
    }
    ret = ldc1614ReadRegister16(coil, LDC1614_CHANNEL_LSB_REG(channel), &dataLsb);
    if (ret != RET_SUCCESS) {
        elog("read channle[%d] lsb data failed", channel);
        return RET_FAILED;
    }

    coil->state.data[channel] = ((uint32_t)(dataMsb & 0x0FFF) << 16) | dataLsb;
    dlog("channle[%d] data: 0x%08X (%u)", channel, (unsigned int)coil->state.data[channel], (unsigned int)coil->state.data[channel]);
    return RET_SUCCESS;
}

static int ldc1614WriteRegister16(coilManage_t *coil, uint8_t reg, uint16_t data)
{
    int ret = 0;
    ret = i2cWriteByteData(coil->bus, coil->config->address, reg, (uint8_t *)&data, 2);
    if (ret != RET_SUCCESS) {
        elog("LDC1614 write register failed, bus=%d, address=0x%x, reg=0x%x", coil->bus, coil->config->address, reg);
        return RET_FAILED;
    }
    return RET_SUCCESS;
}

/**
 * @brief 调试函数：读取并打印所有关键寄存器
 */
static void ldc1614_debug(coilManage_t *coil)
{
    int ret;
    uint16_t data;
    ilog("========== LDC1614 Debug ==========");
    for (int i = 0x08; i < 0x22; i++) {
        ret = ldc1614ReadRegister16(coil, i, &data);
        if (ret != RET_SUCCESS) {
            elog("read register[0x%02X] failed", i);
        }
        ilog("REG 0x%02X: 0x%04X", i, data);
    }

    // 1. 检查设备 ID
    ldc1614ReadRegister16(coil, 0x7F, &data);
    ilog("Device ID:     0x%04X %s", data, (data == 0x3055) ? "[OK]" : "[ERROR]");

    // 2. 读取配置寄存器
    ldc1614ReadRegister16(coil, 0x1A, &data);
    ilog("CONFIG:        0x%04X", data);
    ilog("  - Power mode: %s", (data & (1 << 13)) ? "SHUTDOWN" : "ACTIVE [OK]");
    ilog("  - Conv mode:  %s", ((data >> 10) & 0x03) == 2 ? "Continuous [OK]" : "Other");

    // 6. 检查其他关键寄存器
    ldc1614ReadRegister16(coil, 0x08, &data);
    ilog("RCOUNT_CH0:    0x%04X", data);
    ldc1614ReadRegister16(coil, 0x09, &data);
    ilog("RCOUNT_CH1:    0x%04X", data);
    ldc1614ReadRegister16(coil, 0x0A, &data);
    ilog("RCOUNT_CH2:    0x%04X", data);
    ldc1614ReadRegister16(coil, 0x0B, &data);
    ilog("RCOUNT_CH4:    0x%04X", data);

    ldc1614ReadRegister16(coil, 0x10, &data);
    ilog("SETTLECOUNT0:    0x%04X", data);
    ldc1614ReadRegister16(coil, 0x11, &data);
    ilog("SETTLECOUNT1:    0x%04X", data);
    ldc1614ReadRegister16(coil, 0x12, &data);
    ilog("SETTLECOUNT2:    0x%04X", data);
    ldc1614ReadRegister16(coil, 0x13, &data);
    ilog("SETTLECOUNT3:    0x%04X", data);

    ldc1614ReadRegister16(coil, 0x1E, &data);
    ilog("DRIVE_CURRENT0:    0x%04X", data);
    ldc1614ReadRegister16(coil, 0x1F, &data);
    ilog("DRIVE_CURRENT1:    0x%04X", data);
    ldc1614ReadRegister16(coil, 0x20, &data);
    ilog("DRIVE_CURRENT2:    0x%04X", data);
    ldc1614ReadRegister16(coil, 0x21, &data);
    ilog("DRIVE_CURRENT3:    0x%04X", data);

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
    if (!coil->state.ready || !coil->state.connected) {
        ret = ldc1614Config(coil);
        if (ret != RET_SUCCESS) {
            coil->timeoutCount++;
            if (coil->timeoutCount < 3) {
                elog("LDC1614 configure failed, bus=%d, address=0x%x", coil->bus, coil->config->address);
            }
            return;
        }
        coil->state.ready = true;
        coil->state.connected = true;
        coil->timeoutCount = 0;
        ilog("LDC1614 configure success, bus=%d, address=0x%x", coil->bus, coil->config->address);
        ldc1614_debug(coil);
    }

    for(int i = 0; i < 4; i++) {
        if (coil->config->channelEna & (1 << i)) {
            ret = ldc1614ReadChannelData(coil, i);
            if (ret != RET_SUCCESS) {
                elog("LDC1614 read channel data failed, bus=%d, address=0x%x, channel=%d", coil->bus, coil->config->address, i);
                coil->timeoutCount++;
                continue;
            }
            coil->timeoutCount = 0;
        }
    }

    if (coil->timeoutCount >= 3) {
        coil->state.connected = false;
        coil->state.ready = false;
        coil->timeoutCount = 0;
    }
}