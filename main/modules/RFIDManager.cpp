#include "RFIDManager.hpp"

RFIDManager::RFIDManager(gpio_num_t mosi, gpio_num_t miso, gpio_num_t sclk,
                         gpio_num_t cs, spi_host_device_t host)
    : _mosi(mosi), _miso(miso), _sclk(sclk), _cs(cs), _host(host),
      _scanner(nullptr), _newTagScanned(false), _lastSerialNumber(0), _lastSerialSize(0), _callback(nullptr)
{
}

RFIDManager::~RFIDManager()
{
    if (_scanner != nullptr) {
        rc522_destroy(_scanner);
        _scanner = nullptr;
    }
}

bool RFIDManager::init()
{
    rc522_config_t config = {
        .scan_interval_ms = 125,
        .task_stack_size = 4 * 1024,
        .task_priority = 4,
        .transport = RC522_TRANSPORT_SPI,
        .spi = {
            .host = _host,
            .miso_gpio = _miso,
            .mosi_gpio = _mosi,
            .sck_gpio = _sclk,
            .sda_gpio = _cs,
            .clock_speed_hz = 5000000,
            .device_flags = 0,
            .bus_is_initialized = false,
        },
    };

    esp_err_t ret = rc522_create(&config, &_scanner);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error creating RC522: %s", esp_err_to_name(ret));
        return false;
    }

    ret = rc522_register_events(_scanner, RC522_EVENT_TAG_SCANNED,
                                _eventBridge, this);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error registering events: %s", esp_err_to_name(ret));
        return false;
    }

    ret = rc522_start(_scanner);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error starting scanner: %s", esp_err_to_name(ret));
        return false;
    }

    ESP_LOGI(TAG, "RFIDManager initialized");
    return true;
}

bool RFIDManager::isNewTagScanned()
{
    bool ret = _newTagScanned;
    _newTagScanned = false;
    return ret;
}

void RFIDManager::getLastSerial(uint8_t* uid, uint8_t& uidLen) const
{
    uint8_t size = _lastSerialSize;
    if (size == 0 || size > 7) size = 4;
    uidLen = size;
    for (int i = 0; i < size; i++) {
        uid[i] = (_lastSerialNumber >> (8 * (size - 1 - i))) & 0xFF;
    }
}

std::string RFIDManager::getLastUIDStr() const
{
    char buf[17];
    snprintf(buf, sizeof(buf), "%016llX", (unsigned long long)_lastSerialNumber);
    return std::string(buf);
}

bool RFIDManager::readMifareBlock(uint8_t blockAddr, uint8_t* data, uint8_t dataLen)
{
    if (!_scanner || !data || dataLen < 16) return false;

    // Build uid array from stored serial number
    uint8_t uid[4] = {};
    uint8_t size = _lastSerialSize;
    if (size == 0 || size > 4) size = 4;
    for (int i = 0; i < size; i++) {
        uid[i] = (_lastSerialNumber >> (8 * (size - 1 - i))) & 0xFF;
    }

    // Default MIFARE key (transport config)
    const uint8_t defaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    esp_err_t err = rc522_mifare_read_block(_scanner, blockAddr,
                                            defaultKey, uid, size, data);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MIFARE read block %u fail: %s", blockAddr, esp_err_to_name(err));
        return false;
    }
    return true;
}

void RFIDManager::setTagCallback(TagCallback callback)
{
    _callback = callback;
}

void RFIDManager::_eventBridge(void* arg, esp_event_base_t base,
                               int32_t event_id, void* data)
{
    auto* self = static_cast<RFIDManager*>(arg);
    auto* event_data = static_cast<rc522_event_data_t*>(data);
    auto* tag = static_cast<rc522_tag_t*>(event_data->ptr);

    self->_newTagScanned = true;
    self->_lastSerialNumber = tag->serial_number;
    self->_lastSerialSize = 4;

    ESP_LOGI(self->TAG, "Tag scanned: serial=%016llX",
             (unsigned long long)tag->serial_number);

    if (self->_callback) {
        self->_callback(tag->serial_number);
    }
}
