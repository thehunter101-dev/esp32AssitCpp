#ifndef RFIDMANAGER_H_
#define RFIDMANAGER_H_

#include <cstdint>
#include <functional>
#include <string>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "rc522.h"

class RFIDManager {
public:
    using TagCallback = std::function<void(uint64_t serialNumber)>;

    RFIDManager(gpio_num_t mosi, gpio_num_t miso, gpio_num_t sclk,
                gpio_num_t cs, spi_host_device_t host = SPI3_HOST);
    ~RFIDManager();

    bool init();

    bool isNewTagScanned();
    void getLastSerial(uint8_t* uid, uint8_t& uidLen) const;
    std::string getLastUIDStr() const;

    bool readMifareBlock(uint8_t blockAddr, uint8_t* data, uint8_t dataLen);

    void setTagCallback(TagCallback callback);

private:
    gpio_num_t _mosi, _miso, _sclk, _cs;
    spi_host_device_t _host;

    rc522_handle_t _scanner;
    bool _newTagScanned;
    uint64_t _lastSerialNumber;
    uint8_t _lastSerialSize;
    TagCallback _callback;

    static constexpr const char* TAG = "RFID_MGR";

    static void _eventBridge(void* arg, esp_event_base_t base, int32_t event_id, void* data);
};

#endif
