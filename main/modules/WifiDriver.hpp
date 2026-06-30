#ifndef WIFIDRIVER_H_
#define WIFIDRIVER_H_

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "esp_event.h"
#include "nvs_flash.h"

class WifiDriver {
    public:
        WifiDriver(std::string ssID, std::string passWord, int maxRetry = 5);
        ~WifiDriver();
        void init();
        void conect();
        void disconect();
        void scanNetworks();
        void makeGetRequest(const std::string& ipAddress, int port, const std::string& path);
        void makePostRequest(const std::string& ipAddress, int port, const std::string& path, const char* post_data, size_t post_len, const std::string& contentType = "application/json");
        void setPayload(const char* data, size_t len);
        const char* getPayloadBytes() const;
        size_t getPayloadLength() const;
        void clearPayload();
        void appendPayload(const char* data, int len);
        int s_retry_num;

    private:
        std::string _ssID;
        std::string _passWord;
        int _maxRetry;
        char* _payload;
        size_t _payload_len;
        size_t _payload_cap;

        EventGroupHandle_t _wifi_event_group;

        static constexpr unsigned int WIFI_CONNECTED_BIT = (1 << 0);
        static constexpr unsigned int WIFI_FAIL_BIT      = (1 << 1);
        static constexpr const char* TAG = "WIFI_MAIN";

        static void _eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
};

#endif // WIFIDRIVER_H_
