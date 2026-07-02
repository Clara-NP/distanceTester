
set(framework_driver_path framework/source/driver)
set(framework_platform_path framework/source/platform)
set(framework_component_path framework/source/component)
set(framework_third_path framework/third)
set(esp32_platform_path ${framework_platform_path}/esp32)
set(module_path module)


set(include_dirs .  ${module_path}/include
                    ${IDF_PATH}/components/esp_ringbuf/include
                    ${IDF_PATH}/components/driver/gpio/include
                    ${IDF_PATH}/components/driver/uart/include
                    ${IDF_PATH}/components/driver/twai/include
                    ${IDF_PATH}/components/driver/pcnt/include
                    # ${IDF_PATH}/components/driver/i2c/include
                    ${IDF_PATH}/components/esp_adc/include
                    ${IDF_PATH}/components/console
                    ${IDF_PATH}/components/driver/usb_serial_jtag/include
                    ${IDF_PATH}/components/esp_https_ota/include/
                    ${IDF_PATH}/components/esp_http_server/include/
                    ${IDF_PATH}/components
                    ${framework_platform_path}
                    ${framework_driver_path}
                    ${framework_component_path}
                    ${framework_third_path})

# 判断目标设备类型
if(CONFIG_IDF_TARGET_ESP32S2)
    # ESP32-S2
    list(APPEND include_dirs ${IDF_PATH}/components/esp_adc/esp32s2/include)
elseif(CONFIG_IDF_TARGET_ESP32S3)
    # ESP32-S3
    list(APPEND include_dirs ${IDF_PATH}/components/esp_adc/esp32s3/include)
endif()



## 平台相关
list(APPEND srcs ${esp32_platform_path}/platform_private.c)
list(APPEND srcs ${esp32_platform_path}/soc_trace.c)
list(APPEND srcs ${esp32_platform_path}/soc_gpio.c)
list(APPEND srcs ${esp32_platform_path}/soc_adc.c)
list(APPEND srcs ${esp32_platform_path}/soc_uart.c)
list(APPEND srcs ${esp32_platform_path}/soc_can.c)
# list(APPEND srcs ${esp32_platform_path}/soc_i2c.c)
list(APPEND srcs ${esp32_platform_path}/sys_tick.c)
# list(APPEND srcs ${esp32_platform_path}/soc_quad_encoder.c)

## 驱动相关
list(APPEND srcs ${framework_driver_path}/gpio/gpio.c)
list(APPEND srcs ${framework_driver_path}/adc/adc.c)
list(APPEND srcs ${framework_driver_path}/serial/serial.c)
list(APPEND srcs ${framework_driver_path}/can/can.c)
# list(APPEND srcs ${framework_driver_path}/quad_encoder/quad_encoder.c)
# list(APPEND srcs ${framework_driver_path}/i2c/i2c.c)
# list(APPEND srcs ${framework_driver_path}/i2c/i2c_gpio.c)

## 组件相关
list(APPEND srcs ${framework_component_path}/console/command.c)
list(APPEND srcs ${framework_component_path}/console/console.c)
list(APPEND srcs ${framework_component_path}/flow_task/flow_task.c)
list(APPEND srcs ${framework_component_path}/misc/simple_crc.c)
list(APPEND srcs ${framework_component_path}/misc/string_helper.c)
list(APPEND srcs ${framework_component_path}/misc/date_time.c)
list(APPEND srcs ${framework_component_path}/stream_fifo/stream_fifo.c)
list(APPEND srcs ${framework_component_path}/npcp/lib/npcp_frame.c)
list(APPEND srcs ${framework_component_path}/npcp/slave/npcp_slave.c)
list(APPEND srcs ${framework_component_path}/simple_modbus/serial_modbus.c)


## 第三方
list(APPEND srcs ${framework_third_path}/json/cJSON/cJSON.c)
list(APPEND srcs ${framework_third_path}/json/cJSON/cJSON_Utils.c)


## 遥控箱模块
list(APPEND srcs ${module_path}/source/DJI_motor.c)
list(APPEND srcs ${module_path}/source/rotary.c)
list(APPEND srcs ${module_path}/source/io_module.c)

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS ${include_dirs})

