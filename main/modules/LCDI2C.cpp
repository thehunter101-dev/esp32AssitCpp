#include "LCDI2C.hpp"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "LCD_2004";

LCDI2C::LCDI2C(gpio_num_t sdaPin, gpio_num_t sclPin, uint8_t address){
  _sdaPin = sdaPin;
  _sclPin = sclPin;
  _address = address;
}

void LCDI2C::_sendNibble(uint8_t nibble, uint8_t mode){
    uint8_t data = (nibble & 0xF0) | mode | LCD_BACKLIGHT;

    uint8_t buf[3];
    buf[0] = data | LCD_ENABLE;   // Pulso alto en Enable
    buf[1] = data | LCD_ENABLE;   // Mantener
    buf[2] = data & ~LCD_ENABLE;  // Pulso bajo en Enable (aquí el LCD lee los datos)
    i2c_master_transmit(_dev_handle, buf, 3, 1000);
}
void LCDI2C::_sendByte(uint8_t byte, uint8_t mode){
    _sendNibble(byte & 0xF0, mode);// Parte alta
    _sendNibble((byte << 4) & 0xF0, mode); // Parte baja
}
void LCDI2C::_command(uint8_t cmd){
  _sendByte(cmd, 0);
}
void LCDI2C::_char(char data){
  _sendByte(data, LCD_REG_SELECT);
}

void LCDI2C::init(){
    i2c_master_bus_config_t bus_config = {};
    bus_config.i2c_port = I2C_NUM_0;
    bus_config.sda_io_num = _sdaPin;
    bus_config.scl_io_num = _sclPin;
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.flags.enable_internal_pullup = true;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &_bus_handle));

    i2c_device_config_t dev_config = {};
    dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_config.device_address = _address;
    dev_config.scl_speed_hz = 100000; // 100 kHz estándar para pantallas LCD

    ESP_ERROR_CHECK(i2c_master_bus_add_device(_bus_handle, &dev_config, &_dev_handle));
    ESP_LOGI(TAG, "Escribiendo texto en pantalla...");

    // 1. Esperar un tiempo largo a que el LCD se estabilice eléctricamente
    vTaskDelay(pdMS_TO_TICKS(100));

    // 2. Secuencia de reinicio por software forzado (Protocolo HD44780)
    _sendNibble(0x30, 0);
    vTaskDelay(pdMS_TO_TICKS(10)); // Delay largo obligatorio

    _sendNibble(0x30, 0);
    vTaskDelay(pdMS_TO_TICKS(5));

    _sendNibble(0x30, 0);
    vTaskDelay(pdMS_TO_TICKS(5));

    // 3. Activar oficialmente el modo de 4 bits
    _sendNibble(0x20, 0);
    vTaskDelay(pdMS_TO_TICKS(5));

    // 4. Comandos de configuración con delays entre ellos
    _command(0x28); // Modo 4 bits, 2/4 líneas, fuente 5x8
    vTaskDelay(pdMS_TO_TICKS(2));

    _command(0x0C); // Encender pantalla, apagar cursor, apagar parpadeo
    vTaskDelay(pdMS_TO_TICKS(2));

    _command(0x06); // Dirección del cursor (Izquierda a Derecha)
    vTaskDelay(pdMS_TO_TICKS(2));

    _command(0x01); // Limpiar pantalla (Borra los cuadros negros iniciales)
    vTaskDelay(pdMS_TO_TICKS(20));
}

void LCDI2C::setCursor(uint8_t row, uint8_t column){
    uint8_t row_offsets[] = { 0x00, 0x40, 0x14, 0x54 }; // Direcciones DDRAM para LCD 2004
    if (row > 3) row = 3;
    _command(0x80 | (column + row_offsets[row]));
}

void LCDI2C::print(const char *str, uint8_t row, uint8_t column){
  setCursor(row, column);
  while (*str){
    _char(*str++);
  }
}


void LCDI2C::clean(uint8_t row, uint8_t column){
  setCursor(row, column);
  print("                    ",row,column);
}
