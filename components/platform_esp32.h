
#ifndef __PLATFORM_ESP32_H__
#define __PLATFORM_ESP32_H__

/*******************************************/
/*              PLATFORM-ESP32             */
/*******************************************/

/**
 * @brief 平台选择 
 * 
 */
#define CONFIG_PLATFORM_HEADER  "esp32/platform.h"

/// 配置默认的调试串口索引
#ifndef CONFIG_TRACE_UART
#define CONFIG_TRACE_UART  _UART0
#endif 


/**
 * @brief 启用CAN错误统计
 * 
 */
#define CONFIG_ESP32_CAN_ERROR_COUNTERS  1


#endif // __PLATFORM_ESP32_H__

