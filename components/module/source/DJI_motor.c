#include "common/generic.h"
#include "can/can.h"
#include "motor.h"

#define TRACE_TAG "DJI-motor"
#define TRACE_LEVEL T_INFO
#define TRACE_ENABLE

#include <common/trace.h>



/**
 * @brief 接收帧缓存条例大小
 * 
 */
#ifndef CONFIG_POWER_RECEIVE_BUFFER_ITEM_SIZE
#define CONFIG_POWER_RECEIVE_BUFFER_ITEM_SIZE   1
#endif


typedef struct __PACKED {
    uint8_t angleH;
    uint8_t angleL;
    uint8_t speedH;
    uint8_t speedL;
    uint8_t torqueH;
    uint8_t torqueL;
    uint8_t resv1;
    uint8_t resv2;
}canDataFrame_t;

/**
 * @brief can接收帧管理
 * 
 */
typedef struct {
    /// @brief 是否有效
    uint8_t valid;    
    /// @brief can ID
    uint32_t canId;    
    /// @brief 帧数据
    canDataFrame_t frame;
    /// @brief 超时时间，未处理将被丢掉
    sysTick_t expired;
}receiveFrame_t;

/**
 * @brief 定义一个DJI电机控制器
 * 
 */
struct motorController
{
    /// 逻辑ID
    //uint8_t id;
    /// CAN ID
    uint8_t can;
    /// 地址，0为广播地址，自动探测发现地址后更新地址，离线后地址变成广播地址
    uint8_t address;
    // /// 配置
    // const powerConfig_t *config;
    /// 超时计数
    uint8_t timeoutCount;

    // /// 电流是否配置完成（华为电源每次打开，都要设置输出电流，并且只有在电压有输出的情况下设置输出电流才有效）
    // uint8_t outputCurrentReady;
    // /// 设置(输出电流)尝试次数
    // uint8_t setConfigAttempts;

    /// 接收缓存
    receiveFrame_t receive[CONFIG_POWER_RECEIVE_BUFFER_ITEM_SIZE];

    // /// power信息 硬件相关(软硬件型号/厂商/序列号...) 暂时不需要
    // powerInfo_t info;
    /// motor状态
    motorState_t state;

    // /// 下次探测时间
    // sysTick_t expiredDetection;
    // /// 离线时间
    // sysTick_t expiredOffline;

    // 数据处理事件
    sysTick_t processTimer;
    /// 数据发送事件
    sysTick_t sendTimer;

    // /// P0级别需要获取的状态
    // uint32_t batchRequestMaskP0;
    // /// p0状态下次请求时间
    // sysTick_t expiredP0;
    // /// P1级别需要获取的状态
    // uint32_t batchRequestMaskP1;
    // /// p1状态下次请求时间
    // sysTick_t expiredP1;
    // /// P2级别需要获取的状态
    // uint32_t batchRequestMaskP2;
    // /// p2状态下次请求时间
    // sysTick_t expiredP2;
    // /// 运行时间1秒超时
    // sysTick_t expiredSecond;

    /// @brief 输出电流设定值
    int speedLevel;
    /// @brief 是否使能
    bool enable;
    // 设置的电流数据
    int outputCurrentSet;
};


const int controlData[11] = {0, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000};

#define RESET_EXPIRED_TIME(_command, _time)     _command = upTime() + (_time)
static void __loopReceive(motorController_t *motor);
static void __processData(motorController_t *motor);
static void __sendData(motorController_t *motor);

/**
 * @brief 创建一个power控制器
 * 
 * @param can 
 * @param name 
 * @return upsController_t * 
 */
motorController_t *motorNew(uint8_t can, const char *name, const motorConfig_t *config)
{
    motorController_t *m;

    // 参数判断
    if(!config) {
        return NULL;
    }

    // if(!config->detectionPeriod || !config->samplePeriodP0 || !config->samplePeriodP1 || !config->samplePeriodP2)
    // {
    //     elog("power new failed, invaild sample period");
    //     return NULL;
    // }

    // if(config->outputVoltage == 0 || config->outputCurrent == 0 || config->outputProtectVoltage == 0)
    // {
    //     elog("power new failed, invaild output setting");
    //     return NULL;
    // }


    m = (motorController_t *)osMalloc(sizeof(*m));
    if(!m)
    {
        elog("motor new failed,not memory");
        return NULL;
    }

    osMemset(m , 0, sizeof(*m));

    //m->id = id;
    m->can = can;
    // m->address = CONFIG_POWER_CAN_BROADCAST_ADDRESS;  // 不需要广播
    // m->config = config;
    // osMemcpy(m->info.name, name, strlen(name));
    // osMemcpy(m->info.manufacturer, "DJI", strlen("DJI"));

    // 重置各种时间参数
    RESET_EXPIRED_TIME(m->processTimer, CONFIG_MOTOR_PROCESS_TIME);
    RESET_EXPIRED_TIME(m->sendTimer, CONFIG_MOTOR_SEND_TIME);
    // RESET_EXPIRED_TIME(p->expiredDetection, 0);
    // RESET_EXPIRED_TIME(p->expiredOffline, CONFIG_POWER_DEFAULT_OFFLINE_TIME);
    // RESET_EXPIRED_TIME(p->expiredP0, 0);
    // RESET_EXPIRED_TIME(p->expiredP1, 0);
    // RESET_EXPIRED_TIME(p->expiredP2, 0);
    // RESET_EXPIRED_TIME(p->expiredSecond, 1000);

    /*
    // // P0
    // p->batchRequestMaskP0 = BATCH_REQUEST_STATUS_INFO | BATCH_REQUEST_OUTPUT_VOLTAGE | BATCH_REQUEST_OUTPUT_CURRENT1 | BATCH_REQUEST_OUTPUT_CURRENT2;
    // // P1
    // p->batchRequestMaskP1 =  BATCH_REQUEST_OUTPUT_POWER | BATCH_REQUEST_INPUT_POWER | BATCH_REQUEST_INPUT_FREQUENCY | BATCH_REQUEST_INPUT_CURRENT |\
    //                         BATCH_REQUEST_INPUT_VOLTAGE_UV | BATCH_REQUEST_INPUT_VOLTAGE_VW | BATCH_REQUEST_INPUT_VOLTAGE_WU | BATCH_REQUEST_INTERNAL_TEMPERATURE;
    // p->batchRequestMaskP2 =  0;
    */

    ilog("motor(%d %s) new success", can, name);

    return m;
}

