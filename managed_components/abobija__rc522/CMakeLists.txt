idf_component_register(
    INCLUDE_DIRS include
    SRCS src/rc522.c
    REQUIRES esp_event esp_driver_spi freertos
)
