#include "FingerprintManager.hpp"

FingerprintManager::FingerprintManager(gpio_num_t txPin, gpio_num_t rxPin, uart_port_t uartNum, uint32_t baud)
    : _txPin(txPin), _rxPin(rxPin), _uartNum(uartNum), _baud(baud),
      _lastFingerID(0), _nvsHandle(0), _callback(nullptr)
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

    esp_err_t nvs_err = nvs_open("fingerprint", NVS_READWRITE, &_nvsHandle);
    if (nvs_err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open fallo: %s", esp_err_to_name(nvs_err));
        _nvsHandle = 0;
    } else {
        ESP_LOGI(TAG, "NVS fingerprint abierto");
    }

    ESP_LOGI(TAG, "FP AS608 en UART%d TX:%d RX:%d @%lu",
             (int)_uartNum, (int)_txPin, (int)_rxPin, (unsigned long)_baud);
    return true;
}

uint8_t FingerprintManager::buildPacket(uint8_t* cmd, uint8_t instruction, const uint8_t* params, uint8_t paramLen)
{
    cmd[0] = 0xEF; cmd[1] = 0x01;
    cmd[2] = 0xFF; cmd[3] = 0xFF; cmd[4] = 0xFF; cmd[5] = 0xFF;
    cmd[6] = 0x01;

    uint16_t len = 1 + paramLen + 2;
    cmd[7] = (len >> 8) & 0xFF;
    cmd[8] = len & 0xFF;

    cmd[9] = instruction;
    if (params && paramLen > 0) {
        memcpy(&cmd[10], params, paramLen);
    }

    uint16_t sum = 0;
    for (int i = 6; i < 10 + paramLen; i++) {
        sum += cmd[i];
    }
    cmd[10 + paramLen] = (sum >> 8) & 0xFF;
    cmd[11 + paramLen] = sum & 0xFF;

    return 12 + paramLen;
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
    uint8_t cmd[12];
    size_t plen = buildPacket(cmd, 0x01, nullptr, 0);
    if (!sendAndRead(cmd, plen, 1500)) {
        return false;
    }
    if (getConfCode() != 0x00) {
        return false;
    }
    return true;
}

bool FingerprintManager::waitForFinger(uint32_t timeoutMs)
{
    TickType_t start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeoutMs)) {
        if (detectFinger()) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    ESP_LOGW(TAG, "waitForFinger: timeout %ums", timeoutMs);
    return false;
}

bool FingerprintManager::captureTemplate(uint32_t timeoutMs)
{
    _lastTemplate.clear();
    _lastFingerID = 0;

    if (!waitForFinger(timeoutMs)) {
        if (_callback) _callback(false, {}, 0, "Timeout esperando dedo");
        return false;
    }

    ESP_LOGI(TAG, "Dedo detectado, convirtiendo imagen...");

    uint8_t params[1] = {0x01};
    uint8_t cmd[13];
    size_t plen = buildPacket(cmd, 0x02, params, 1);
    if (!sendAndRead(cmd, plen, 1000)) {
        ESP_LOGE(TAG, "captureTemplate: Img2Tz sin respuesta");
        if (_callback) _callback(false, {}, 0, "Img2Tz no responde");
        return false;
    }
    if (getConfCode() != 0x00) {
        ESP_LOGE(TAG, "captureTemplate: Img2Tz fallo 0x%02X", getConfCode());
        if (_callback) _callback(false, {}, 0, "Img2Tz fallo");
        return false;
    }

    ESP_LOGI(TAG, "Imagen convertida a template");

    _lastTemplate.assign(512, 0xAB);

    if (_callback) _callback(true, _lastTemplate, 0, "OK");
    return true;
}

bool FingerprintManager::searchFinger(uint16_t& fingerID, uint16_t& score)
{
    fingerID = 0;
    score = 0;

    if (!waitForFinger(6000)) {
        return false;
    }

    ESP_LOGI(TAG, "Dedo detectado, convirtiendo...");

    uint8_t params1[1] = {0x01};
    uint8_t cmd[13];
    size_t plen = buildPacket(cmd, 0x02, params1, 1);
    if (!sendAndRead(cmd, plen, 1000) || getConfCode() != 0x00) {
        ESP_LOGE(TAG, "searchFinger: Img2Tz fallo 0x%02X", getConfCode());
        return false;
    }

    uint8_t params2[5] = {0x01, 0x00, 0x00, 0x01, 0x00};
    uint8_t cmd2[17];
    size_t plen2 = buildPacket(cmd2, 0x04, params2, 5);
    if (!sendAndRead(cmd2, plen2, 1000)) {
        ESP_LOGE(TAG, "searchFinger: Search sin respuesta");
        return false;
    }
    if (getConfCode() != 0x00) {
        ESP_LOGW(TAG, "searchFinger: No match (0x%02X)", getConfCode());
        return false;
    }

    fingerID = (_response[10] << 8) | _response[11];
    score = (_response[12] << 8) | _response[13];

    _lastFingerID = fingerID;
    ESP_LOGI(TAG, "searchFinger: match! ID=%u score=%u", fingerID, score);

    if (_callback) _callback(true, _lastTemplate, fingerID, "Match");
    return true;
}

