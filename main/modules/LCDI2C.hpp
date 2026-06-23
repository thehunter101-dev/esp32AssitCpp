#ifndef LCDI2C_H_
#define LCDI2C_H_
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

class LCDI2C{
  public:
  LCDI2C(gpio_num_t sdaPin = GPIO_NUM_21, gpio_num_t sclPin = GPIO_NUM_22, uint8_t address = 0x27);
  void init();
  void setCursor(uint8_t row, uint8_t column);
  void print(const char *str,uint8_t row, uint8_t column);
  void clean(uint8_t row, uint8_t column);
  private:
  gpio_num_t _sdaPin;
  gpio_num_t _sclPin;
  uint8_t _address;

  i2c_master_bus_handle_t _bus_handle;
  i2c_master_dev_handle_t _dev_handle;

  static constexpr uint8_t LCD_BACKLIGHT = 0x08;
  static constexpr uint8_t LCD_ENABLE = 0x04;
  static constexpr uint8_t LCD_REG_SELECT = 0x01;
  void _sendNibble(uint8_t nibble, uint8_t mode);
  void _sendByte(uint8_t byte, uint8_t mode);
  void _command(uint8_t cmd);
  void _char(char data);
};


#endif // LCDI2C_H_
