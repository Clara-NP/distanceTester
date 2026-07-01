list(APPEND srcs main.c)
list(APPEND srcs board.c)
list(APPEND srcs motor_manage.c)
list(APPEND srcs control_manage.c)


set(include_dirs . 
                config
                ../components/framework/source/driver
                ../components/framework/third)

idf_component_register(SRCS ${srcs}
                        INCLUDE_DIRS ${include_dirs})