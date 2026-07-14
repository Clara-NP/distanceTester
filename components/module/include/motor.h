#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "common/generic.h"

/**
 * @brief 电机实例定义
 * 
 */
typedef struct motorController motorController_t;

#ifndef CONFIG_MOTOR_PROCESS_TIME
#define CONFIG_MOTOR_PROCESS_TIME   50
#endif
#ifndef CONFIG_MOTOR_SEND_TIME
#define CONFIG_MOTOR_SEND_TIME   1000
#endif

/**
 * @brief 电机配置
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

    // bool enable;
}motorConfig_t;


typedef struct
{
    /// @brief 连接状态
    bool connected;
    /// @brief 是否已经准备好，可以启动电源输出
    bool ready;

    /// @brief 运行状态，P0
    uint32_t runState;
    /// @brief 运行时间,秒
    uint32_t runTime;

    /// @brief 厂商相关状态码，P0
    uint32_t manufacturerState;

    /// @brief DJI反馈的真实转速
    int16_t actualSpeed;
    /// @brief DJI反馈的实际扭矩
    int16_t actualTorque;
    /// @brief DJI返回的机械角度
    int16_t actualAngle;
    /// @brief 真实转速更新时间
    sysTick_t dataUpdateTime;
}motorState_t;

/**
 * @brief 创建一个电机控制器
 * 
 * @param can 
 * @param name 
 * @param config 
 * @return motorController_t * 
 */
motorController_t *motorNew(uint8_t can, const char *name, const motorConfig_t *config);

/**
 * @brief 调度电机
 * 
 * @param motor 
 */
void motorSchedule(motorController_t *motor);
void motorSetConfig(motorController_t *motor, bool isChange, int speedLevel);
int motorGetSpeedLevel(motorController_t *motor);
bool motorGetEnable(motorController_t *motor);
const motorState_t *getMotorState(motorController_t *motor);
#endif /* __MOTOR_H__ */
