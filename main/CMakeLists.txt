# Generar secrets.h desde .env
set(ENV_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../.env")
if(NOT EXISTS ${ENV_FILE})
    message(FATAL_ERROR ".env no encontrado en ${ENV_FILE}. Copiá .env.example a .env y completá tus credenciales.")
endif()

set(SECRETS_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
file(MAKE_DIRECTORY ${SECRETS_DIR})
file(WRITE ${SECRETS_DIR}/secrets.h "// Auto-generated from .env\n#pragma once\n\n")

file(STRINGS ${ENV_FILE} ENV_LINES)
foreach(LINE ${ENV_LINES})
    string(STRIP "${LINE}" LINE)
    if(LINE MATCHES "^([a-zA-Z_][a-zA-Z0-9_]*)=(.*)$")
        set(KEY ${CMAKE_MATCH_1})
        set(VAL ${CMAKE_MATCH_2})
        file(APPEND ${SECRETS_DIR}/secrets.h "#define ${KEY} \"${VAL}\"\n")
    endif()
endforeach()

idf_component_register(
    SRCS "main.cpp"
         "modules/LCDI2C.cpp"
         "modules/WifiDriver.cpp"
         "modules/ServoManager.cpp"
         "modules/FingerprintManager.cpp"
         "modules/AccessControl.cpp"
         "modules/AccessDB.cpp"
         "modules/RFIDManager.cpp"
         "modules/Indicators.cpp"
    INCLUDE_DIRS "." "modules" "${SECRETS_DIR}"
    REQUIRES freertos nvs_flash driver esp_wifi esp_event esp_http_client esp_timer esp_driver_uart esp_driver_gpio esp_driver_ledc esp_driver_spi esp_driver_i2c rc522
)
