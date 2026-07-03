#ifndef __CONFIG_H__
#define __CONFIG_H__

/**
 * @brief 配置子设备框架内任务默认优先级
 * 
 */
#ifndef CONFIG_FRAMEWORK_TASK_PRIORITY  
#define CONFIG_FRAMEWORK_TASK_PRIORITY  5
#endif 

/**
 * @brief 配置电机管理任务优先级
 * 
 */
#ifndef CONFIG_MOTOR_MANAGE_TASK_PRIORITY  
#define CONFIG_MOTOR_MANAGE_TASK_PRIORITY  6
#endif 

/**
 * @brief 配置控制管理任务优先级
 * 
 */
#ifndef CONFIG_CONTROL_MANAGE_TASK_PRIORITY  
#define CONFIG_CONTROL_MANAGE_TASK_PRIORITY  8
#endif 

/**
 * @brief 配置总线设备管理任务优先级
 * 
 */
 #ifndef CONFIG_BUS_DEVICE_TASK_PRIORITY
 #define CONFIG_BUS_DEVICE_TASK_PRIORITY     10
 #endif

#endif