#include "driver/pulse_cnt.h"

#include "common/generic.h"
#include "rotary.h"
#include "esp_err.h"

#define TRACE_TAG "rotary"
#define TRACE_LEVEL T_DEBUG
#define TRACE_ENABLE

#include <common/trace.h>

struct rotaryController
{
    // uint8_t rotaryA;
    // uint8_t rotaryB;
    // // const powerConfig_t *config;

    /// CAN总线是否忙
    /// 这里采用一问一答方式，执行读取/设置/控制命令必须保证总线是否为空闲状态
    // uint8_t busBusy;
    /// 超时计数
    uint8_t timeoutCount;

    /// 节点ID
    uint8_t nodeId;


    // 函数配置
    

    // /// 电流是否配置完成（华为电源每次打开，都要设置输出电流，并且只有在电压有输出的情况下设置输出电流才有效）
    // uint8_t outputCurrentReady;
    // /// 设置(输出电流)尝试次数
    // uint8_t setConfigAttempts;

    // /// 接收缓存
    // receiveFrame_t receive[CONFIG_POWER_RECEIVE_BUFFER_ITEM_SIZE];

    // /// power信息
    // powerInfo_t info;
    /// motor状态
    // motorState_t state;

    // /// 下次探测时间
    // sysTick_t expiredDetection;
    // /// 离线时间
    // sysTick_t expiredOffline;

    // // 数据处理事件
    // sysTick_t processTimer;
    // /// 数据发送事件
    // sysTick_t sendTimer;

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
    float outputCurrentSet;

};


pcnt_unit_handle_t unit = NULL;
rotaryController_t *rotaryNew(uint8_t nodeId)
{
    rotaryController_t *m;

    m = (rotaryController_t *)osMalloc(sizeof(rotaryController_t));
    if (m == NULL) {
        elog("rotaryNew: malloc failed");
        return NULL;
    }
    osMemset(m, 0, sizeof(rotaryController_t));

    m->nodeId = nodeId;

    return m;
}


#define ROTARY_A_GPIO   33
#define ROTARY_B_GPIO   26
int rotaryEncoderInit(rotaryController_t *m)
{
    int ret = RET_FAILED;
    pcnt_channel_handle_t chan = NULL;
    // 1. 创建 PCNT 单元
    pcnt_unit_config_t unitConfig = {
        .low_limit  = -1000,
        .high_limit = 1000,
    };
    ret = pcnt_new_unit(&unitConfig, &unit);
    ASSERT(ret == ESP_OK, "rotaryEncoderInit: pcnt_new_unit failed");

    // 2. 毛刺滤波 1us，去抖
    pcnt_glitch_filter_config_t filterConfig = {
        .max_glitch_ns = 1000,
    };
    ret = pcnt_unit_set_glitch_filter(unit, &filterConfig);
    ASSERT(ret == ESP_OK, "rotaryEncoderInit: pcnt_unit_set_glitch_filter failed");

    // 3. 创建通道：A为边沿信号，B为电平信号
    pcnt_chan_config_t chanConfig = {
        .edge_gpio_num  = ROTARY_A_GPIO,
        .level_gpio_num = ROTARY_B_GPIO,
    };
    ret = pcnt_new_channel(unit, &chanConfig, &chan);
    ASSERT(ret == ESP_OK, "rotaryEncoderInit: pcnt_new_channel failed");

    // 4. EC11 正交解码动作
    //    A上升沿 +1，A下降沿 -1
    //    B高电平时方向反向，B低电平时保持
    ret = pcnt_channel_set_edge_action(chan,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    ASSERT(ret == ESP_OK, "rotaryEncoderInit: pcnt_channel_set_edge_action failed");
    ret = pcnt_channel_set_level_action(chan,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP);
    ASSERT(ret == ESP_OK, "rotaryEncoderInit: pcnt_channel_set_level_action failed");

    // 5. 使能并启动
    ret = pcnt_unit_enable(unit);
    ASSERT(ret == ESP_OK, "rotaryEncoderInit: pcnt_unit_enable failed");
    ret = pcnt_unit_start(unit);
    ASSERT(ret == ESP_OK, "rotaryEncoderInit: pcnt_unit_start failed");

    ilog("rotary test start: A=%d B=%d", ROTARY_A_GPIO, ROTARY_B_GPIO);

    return RET_SUCCESS;
}


int rotaryEncoderRead(rotaryController_t *m)
{
    // static int lastCount = 0;
    int count = 0;
    pcnt_unit_get_count(unit, &count);

    // if (count != lastCount) {
    //     int delta = count - lastCount;
    //     ilog("count=%d delta=%d (%s)",
    //              count, delta, delta > 0 ? "CW 顺时针" : "CCW 逆时针");
    //     lastCount = count;
    // }

    return count;
}