void motorSchedule(motorController_t *motor)
{
    int ret;
    sysTick_t now = upTime();
    bool connected;

    if(!motor) {
        return;
    }

    // 循环接收数据
    __loopReceive(motor);

    connected = motor->state.connected;
    if (!connected) {
        return;
    }

    // 数据处理
    if (upTimeAfter(now, motor->processTimer)) {
        __processData(motor);
        motor->processTimer = upTime() + CONFIG_MOTOR_PROCESS_TIME;
    }

    if (!motor->enable) {
        return;
    }

    // 数据发送
    if (upTimeAfter(now, motor->sendTimer)) {
        __sendData(motor);
        motor->sendTimer = upTime() + CONFIG_MOTOR_SEND_TIME;
    }

}


static void __loopReceive(motorController_t *motor)
{
    int ret = RET_FAILED;
    receiveFrame_t* receive = &motor->receive[0];
    int time = 0;

    // 尽量一次将 can rx queue 中的数据全部读出
    // 同时需要避免在这里耗时太久
    while(time < 10) {
        ret = canReceiveData(motor->can, &receive->canId, (uint8_t *)&receive->frame, sizeof(receive->frame));
        if(ret > 0) {
            time++;
            continue;
        }
        break;
    }
    if (time > 0) {
        motor->timeoutCount = 0;
        receive->valid = true;
        receive->expired = upTime() + 100;   // 100ms 超时
        motor->state.connected = true;
        // ilog("canReceiveData: canId=%d, time=%d", receive->canId, time);
    } else {
        if (motor->timeoutCount > 3) {
            // connected = false;
            motor->state.connected = false;
            return;
        }
        motor->timeoutCount++;

        elog("canReceiveData: no data timeoutCount=%d", motor->timeoutCount);
    }
}


static void __processData(motorController_t *motor)
{
    // int ret = RET_FAILED;
    receiveFrame_t* receive = &motor->receive[0];

    if (!receive->valid) {
        wlog("processData: no valid data");
        return;
    }

    if (upTimeAfter(upTime(), receive->expired)) {
        receive->valid = false;
        wlog("processData: data expired, canId=%d", receive->canId);
        return;
    }

    // 解析receive数据
    motor->state.actualSpeed = (int16_t)(((uint16_t)receive->frame.speedH << 8) | receive->frame.speedL);
    motor->state.actualTorque = (int16_t)(((uint16_t)receive->frame.torqueH << 8) | receive->frame.torqueL);
    motor->state.actualAngle = (int16_t)(((uint16_t)receive->frame.angleH << 8) | receive->frame.angleL);
    motor->state.dataUpdateTime = upTime();
    receive->valid = false;
    // dlog("processData: canId=%d", receive->canId);
}

static void __sendData(motorController_t *motor)
{
    int canId = 0x200;
    uint8_t data[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    data[0] = motor->outputCurrentSet >> 8;
    data[1] = motor->outputCurrentSet & 0xFF;
    canSendData(motor->can, canId, data, sizeof(data));
    // dlog("sendData: canId=%d", canId);
}

void motorSetConfig(motorController_t *motor, bool isChange, int speedLevel)
{
    dlog("receive data: isChange=%d, speedLevel=%d", isChange, speedLevel);
    dlog("current data: enable=%d, speedLevel=%d, outputCurrentSet=%d", motor->enable, motor->speedLevel, motor->outputCurrentSet);
    if (isChange) {
        motor->enable = !motor->enable;
        if (!motor->enable) {
            motor->speedLevel = 0;
        }
    }
    if (motor->enable && speedLevel != motor->speedLevel) {
        motor->speedLevel = speedLevel;
    }
    motor->outputCurrentSet = speedLevel >= 0 ?controlData[motor->speedLevel]: -1*controlData[-1*motor->speedLevel];
    dlog("set data: enable=%d, speedLevel=%d, outputCurrentSet=%d", motor->enable, motor->speedLevel, motor->outputCurrentSet);
}

int motorGetSpeedLevel(motorController_t *motor)
{
    return motor->speedLevel;
}

bool motorGetEnable(motorController_t *motor)
{
    return motor->enable;
}


const motorState_t *getMotorState(motorController_t *motor)
{
    if (!motor) {
        return NULL;
    }
    return &motor->state;
}