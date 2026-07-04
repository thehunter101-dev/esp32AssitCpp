#ifndef FINGERPRINTMANAGER_H_
#define FINGERPRINTMANAGER_H_

#include <cstdint>
#include <vector>
#include <functional>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define FPM_BUF_SIZE 1024

// Comandos estáticos pre-calculados (Header + Addr + PkgID + Len + Instr + Data + Checksum)
static const uint8_t FPM_CMD_GEN_IMG[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x01, 0x00, 0x05};
static const uint8_t FPM_CMD_IMG2TZ[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x02, 0x01, 0x00, 0x08};

class FingerprintManager {
public:
    using ResultCallback = std::function<void(bool success, const std::vector<uint8_t>& templateData, uint16_t fingerID, const char* errorMsg)>;

    FingerprintManager(gpio_num_t txPin = GPIO_NUM_12,
                       gpio_num_t rxPin = GPIO_NUM_14,
                       uart_port_t uartNum = UART_NUM_1,
                       uint32_t baud = 57600);
    ~FingerprintManager();

    bool init();

    // GenImg: true si hay dedo (response[9] == 0x00)
    bool detectFinger();

    // Bucle 6s: espera dedo → GenImg → Img2Tz → template dummy 512B
    bool captureTemplate(uint32_t timeoutMs = 6000);

    const std::vector<uint8_t>& getLastTemplate() const;
    uint16_t getLastFingerID() const;
    void setResultCallback(ResultCallback cb);

private:
    gpio_num_t _txPin;
    gpio_num_t _rxPin;
    uart_port_t _uartNum;
    uint32_t _baud;
    std::vector<uint8_t> _lastTemplate;
    uint16_t _lastFingerID;
    ResultCallback _callback;

    uint8_t _response[FPM_BUF_SIZE];

    static constexpr const char* TAG = "FP_MGR";

    bool sendAndRead(const uint8_t* cmd, size_t cmdLen, int timeoutMs);
    uint8_t getConfCode() const;
};

#endif
