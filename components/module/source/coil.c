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


#define LDC1614_CHANNEL_0   0
#define LDC1614_CHANNEL_1   1
#define LDC1614_CHANNEL_2   2
#define LDC1614_CHANNEL_3   3

#define COIL_AVG_N          4

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
[15:12] FIN_DIVIDER0 - 传感器输入分频：fIN0 = fSENSOR0 / FIN_DIVIDER0，最小为 1
[11:10] RESERVED
[9:0]  FREF_DIVIDER0   fREF0 = fCLK / FREF_DIVIDER0
单通道 fREF 上限 35 MHz，内部钟约 43 MHz 时 FREF_DIV 至少为 2
*/
#define LDC1614_CHANNEL_CLOCK_DIVIDERS_REG(x)       (0x14 + x)  // 14 15 16 17
#define LDC1614_CHANNEL_DRIVE_CURRENT_REG(x)        (0x1E + x) // 1e 1f 20 21

#define LDC1614_STATUS_REG                          0x18
#define LDC1614_ERROR_CONFIG_REG                    0x19
#define LDC1614_CONFIG_REG                          0x1A
#define LDC1614_MUX_CONFIG_REG                      0x1B
#define LDC1614_RESET_DEV_REG                       0x1C
#define LDC1614_MANUFACTURER_ID_REG                 0x7E
#define LDC1614_DEVICE_ID_REG                       0x7F

#define LDC1614_CHANNEL_RCOUNT_DEFAULT              0xFFFF
#define LDC1614_CHANNEL_OFFSET_DEFAULT              0x0000
#define LDC1614_SETTLECOUNT_DEFAULT                 0x0400
#define LDC1614_RESET_DEV_BIT                       0x8000
#define LDC1614_MUX_RESERVED                        0x0200
#define LDC1614_DEGLITCH_1MHZ                       0x0001
#define LDC1614_ERROR_CONFIG_DEFAULT                0x001E
#define LDC1614_IDRIVE_MAX                          31
#define LDC1614_DEVICE_ID_VALUE                     0x3055

/**
 * @brief 定义一个磁感应线圈管理器
 */
struct coilManage
{
    uint8_t bus;
    uint8_t timeoutCount;
    const coilConfig_t *config;
    coilManageState_t state;
    uint32_t avgSample[4][COIL_AVG_N];
    uint8_t avgN[4];
};

static int ldc1614Config(coilManage_t *coil);
static void ldc1614_debug(coilManage_t *coil);
static int ldc1614WriteRegister16(coilManage_t *coil, uint8_t reg, uint16_t data);
static int ldc1614ReadChannelData(coilManage_t *coil, uint8_t channel, uint32_t *outData);
static int ldc1614ReadRegister16(coilManage_t *coil, uint8_t reg, uint16_t *data);
static int ldc1614FirstEnabledChannel(uint8_t channelEna);
static int ldc1614EnabledChannelCount(uint8_t channelEna);
static uint16_t ldc1614BuildMuxConfig(uint8_t channelEna);
static uint16_t ldc1614BuildConfigReg(const coilConfig_t *cfg);
static uint16_t ldc1614BuildClockDividers(uint8_t channelEna);
static void coilResetAverage(coilManage_t *coil);

