#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_log.h"

#include "config.h"

static const char *TAG = "app";

// void app_main(void)
// {
//     esp_chip_info_t chip_info;
//     esp_chip_info(&chip_info);

//     ESP_LOGI(TAG, "Hello ESP-IDF!");
//     ESP_LOGI(TAG, "芯片: %s, 核心数: %d", CONFIG_IDF_TARGET, chip_info.cores);

//     int count = 0;
//     while (1) {
//         ESP_LOGI(TAG, "运行中... count = %d", count++);
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }
extern void boardInit(void);
void startTask(void *parameter)
{
    boardInit();



    
    // 释放启动任务
    vTaskDelete(NULL);
}

void app_main(void)
{
    // // 初始化BSP platform
    // platformInit();

    // 打印平台版本信息
    // ilog("Framework Version:%s", FRAMEWORK_VERSION_STRING);
    // ilog("Project Version: V%d.%d", CONFIG_VERSION_MAJOR, CONFIG_VERSION_MINOR);

    // 预留大点的栈空间，有些API需要比较大的缓存
    xTaskCreate(startTask, "start", 4096, NULL, CONFIG_FRAMEWORK_TASK_PRIORITY, NULL);

}