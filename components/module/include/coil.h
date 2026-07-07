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
    /// @brief 探测周期，ms
    uint16_t detectionPeriod;
    /// @brief P0级别的采样周期，ms
    uint16_t samplePeriodP0;
    /// @brief P1级别的采样周期，ms
    uint16_t samplePeriodP1;
    /// @brief P2级别的采样周期，ms
    uint16_t samplePeriodP2;

    /// @brief 地址
    uint8_t address;
    /// @brief 电压互感器
    uint16_t pt;
    /// @brief 电流互感器
    uint16_t ct;
}coilConfig_t;

/**
 * @brief 空调运行状态
 * 
 * 
 */
typedef struct
{
    /// @brief 连接状态
    bool connected;
    /// @brief 是否已经准备好
    bool ready;

    // /// @brief 运行时间,秒
    // uint32_t runTime;
    // /// @brief 运行状态掩码,AC_RUN_STATE_MASK,P0
    // uint32_t runStateMask;

    // /// @brief 柜内温度，°C，P0
    // float internalTemperature;
    // /// @brief 柜外温度，°C，P0
    // float temperature;
    // /// @brief 柜外湿度，%，P0
    // float humidity;
    // /// @brief 蒸发器温度，°C，P2
    // float evaporateTemperature;
    // /// @brief 冷凝器温度，°C，P2
    // float condensorTemperature;
    // /// @brief 内风机转速，转/分钟，P2
    // float internalFanSpeed;
    // /// @brief 外风机转速，转/分钟，P2
    // float externalFanSpeed;

    // /// @brief 厂商相关状态码，P0
    // uint32_t manufacturerState;

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

#endif // __COIL_H__