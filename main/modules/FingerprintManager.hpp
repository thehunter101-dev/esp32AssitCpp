#ifndef FINGERPRINTMANAGER_H_
#define FINGERPRINTMANAGER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <cstdint>
#include <functional>
#include <vector>
#include <cstring>
#include "nvs_flash.h"

#define FPM_BUF_SIZE 1024

class FingerprintManager {
public:
    using ResultCallback = std::function<void(bool success, const std::vector<uint8_t>& templateData, uint16_t fingerID, const char* errorMsg)>;

    FingerprintManager(gpio_num_t txPin = GPIO_NUM_12,
                       gpio_num_t rxPin = GPIO_NUM_14,
                       uart_port_t uartNum = UART_NUM_1,
                       uint32_t baud = 57600);
    ~FingerprintManager();

    bool init();

    bool detectFinger();
    bool captureTemplate(uint32_t timeoutMs = 6000);
    bool searchFinger(uint16_t& fingerID, uint16_t& score);
    bool enrollFinger(uint16_t& fingerID);
    bool deleteAllFingers();

    const std::vector<uint8_t>& getLastTemplate() const;
    uint16_t getLastFingerID() const;
    void setResultCallback(ResultCallback cb);

private:
    gpio_num_t _txPin, _rxPin;
    uart_port_t _uartNum;
    uint32_t _baud;
    std::vector<uint8_t> _lastTemplate;
    uint16_t _lastFingerID;
    nvs_handle_t _nvsHandle;
    ResultCallback _callback;

    uint8_t _response[FPM_BUF_SIZE];

    static constexpr const char* TAG = "FP_MGR";

    static uint8_t buildPacket(uint8_t* cmd, uint8_t instruction, const uint8_t* params, uint8_t paramLen);
    bool sendAndRead(const uint8_t* cmd, size_t cmdLen, int timeoutMs);
    uint8_t getConfCode() const;
    bool waitForFinger(uint32_t timeoutMs);
    uint16_t _getNextFingerID();
    void _saveNextFingerID(uint16_t nextID);
};

#endif
