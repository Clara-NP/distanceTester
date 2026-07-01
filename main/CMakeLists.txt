list(APPEND srcs main.c)
list(APPEND srcs board.c)


set(include_dirs . 
                config
                ../components/framework/source/driver
                ../components/framework/third)

idf_component_register(SRCS ${srcs}
                        INCLUDE_DIRS ${include_dirs})