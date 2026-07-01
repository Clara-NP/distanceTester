#ifndef __EXPORT_IDS_H__
#define __EXPORT_IDS_H__



/**
 * @brief GPIO定义
 * 
 */
enum EXPORT_GPIO
{
    EM_GPIO_LED,
    EM_GPIO_ROTARY_A,
    EM_GPIO_ROTARY_B,
    EM_GPIO_ROTARY_KEY,
    EXPORT_GPIO_MAX_NUM,
};
/**
 * @brief ADC定义
 * 
 */
enum EXPORT_ADC
{
    EM_ADC_CHARGE_GUN_CC1,
    EM_ADC_CHARGE_GUN_CC2,
    EM_ADC_12V_POWER,

    EXPORT_ADC_MAX_NUM,
};

/**
 * @brief 串口逻辑ID定义
 * 
 */
enum EXPORT_SERIAL
{
    EM_DEBUG_SERIAL,
    EM_RS485_SERIAL,
    EM_RS232_SERIAL,

    EXPORT_SERIAL_MAX_NUM,
};

/**
 * @brief CAN定义
 * 
 */
enum EXPORT_CAN
{
    EM_CAN_MOTOR,

    EXPORT_CAN_MAX_NUM,
};

/**
 * @brief 其他定义
 * 
 */
enum EXPORT_I2C
{
    EM_I2C_1,

    EXPORT_I2C_MAX_NUM,
};

#endif