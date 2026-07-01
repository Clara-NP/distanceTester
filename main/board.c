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
    // i2cInit(s_i2cs, ARRAY_SIZE(s_i2cs));
    // 初始化ADC
    // adcInit(s_adcs, ARRAY_SIZE(s_adcs));
    // 初始化can
    // 这里要注意下，如果can初始化放在串口后面会导致申请中断资源失败，待排查
    canInit(s_cans, ARRAY_SIZE(s_cans));
    // // 初始化串口
    // serialInit(s_serials, ARRAY_SIZE(s_serials));

    // int ret;
    // char data[8];
    // int canId;

    // while(1)
    // {
    //     // gpioSet(EM_GPIO_LED, 1);
    //     // ESP_LOGI(TRACE_TAG, "GPIO_LED on");
    //     // vTaskDelay(pdMS_TO_TICKS(500));
    //     // gpioSet(EM_GPIO_LED, 0);
    //     // ESP_LOGI(TRACE_TAG, "GPIO_LED off");
    //     // vTaskDelay(pdMS_TO_TICKS(500));


    //     // ret = canReceiveData(EM_CAN_MOTOR, &canId, (uint8_t *)&data, sizeof(data));
    //     // if(ret > 0) {
    //     //     ESP_LOGI(TRACE_TAG, "canReceiveData: canId=%d", canId);
    //     // } else {
    //     //     ESP_LOGI(TRACE_TAG, "canReceiveData: no data");
    //     // }

    //     // gpio_set_level(1, 0);
    //     vTaskDelay(pdMS_TO_TICKS(500));
    //     // gpio_set_level(1, 1);
    //     // vTaskDelay(pdMS_TO_TICKS(500));
    // }
}