coilManage_t *coilManageNew(uint8_t bus, const char *name, const coilConfig_t *config)
{
    coilManage_t *m;

    if (!config) {
        elog("coilManageNew failed, config is null");
        return NULL;
    }
    if (ldc1614FirstEnabledChannel(config->channelEna) < 0) {
        elog("coilManageNew failed, channelEna=0x%x", config->channelEna);
        return NULL;
    }

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

static int ldc1614FirstEnabledChannel(uint8_t channelEna)
{
    for (int i = 0; i < 4; i++) {
        if (channelEna & (1 << i)) {
            return i;
        }
    }
    return -1;
}

static int ldc1614EnabledChannelCount(uint8_t channelEna)
{
    int n = 0;
    for (int i = 0; i < 4; i++) {
        if (channelEna & (1 << i)) {
            n++;
        }
    }
    return n;
}

static uint16_t ldc1614BuildMuxConfig(uint8_t channelEna)
{
    int n = ldc1614EnabledChannelCount(channelEna);
    int last = -1;

    for (int i = 0; i < 4; i++) {
        if (channelEna & (1 << i)) {
            last = i;
        }
    }

    if (n <= 1) {
        return LDC1614_MUX_RESERVED | LDC1614_DEGLITCH_1MHZ;
    }

    uint16_t seq = (last <= 1) ? 0 : (last == 2) ? 1 : 2;
    return (1u << 15) | (seq << 13) | LDC1614_MUX_RESERVED | LDC1614_DEGLITCH_1MHZ;
}

static uint16_t ldc1614BuildClockDividers(uint8_t channelEna)
{
    uint16_t finDiv = 1;
    uint16_t frefDiv = (ldc1614EnabledChannelCount(channelEna) <= 1) ? 2 : 1;

    return (uint16_t)((finDiv & 0xF) << 12) | (frefDiv & 0x3FF);
}

static uint16_t ldc1614BuildConfigReg(const coilConfig_t *cfg)
{
    int active = ldc1614FirstEnabledChannel(cfg->channelEna);
    uint16_t v = 0;

    if (active < 0) {
        active = 0;
    }

    v |= (uint16_t)((active & 0x3) << 14);
    v |= (1u << 12);              // RP_OVERRIDE_EN：用手写 IDRIVE
    // SENSOR_ACTIVATE_SEL=0：全电流起振
    v |= (1u << 10);              // AUTO_AMP_DIS
    if (cfg->refClkExt) {
        v |= (1u << 9);           // 外部 CLKIN
    }
    v |= 1u;                      // datasheet：reserved [5:0] 写 000001
    return v;
}

static uint32_t coilMedianU32(uint32_t *v, uint8_t n)
{
    for (uint8_t i = 1; i < n; i++) {
        uint32_t x = v[i];
        int j = (int)i - 1;
        while (j >= 0 && v[j] > x) {
            v[j + 1] = v[j];
            j--;
        }
        v[j + 1] = x;
    }
    if (n & 1) {
        return v[n / 2];
    }
    return (uint32_t)(((uint64_t)v[n / 2 - 1] + v[n / 2]) / 2);
}

static void coilResetAverage(coilManage_t *coil)
{
    osMemset(coil->avgSample, 0, sizeof(coil->avgSample));
    osMemset(coil->avgN, 0, sizeof(coil->avgN));
    coil->state.valid = false;
    coil->state.sampleSpan = 0;
}

static int ldc1614Config(coilManage_t *coil)
{
    uint16_t device_id = 0;
    int ret;
    const coilConfig_t *cfg = coil->config;
    int ch = ldc1614FirstEnabledChannel(cfg->channelEna);

    ret = ldc1614ReadRegister16(coil, LDC1614_DEVICE_ID_REG, &device_id);
    if (ret != RET_SUCCESS) {
        return RET_FAILED;
    }
    if (device_id != LDC1614_DEVICE_ID_VALUE) {
        elog("LDC1614 unexpected device id 0x%04X", device_id);
        return RET_FAILED;
    }

    ret = ldc1614WriteRegister16(coil, LDC1614_RESET_DEV_REG, LDC1614_RESET_DEV_BIT);
    if (ret != RET_SUCCESS) {
        return RET_FAILED;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t idrive = cfg->idrive;
    if (idrive > LDC1614_IDRIVE_MAX) {
        elog("LDC1614 idrive=%u out of range", (unsigned)idrive);
        return RET_FAILED;
    }
    uint16_t drive = (uint16_t)idrive << 11;
    uint16_t settle = cfg->settleCount ? cfg->settleCount : LDC1614_SETTLECOUNT_DEFAULT;
    if (settle < 2) {
        elog("LDC1614 settleCount=%u too small", (unsigned)settle);
        return RET_FAILED;
    }
    uint16_t clockDiv = ldc1614BuildClockDividers(cfg->channelEna);

    for (int i = 0; i < 4; i++) {
        if ((cfg->channelEna & (1 << i)) == 0) {
            continue;
        }
        ret = ldc1614WriteRegister16(coil, LDC1614_CHANNEL_RCOUNT_REG(i), LDC1614_CHANNEL_RCOUNT_DEFAULT);
        if (ret != RET_SUCCESS) {
            return RET_FAILED;
        }
        ret = ldc1614WriteRegister16(coil, LDC1614_CHANNEL_OFFSET_REG(i), LDC1614_CHANNEL_OFFSET_DEFAULT);
        if (ret != RET_SUCCESS) {
            return RET_FAILED;
        }
        ret = ldc1614WriteRegister16(coil, LDC1614_CHANNEL_SETTLECOUNT_REG(i), settle);
        if (ret != RET_SUCCESS) {
            return RET_FAILED;
        }
        ret = ldc1614WriteRegister16(coil, LDC1614_CHANNEL_CLOCK_DIVIDERS_REG(i), clockDiv);
        if (ret != RET_SUCCESS) {
            return RET_FAILED;
        }
        ret = ldc1614WriteRegister16(coil, LDC1614_CHANNEL_DRIVE_CURRENT_REG(i), drive);
        if (ret != RET_SUCCESS) {
            return RET_FAILED;
        }
    }

    ret = ldc1614WriteRegister16(coil, LDC1614_ERROR_CONFIG_REG, LDC1614_ERROR_CONFIG_DEFAULT);
    if (ret != RET_SUCCESS) {
        return RET_FAILED;
    }

    uint16_t mux = ldc1614BuildMuxConfig(cfg->channelEna);
    uint16_t configReg = ldc1614BuildConfigReg(cfg);
    ret = ldc1614WriteRegister16(coil, LDC1614_MUX_CONFIG_REG, mux);
    if (ret != RET_SUCCESS) {
        return RET_FAILED;
    }
    ret = ldc1614WriteRegister16(coil, LDC1614_CONFIG_REG, configReg);
    if (ret != RET_SUCCESS) {
        return RET_FAILED;
    }

    ilog("LDC1614 CONFIG=0x%04X MUX=0x%04X SETTLE=0x%04X DRIVE=0x%04X CLKDIV=0x%04X ch=%d clk=%s avgN=%d",
         configReg, mux, settle, drive, clockDiv, ch, cfg->refClkExt ? "CLKIN" : "internal", COIL_AVG_N);

    vTaskDelay(pdMS_TO_TICKS(10));
    return RET_SUCCESS;
}

static int ldc1614ReadRegister16(coilManage_t *coil, uint8_t reg, uint16_t *data)
{
    uint8_t readBuffer[2] = {0};
    int ret = 0;
    ret = i2cReadByteData(coil->bus, coil->config->address, reg, readBuffer, 2);
    if (ret != RET_SUCCESS) {
        return RET_FAILED;
    }
    *data = (readBuffer[0] << 8) | readBuffer[1];
    return RET_SUCCESS;
}

/**
 * @brief 读取通道28位数据 (两次独立I2C读：先读MSB触发锁存，再读LSB)
 */
static int ldc1614ReadChannelData(coilManage_t *coil, uint8_t channel, uint32_t *outData)
{
    uint16_t dataMsb = 0;
    uint16_t dataLsb = 0;
    uint16_t errorBits = 0;
    int ret;

    ret = ldc1614ReadRegister16(coil, LDC1614_CHANNEL_MSB_REG(channel), &dataMsb);
    if (ret != RET_SUCCESS) {
        elog("read channel[%d] msb data failed", channel);
        return RET_FAILED;
    }
    errorBits = (dataMsb >> 12) & 0x000F;
    if (errorBits) {
        coil->state.runStateMask |= ((uint32_t)errorBits << (channel * 4));
        elog("channel[%d] error flags: 0x%X", channel, errorBits);
        return RET_FAILED;
    }
    ret = ldc1614ReadRegister16(coil, LDC1614_CHANNEL_LSB_REG(channel), &dataLsb);
    if (ret != RET_SUCCESS) {
        elog("read channel[%d] lsb data failed", channel);
        return RET_FAILED;
    }

    *outData = ((uint32_t)(dataMsb & 0x0FFF) << 16) | dataLsb;
    coil->state.runStateMask &= ~(0xFu << (channel * 4));
    dlog("channel[%d] data: 0x%08X (%u)", channel, (unsigned int)*outData, (unsigned int)*outData);
    return RET_SUCCESS;
}

static int ldc1614WriteRegister16(coilManage_t *coil, uint8_t reg, uint16_t data)
{
    uint8_t buf[2];
    buf[0] = (uint8_t)(data >> 8);
    buf[1] = (uint8_t)(data & 0xFF);
    int ret = i2cWriteByteData(coil->bus, coil->config->address, reg, buf, 2);
    if (ret != RET_SUCCESS) {
        elog("LDC1614 write register failed, bus=%d, address=0x%x, reg=0x%x", coil->bus, coil->config->address, reg);
        return RET_FAILED;
    }
    return RET_SUCCESS;
}

static void ldc1614_debug(coilManage_t *coil)
{
    int ret;
    uint16_t data;
    ilog("========== LDC1614 Debug ==========");
    for (int i = 0x08; i < 0x22; i++) {
        ret = ldc1614ReadRegister16(coil, i, &data);
        if (ret != RET_SUCCESS) {
            elog("read register[0x%02X] failed", i);
            continue;
        }
        ilog("REG 0x%02X: 0x%04X", i, data);
    }

    ldc1614ReadRegister16(coil, 0x7F, &data);
    ilog("Device ID:     0x%04X %s", data, (data == LDC1614_DEVICE_ID_VALUE) ? "[OK]" : "[ERROR]");

    ldc1614ReadRegister16(coil, 0x1A, &data);
    ilog("CONFIG:        0x%04X", data);
    ilog("  - Power mode: %s", (data & (1 << 13)) ? "SLEEP" : "ACTIVE [OK]");
    ilog("  - ACTIVE_CHAN: %u", (unsigned)((data >> 14) & 0x3));
    ilog("  - REF_CLK:     %s", (data & (1 << 9)) ? "CLKIN [WARN: board clock unverified]" : "internal [OK]");
    ilog("  - ACTIVATE:    %s", (data & (1 << 11)) ? "low-power" : "full-current");

    ldc1614ReadRegister16(coil, 0x08, &data);
    ilog("RCOUNT_CH0:    0x%04X", data);
    ldc1614ReadRegister16(coil, 0x09, &data);
    ilog("RCOUNT_CH1:    0x%04X", data);

    ldc1614ReadRegister16(coil, 0x10, &data);
    ilog("SETTLECOUNT0:    0x%04X", data);
    ldc1614ReadRegister16(coil, 0x11, &data);
    ilog("SETTLECOUNT1:    0x%04X", data);

    ldc1614ReadRegister16(coil, 0x1E, &data);
    ilog("DRIVE_CURRENT0:    0x%04X", data);
    ldc1614ReadRegister16(coil, 0x1F, &data);
    ilog("DRIVE_CURRENT1:    0x%04X", data);

    ilog("====================================");
}

void coilManageSchedule(coilManage_t *coil)
{
    int ret = 0;

    if (!coil || !coil->config) {
        return;
    }

    if (!coil->state.ready || !coil->state.connected) {
        ret = ldc1614Config(coil);
        if (ret != RET_SUCCESS) {
            if (coil->timeoutCount < 3) {
                coil->timeoutCount++;
                elog("LDC1614 configure failed, bus=%d, address=0x%x", coil->bus, coil->config->address);
            }
            return;
        }
        coil->state.ready = true;
        coil->state.connected = true;
        coil->timeoutCount = 0;
        coilResetAverage(coil);
        ilog("LDC1614 configure success, bus=%d, address=0x%x", coil->bus, coil->config->address);
        ldc1614_debug(coil);
    }

    uint16_t status = 0;
    ret = ldc1614ReadRegister16(coil, LDC1614_STATUS_REG, &status);
    if (ret != RET_SUCCESS) {
        coil->timeoutCount++;
        elog("LDC1614 status read failed, bus=%d, address=0x%x", coil->bus, coil->config->address);
    } else {
        dlog("status register: 0x%04X", status);
        coil->state.valid = false;
        for (int i = 0; i < 4; i++) {
            if ((coil->config->channelEna & (1 << i)) == 0) {
                continue;
            }
            if ((status & (1 << (3 - i))) == 0) {
                continue;
            }
            uint32_t sample = 0;
            ret = ldc1614ReadChannelData(coil, i, &sample);
            if (ret != RET_SUCCESS) {
                elog("LDC1614 read channel data failed, bus=%d, address=0x%x, channel=%d",
                     coil->bus, coil->config->address, i);
                coil->timeoutCount++;
                coil->avgN[i] = 0;
                continue;
            }
            coil->timeoutCount = 0;
            coil->avgSample[i][coil->avgN[i]] = sample;
            coil->avgN[i]++;
            if (coil->avgN[i] >= COIL_AVG_N) {
                uint32_t sorted[COIL_AVG_N];
                for (uint8_t k = 0; k < COIL_AVG_N; k++) {
                    sorted[k] = coil->avgSample[i][k];
                }
                coil->state.data[i] = coilMedianU32(sorted, COIL_AVG_N);
                coil->state.sampleSpan = sorted[COIL_AVG_N - 1] - sorted[0];
                coil->state.valid = true;
                coil->state.updateTime = upTime();
                coil->avgN[i] = 0;
                dlog("channel[%d] med: %u span: %u", i,
                     (unsigned int)coil->state.data[i], (unsigned int)coil->state.sampleSpan);
            }
        }
    }

    if (coil->timeoutCount >= 3) {
        coil->state.connected = false;
        coil->state.ready = false;
        coil->timeoutCount = 0;
        coilResetAverage(coil);
        elog("LDC1614 offline, bus=%d, address=0x%x", coil->bus, coil->config->address);
    }
}

const coilManageState_t *getCoilState(coilManage_t *coil)
{
    if (!coil) {
        return NULL;
    }
    return &coil->state;
}

void coilStateConsume(coilManage_t *coil)
{
    if (!coil) {
        return;
    }
    coil->state.updateTime = 0;
    coil->state.valid = false;
}