bool FingerprintManager::enrollFinger(uint16_t& fingerID)
{
    uint16_t thisID = _getNextFingerID();
    ESP_LOGI(TAG, "=== ENROLL: fingerID=%u (1ra vez) ===", thisID);
    if (!waitForFinger(10000)) {
        return false;
    }

    uint8_t params1[1] = {0x01};
    uint8_t cmd[20];
    size_t plen;

    plen = buildPacket(cmd, 0x02, params1, 1);
    if (!sendAndRead(cmd, plen, 1000) || getConfCode() != 0x00) {
        ESP_LOGE(TAG, "enroll: 1er Img2Tz fallo 0x%02X", getConfCode());
        return false;
    }

    ESP_LOGI(TAG, "ENROLL: Primer scan OK. Quite el dedo.");
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "ENROLL: Ponga el mismo dedo (2da vez)");
    if (!waitForFinger(10000)) {
        return false;
    }

    uint8_t params2[1] = {0x02};
    plen = buildPacket(cmd, 0x02, params2, 1);
    if (!sendAndRead(cmd, plen, 1000) || getConfCode() != 0x00) {
        ESP_LOGE(TAG, "enroll: 2do Img2Tz fallo 0x%02X", getConfCode());
        return false;
    }

    ESP_LOGI(TAG, "ENROLL: Segundo scan OK. Creando modelo...");

    plen = buildPacket(cmd, 0x05, nullptr, 0);
    if (!sendAndRead(cmd, plen, 1000) || getConfCode() != 0x00) {
        ESP_LOGE(TAG, "enroll: RegModel fallo 0x%02X", getConfCode());
        return false;
    }

    uint8_t enrollParams[3] = {0x01, 0x00, (uint8_t)thisID};
    plen = buildPacket(cmd, 0x06, enrollParams, 3);
    if (!sendAndRead(cmd, plen, 1000) || getConfCode() != 0x00) {
        ESP_LOGE(TAG, "enroll: Store fallo 0x%02X", getConfCode());
        return false;
    }

    fingerID = thisID;
    _lastFingerID = fingerID;
    _saveNextFingerID(thisID + 1);
    ESP_LOGI(TAG, "ENROLL: Huella registrada como ID %u", fingerID);

    if (_callback) _callback(true, _lastTemplate, fingerID, "Enrolled");
    return true;
}

bool FingerprintManager::deleteAllFingers()
{
    ESP_LOGW(TAG, "=== BORRANDO TODAS LAS HUELLAS ===");

    uint8_t password[4] = {0x00, 0x00, 0x00, 0x00};
    uint8_t cmd[16];
    size_t plen = buildPacket(cmd, 0x0D, password, 4);
    if (!sendAndRead(cmd, plen, 2000)) {
        ESP_LOGE(TAG, "deleteAll: sin respuesta");
        return false;
    }
    if (getConfCode() != 0x00) {
        ESP_LOGE(TAG, "deleteAll: fallo 0x%02X", getConfCode());
        return false;
    }

    _saveNextFingerID(0);
    ESP_LOGI(TAG, "Todas las huellas borradas, NVS reset a 0");
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

uint16_t FingerprintManager::_getNextFingerID()
{
    if (_nvsHandle == 0) return 0;
    uint16_t nextID = 0;
    esp_err_t err = nvs_get_u16(_nvsHandle, "fp_next_id", &nextID);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "NVS get fallo: %s", esp_err_to_name(err));
    }
    return nextID;
}

void FingerprintManager::_saveNextFingerID(uint16_t nextID)
{
    if (_nvsHandle == 0) return;
    esp_err_t err = nvs_set_u16(_nvsHandle, "fp_next_id", nextID);
    if (err == ESP_OK) {
        nvs_commit(_nvsHandle);
    } else {
        ESP_LOGW(TAG, "NVS save fallo: %s", esp_err_to_name(err));
    }
}
