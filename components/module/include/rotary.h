#ifndef __ROTARY_H__
#define __ROTARY_H__

#include "common/generic.h"

/**
 * @brief 旋钮实例定义
 * 
 */
typedef struct rotaryController rotaryController_t;


/// @brief 一个编码器配置
typedef struct 
{
    /// 逻辑ID
    uint8_t id;
    /// 编码器名称
    const char *name;
    /// 编码器A引脚
    uint8_t rotaryA;
    /// 编码器B引脚
    uint8_t rotaryB;
//     /// 伺服驱动
//     uint8_t model;
//     /// 模式
//     uint8_t mode;
    /// 节点ID
    uint8_t address;
//     /// 编码器ID
//     uint8_t encoder;
//     /// 小轮编码器的精度 
//     uint16_t encoderAccuracy;

//     // 控制标志
// #define _ENCODER_OPPOSITE  (1 << 0)
//     uint8_t flag;
}rotaryConfig_t;

// rotaryController_t *rotaryNew(uint8_t rotaryA, uint8_t rotaryB, const char *name, const rotaryConfig_t *config);
rotaryController_t *rotaryNew(uint8_t nodeId);
int rotaryEncoderInit(rotaryController_t *m);
int rotaryEncoderRead(rotaryController_t *m);
#endif // __ROTARY_H__