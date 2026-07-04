idf_component_register(
                    SRCS "main.cpp"
                    "modules/LCDI2C.cpp"
                    "modules/WifiDriver.cpp"
                    "modules/JsonParser.cpp"
                    "modules/RFIDManager.cpp"
                    "modules/ServoManager.cpp"
                    "modules/FingerprintManager.cpp"
                         INCLUDE_DIRS "." "modules"
                         REQUIRES freertos nvs_flash driver esp_wifi esp_event esp_http_client json
                       )
