list(APPEND srcs main.c)
list(APPEND srcs board.c)
list(APPEND srcs motor_manage.c)
list(APPEND srcs control_manage.c)
list(APPEND srcs sys_event.c)
list(APPEND srcs bus_device_manage.c)
list(APPEND srcs monitor.c)



set(include_dirs . 
                config
                ../components/framework/source/driver
                ../components/framework/third)

idf_component_register(SRCS ${srcs}
                        INCLUDE_DIRS ${include_dirs})