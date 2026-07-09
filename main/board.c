#include "board_distance_test.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "driver/gpio.h"
#define TRACE_TAG "board"
#define TRACE_LEVEL T_DEBUG
#define TRACE_ENABLE

#include <common/trace.h>


void boardInit(void)
{
    // ilog("Board: %s", BOARD_NAME);
    // 初始化GPIO
    gpioInit(s_gpios, ARRAY_SIZE(s_gpios));
    // 初始化IIC
    i2cInit(s_i2cs, ARRAY_SIZE(s_i2cs));
    // 初始化SPI
    spiInit(s_spis, ARRAY_SIZE(s_spis));
    // 初始化can
    // 这里要注意下，如果can初始化放在串口后面会导致申请中断资源失败，待排查
    canInit(s_cans, ARRAY_SIZE(s_cans));
    // // 初始化串口
    // serialInit(s_serials, ARRAY_SIZE(s_serials));
}