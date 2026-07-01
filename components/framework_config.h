
#ifndef __FRAMEWORK_CONFIG_H__
#define __FRAMEWORK_CONFIG_H__


/**
 * @brief 是否支持FREERTOS
 * 
 */
//#define CONFIG_FREERTOS   1 


/**
 * @brief 支持打印颜色
 * 
 */
#define CONFIG_TRACE_COLORS             1

/**
 * @brief 配置CONSOLE提示字串
 * 
 */
#define CONFIG_CONSOLE_PROMPT           "CLI> "

/**
 * @brief 配置CONSOLE是否需要登录
 * 
 */
// #define CONFIG_CONSOLE_REQUIRE_LOGIN    1

/**
 * @brief 配置CONSOLE输入回车时是否执行上一次的命令
 * 
 */
#define CONFIG_CONSOLE_ENTER_EXECUTE_LAST_COMMAND  1

/**
 * @brief 
 * 
 */
#ifndef CONFIG_CONSOLE_PLATFORM_LOWLEVEL_FUNCTION
#define CONFIG_CONSOLE_PLATFORM_LOWLEVEL_FUNCTION   1
#endif


/**
 * @brief CONSOLE关联到telnet
*/
#define CONFIG_CONSOLE_ASSOCIATION_TELNET       1


/**
 * @brief simple modbus包最大大小定义
*/
#define CONFIG_SIMPLE_MODBUS_PACKET_BUFFER_SIZE     200

/**
 * @brief 当前使用了的最大GPIO的数量，可在外部配置
 * 
 */
#define CONFIG_GPIO_MAX_NUM      32

#include "platform_esp32.h"

#endif // __FRAMEWORK_CONFIG_H__

