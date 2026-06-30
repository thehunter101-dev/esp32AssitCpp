#ifndef RFIDMANAGER_H_
#define RFIDMANAGER_H_

#include <cstdint>
#include <functional>
#include <string>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "rc522.h"
#include "driver/rc522_spi.h"
#include "rc522_picc.h"

class RFIDManager {
public:
    using TagCallback = std::function<void(bool tagPresent, const rc522_picc_t* picc)>;

    RFIDManager(gpio_num_t mosi, gpio_num_t miso, gpio_num_t sclk,
                gpio_num_t cs, gpio_num_t rst,
                spi_host_device_t host = SPI3_HOST);
    ~RFIDManager();

    bool init();
    bool isTagPresent() const;
    void setTagCallback(TagCallback callback);
    std::string getLastUID() const;

private:
    gpio_num_t _mosi;
    gpio_num_t _miso;
    gpio_num_t _sclk;
    gpio_num_t _cs;
    gpio_num_t _rst;
    spi_host_device_t _host;

    rc522_driver_handle_t _driver;
    rc522_handle_t _scanner;
    bool _tagPresent;
    rc522_picc_t _lastPicc;
    TagCallback _callback;

    static constexpr const char* TAG = "RFID_MGR";

    static void _eventBridge(void* arg, esp_event_base_t base, int32_t event_id, void* data);
    void _handleEvent(rc522_picc_state_changed_event_t* event);
};

#endif
