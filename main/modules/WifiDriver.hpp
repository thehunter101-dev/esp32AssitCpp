#ifndef WIFIDRIVER_H_
#define WIFIDRIVER_H_
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
    WifiDriver(std::string ssID, std::string passWord, int maxRetry = 5);
    public:
        void conect();
        void disconect();
        static int s_retry_num;

    private:
        static constexpr unsigned int WIFI_CONNECTED_BIT = BIT0;
        static constexpr unsigned int WIFI_FAIL_BIT = BIT1;
        std::string _ssID;
        std::string _passWord;
        int _retry;

        static constexpr std::string TAG = "WIFI_MAIN";

};

#endif // WIFIDRIVER_H_
