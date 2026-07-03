#include "gpio/gpio.h"
#include "adc/adc.h"
#include "serial/serial.h"
#include "can/can.h"
#include "export_ids.h"
#include "i2c/i2c.h"


/**
 * @brief 定义使用的GPIO
 * 
 */
static const gpioConfig_t s_gpios[] = 
{
    { EM_GPIO_LED,              GPIO_CHIP_SOC, _GPIO2, GPIO_DIR_OUTPUT, 0},
    { EM_GPIO_ROTARY_A,         GPIO_CHIP_SOC, _GPIO33, GPIO_DIR_OUTPUT, 0},
    { EM_GPIO_ROTARY_B,         GPIO_CHIP_SOC, _GPIO26, GPIO_DIR_OUTPUT, 0},
    { EM_GPIO_ROTARY_KEY,       GPIO_CHIP_SOC, _GPIO21, GPIO_DIR_INPUT, 0},
};

 /**
  * @brief 定义使用的ADC
  * 
  */
static const adcConfig_t s_adcs[] =
{
    //_ADC1 : GPIO1 -> channel0, GPIO2 -> channel1,... GPIO10 -> channel9
    {.id = EM_ADC_12V_POWER,         .adc = _ADC1, .channel = _ADC_CH9,  .flags = ADC_FLAG_RANGE(13), .calculation = ADC_LINEAR(0, 0.011f), .parameter = NULL},
    {.id = EM_ADC_CHARGE_GUN_CC1,    .adc = _ADC1, .channel = _ADC_CH8,  .flags = ADC_FLAG_RANGE(13), .calculation = ADC_LINEAR(0, 0.001f), .parameter = NULL},
    {.id = EM_ADC_CHARGE_GUN_CC2,    .adc = _ADC1, .channel = _ADC_CH7,  .flags = ADC_FLAG_RANGE(13), .calculation = ADC_LINEAR(0, 0.001f), .parameter = NULL},

};
 
 
 /**
  * @brief 定义项目使用串口列表 
  * 
  */
static const serialConfig_t s_serials[] = 
{
    /// CORE USART0 _GPIO43(tx) _GPIO44(rx)
    // {
    //     .id = EM_DEBUG_SERIAL, .chip = SERIAL_CHIP_SOC, .port = _UART0, .setting = _UART_SETTING_DEFAULT(115200), 
    //     .control = 0, .chipParameter = NULL
    // },  
    /// CORE USART1 _GPIO37(tx) _GPIO39(rx) _GPIO38(DE) 
    {
        .id = EM_RS485_SERIAL, .chip = SERIAL_CHIP_SOC, .port = _UART1, .setting = _UART_SETTING_DEFAULT(9600), 
        .control = _UART_RS485_GPIO(_GPIO38), .chipParameter = NULL
    }, 
    /// CORE USART2 _GPIO41(tx) _GPIO40(rx)
    {
        .id = EM_RS232_SERIAL, .chip = SERIAL_CHIP_SOC, .port = _UART2, .setting = _UART_SETTING_DEFAULT(2400), 
        .control = 0, .chipParameter = NULL
    }, 
};
 
 /**
  * @brief 定义使用的CAN
  * 
  */
static const canConfig_t s_cans[] =
{
    {.id = EM_CAN_MOTOR, .chip = CAN_CHIP_SOC, .port = _CAN0, .parameter = \
        {.canIdFilterValue = 0, .canIdFilterMask = 0, .rate = CAN_RATE_1000KBS, \
        .specialParameter = "tx:4,rx:2"}}
};

/**
 * @brief 定义使用的I2C
 * 
 */
static const i2cConfig_t s_i2cs[] =
{
    {.id = EM_I2C_1, .chip = I2C_CHIP_GPIO_SIMULATE, .port = _I2C1, .rate = 100000, .chipParameter = "scl:18,sda:17"}
};