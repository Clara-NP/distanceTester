#ifndef __COIL_H__
#define __COIL_H__


#include <common/generic.h>

typedef struct coilManage coilManage_t;

/**
 * @brief 磁感应线圈配置
 *
 * @note P0的级别最高，依次递减，P0的采样周期尽量短，可能包含一些紧急参数
 */
typedef struct
{
    /// @brief 地址
    uint8_t address;
    /// @brief 通道使能，bit0-3为通道0-3的使能，其他保留
    uint8_t channelEna;
    /// @brief SETTLECOUNT，tS = settleCount × 16 / fREF；0 则用 0x0400
    uint16_t settleCount;
    /// @brief IDRIVE 0–31，写入 DRIVE_CURRENT[15:11]
    uint8_t idrive;
    /// @brief 0=内部振荡器，1=外部 CLKIN（本板未确认晶振时必须为 0）
    uint8_t refClkExt;
}coilConfig_t;

typedef enum {
    COIL_CHANNEL_ENABLE_0 = 1,
    COIL_CHANNEL_ENABLE_1 = 2,
    COIL_CHANNEL_ENABLE_2 = 4,
    COIL_CHANNEL_ENABLE_3 = 8,
}coilChannelEnable_t;

/**
 * @brief 磁感应线圈运行状态
 */
typedef struct
{
    /// @brief 连接状态
    bool connected;
    /// @brief 是否已经准备好
    bool ready;
    /// @brief 满 N 次有效转换后的通道数据；未满时保持上次有效值
    uint32_t data[4];
    /// @brief 1=本笔 data[] 是新的平均值
    bool valid;
    /// @brief 本窗 N 点峰峰值（max-min），换钟后不作绝对门限
    uint32_t sampleSpan;
    /// @brief 通道数据更新时间
    sysTick_t updateTime;
    /// @brief 运行时间,秒
    uint32_t runTime;
    /// @brief 错误码（每通道 4bit：UR/OR/WD/AE）
    uint32_t runStateMask;
}coilManageState_t;


/**
 * @brief 创建一个磁感应线圈管理器
 *
 * @param bus
 * @param name
 * @param config
 * @return coilManage_t *
 */
coilManage_t *coilManageNew(uint8_t bus, const char *name, const coilConfig_t *config);

/**
 * @brief coil Schedule
 *
 * @param coil
 */
void coilManageSchedule(coilManage_t *coil);

/**
 * @brief 获取磁感应线圈运行状态
 *
 * @param coil
 * @return const coilManageState_t*
 */
const coilManageState_t *getCoilState(coilManage_t *coil);

/**
 * @brief 上报侧已取走本笔有效样本
 */
void coilStateConsume(coilManage_t *coil);

#endif // __COIL_H__
