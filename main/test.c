/**
 * @file test.c
 * @brief 最简 rotary(旋转编码器) 检测测试程序
 *
 * 使用 ESP-IDF 5.2 新版 PCNT API (driver/pulse_cnt.h)
 * 引脚：A=GPIO33, B=GPIO26
 *
 * 使用方法：
 *   1. 将本文件加入 main/CMakeLists.txt 的 srcs
 *   2. 把 main.c 里的 app_main 注释掉，避免 app_main 重复定义
 *   3. idf.py build flash monitor
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"

static const char *TAG = "rotary-test";

// 旋转编码器引脚（与板子 board_distance_test.h 对应）
#define ROTARY_A_GPIO   33
#define ROTARY_B_GPIO   26

void test(void)
{
    pcnt_unit_handle_t unit = NULL;
    pcnt_channel_handle_t chan = NULL;

    // 1. 创建 PCNT 单元
    pcnt_unit_config_t unitConfig = {
        .low_limit  = -1000,
        .high_limit = 1000,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unitConfig, &unit));

    // 2. 毛刺滤波 1us，去抖
    pcnt_glitch_filter_config_t filterConfig = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(unit, &filterConfig));

    // 3. 创建通道：A为边沿信号，B为电平信号
    pcnt_chan_config_t chanConfig = {
        .edge_gpio_num  = ROTARY_A_GPIO,
        .level_gpio_num = ROTARY_B_GPIO,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(unit, &chanConfig, &chan));

    // 4. EC11 正交解码动作
    //    A上升沿 +1，A下降沿 -1
    //    B高电平时方向反向，B低电平时保持
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(chan,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(chan,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP));

    // 5. 使能并启动
    ESP_ERROR_CHECK(pcnt_unit_enable(unit));
    ESP_ERROR_CHECK(pcnt_unit_start(unit));

    ESP_LOGI(TAG, "rotary test start: A=%d B=%d", ROTARY_A_GPIO, ROTARY_B_GPIO);

    // 6. 周期读取计数值，判断旋转方向
    int lastCount = 0;
    while (1) {
        int count = 0;
        pcnt_unit_get_count(unit, &count);

        if (count != lastCount) {
            int delta = count - lastCount;
            ESP_LOGI(TAG, "count=%d delta=%d (%s)",
                     count, delta, delta > 0 ? "CW 顺时针" : "CCW 逆时针");
            lastCount = count;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
