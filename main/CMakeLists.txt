idf_component_register(
                    SRCS "main.cpp"
                         "modules/LCDI2C.cpp"
                         INCLUDE_DIRS "." "modules"
                         REQUIRES freertos
                       )
