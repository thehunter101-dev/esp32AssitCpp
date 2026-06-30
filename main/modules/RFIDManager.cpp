#include "RFIDManager.hpp"

RFIDManager::RFIDManager(gpio_num_t mosi, gpio_num_t miso, gpio_num_t sclk,
                         gpio_num_t cs, gpio_num_t rst, spi_host_device_t host)
    : _mosi(mosi), _miso(miso), _sclk(sclk), _cs(cs), _rst(rst), _host(host),
      _driver(nullptr), _scanner(nullptr), _tagPresent(false), _callback(nullptr)
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
    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num = _mosi;
    bus_cfg.miso_io_num = _miso;
    bus_cfg.sclk_io_num = _sclk;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;

    spi_device_interface_config_t dev_cfg = {};
    dev_cfg.mode = 0;
    dev_cfg.clock_speed_hz = 4 * 1000 * 1000;
    dev_cfg.spics_io_num = _cs;
    dev_cfg.queue_size = 7;

    rc522_spi_config_t driver_config = {};
    driver_config.host_id = _host;
    driver_config.bus_config = &bus_cfg;
    driver_config.dev_config = dev_cfg;
    driver_config.dma_chan = SPI_DMA_DISABLED;
    driver_config.rst_io_num = _rst;

    esp_err_t ret = rc522_spi_create(&driver_config, &_driver);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al crear driver SPI: %s", esp_err_to_name(ret));
        return false;
    }

    ret = rc522_driver_install(_driver);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al instalar driver: %s", esp_err_to_name(ret));
        return false;
    }

    rc522_config_t scanner_config = {};
    scanner_config.driver = _driver;

    ret = rc522_create(&scanner_config, &_scanner);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al crear scanner: %s", esp_err_to_name(ret));
        return false;
    }

    ret = rc522_register_events(_scanner, RC522_EVENT_PICC_STATE_CHANGED,
                                _eventBridge, this);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al registrar eventos: %s", esp_err_to_name(ret));
        return false;
    }

    ret = rc522_start(_scanner);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al iniciar scanner: %s", esp_err_to_name(ret));
        return false;
    }

    ESP_LOGI(TAG, "RFIDManager inicializado correctamente");
    return true;
}

bool RFIDManager::isTagPresent() const
{
    return _tagPresent;
}

void RFIDManager::setTagCallback(TagCallback callback)
{
    _callback = callback;
}

std::string RFIDManager::getLastUID() const
{
    char buf[RC522_PICC_UID_STR_BUFFER_SIZE_MAX] = {};
    rc522_picc_uid_to_str(&_lastPicc.uid, buf, sizeof(buf));
    return std::string(buf);
}

void RFIDManager::_eventBridge(void* arg, esp_event_base_t base,
                               int32_t event_id, void* data)
{
    auto* self = static_cast<RFIDManager*>(arg);
    auto* event = static_cast<rc522_picc_state_changed_event_t*>(data);
    self->_handleEvent(event);
}

void RFIDManager::_handleEvent(rc522_picc_state_changed_event_t* event)
{
    rc522_picc_t* picc = event->picc;

    if (picc->state == RC522_PICC_STATE_ACTIVE && event->old_state < RC522_PICC_STATE_ACTIVE) {
        _tagPresent = true;
        _lastPicc = *picc;
        rc522_picc_print(picc);
        ESP_LOGI(TAG, "Tag detectado");

    } else if (picc->state == RC522_PICC_STATE_IDLE && event->old_state >= RC522_PICC_STATE_ACTIVE) {
        _tagPresent = false;
        ESP_LOGI(TAG, "Tag removido");
    }

    if (_callback) {
        _callback(_tagPresent, picc);
    }
}
