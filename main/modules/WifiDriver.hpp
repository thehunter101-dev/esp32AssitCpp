#ifndef WIFIDRIVER_H_
#define WIFIDRIVER_H_
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

class WifiDriver {
    public:
        WifiDriver(std::string ssID, std::string passWord, int maxRetry = 5);
        void conect();
        void init();
        void disconect();
        static int s_retry_num;

    private:
        static constexpr unsigned int WIFI_CONNECTED_BIT = BIT0;
        static constexpr unsigned int WIFI_FAIL_BIT = BIT1;
        std::string _ssID;
        std::string _passWord;
        int _retry;

        static constexpr std::string TAG = "WIFI_MAIN";

        void _eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_date);


};

#endif // WIFIDRIVER_H_
