#include "FingerprintManager.hpp"

FingerprintManager::FingerprintManager(gpio_num_t txPin, gpio_num_t rxPin, uart_port_t uartNum, uint32_t baud)
    : _txPin(txPin), _rxPin(rxPin), _uartNum(uartNum), _baud(baud),
      _lastFingerID(0), _callback(nullptr)
{
}

FingerprintManager::~FingerprintManager()
{
    uart_driver_delete(_uartNum);
}

bool FingerprintManager::init()
{
    uart_config_t uart_config = {};
    uart_config.baud_rate = (int)_baud;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity    = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;

    esp_err_t ret;

    ret = uart_param_config(_uartNum, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fallo uart_param_config: %s", esp_err_to_name(ret));
        return false;
    }

    ret = uart_set_pin(_uartNum, _txPin, _rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fallo uart_set_pin: %s", esp_err_to_name(ret));
        return false;
    }

    ret = uart_driver_install(_uartNum, FPM_BUF_SIZE * 2, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fallo uart_driver_install: %s", esp_err_to_name(ret));
        return false;
    }

    gpio_set_pull_mode(_txPin, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(_rxPin, GPIO_PULLUP_ONLY);

    ESP_LOGI(TAG, "FP AS608 en UART%d TX:%d RX:%d @%lu",
             (int)_uartNum, (int)_txPin, (int)_rxPin, (unsigned long)_baud);
    return true;
}

bool FingerprintManager::sendAndRead(const uint8_t* cmd, size_t cmdLen, int timeoutMs)
{
    uart_flush(_uartNum);
    uart_write_bytes(_uartNum, (const char*)cmd, cmdLen);
    int len = uart_read_bytes(_uartNum, _response, FPM_BUF_SIZE, pdMS_TO_TICKS(timeoutMs));
    return (len >= 12);
}

uint8_t FingerprintManager::getConfCode() const
{
    return _response[9];
}

bool FingerprintManager::detectFinger()
{
    if (!sendAndRead(FPM_CMD_GEN_IMG, sizeof(FPM_CMD_GEN_IMG), 200)) {
        return false;
    }
    return (getConfCode() == 0x00);
}

bool FingerprintManager::captureTemplate(uint32_t timeoutMs)
{
    _lastTemplate.clear();
    _lastFingerID = 0;

    // 1. Esperar dedo con timeout (polling cada 200ms)
    TickType_t start = xTaskGetTickCount();
    bool fingerDetected = false;

    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeoutMs)) {
        if (detectFinger()) {
            fingerDetected = true;
            ESP_LOGI(TAG, "Dedo detectado, convirtiendo imagen...");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    if (!fingerDetected) {
        ESP_LOGW(TAG, "captureTemplate: timeout esperando dedo");
        if (_callback) _callback(false, {}, 0, "Timeout esperando dedo");
        return false;
    }

    // 2. Img2Tz (convertir imagen a template en buffer 1)
    if (!sendAndRead(FPM_CMD_IMG2TZ, sizeof(FPM_CMD_IMG2TZ), 1000)) {
        ESP_LOGE(TAG, "captureTemplate: Img2Tz sin respuesta");
        if (_callback) _callback(false, {}, 0, "Img2Tz no responde");
        return false;
    }
    if (getConfCode() != 0x00) {
        ESP_LOGE(TAG, "captureTemplate: Img2Tz fallo 0x%02X", getConfCode());
        if (_callback) _callback(false, {}, 0, "Img2Tz fallo");
        return false;
    }

    ESP_LOGI(TAG, "Imagen convertida a template. Generando dummy...");

    // 3. Template dummy de 512 bytes (simula plantilla real)
    _lastTemplate.assign(512, 0xAB);

    if (_callback) _callback(true, _lastTemplate, 0, "OK");
    return true;
}

const std::vector<uint8_t>& FingerprintManager::getLastTemplate() const
{
    return _lastTemplate;
}

uint16_t FingerprintManager::getLastFingerID() const
{
    return _lastFingerID;
}

void FingerprintManager::setResultCallback(ResultCallback cb)
{
    _callback = cb;
}
