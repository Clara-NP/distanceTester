#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_log.h"

#include "config.h"
#include "motor_manage.h"
#include "control_manage.h"
#include "bus_device_manage.h"


static const char *TAG = "app";

extern void boardInit(void);
void startTask(void *parameter)
{
    // 初始化板级硬件
    boardInit();
    // 电机管理任务
    motorManageInit();
    // 控制任务
    controlManageInit();
    // 总线设备管理
    busDeviceManageInit();
    
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