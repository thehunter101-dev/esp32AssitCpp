#include "Indicators.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

Indicators::Indicators(gpio_num_t greenLed, gpio_num_t redLed, gpio_num_t buzzer)
    : _green(greenLed), _red(redLed), _buzzer(buzzer)
{
}

void Indicators::init()
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << _green) | (1ULL << _red) | (1ULL << _buzzer);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    off();
    ESP_LOGI(TAG, "Indicators: green=%d red=%d buzzer=%d", _green, _red, _buzzer);
}

void Indicators::granted()
{
    gpio_set_level(_green, 1);
    gpio_set_level(_red, 0);
    beep(100);
    gpio_set_level(_buzzer, 0);
}

void Indicators::denied()
{
    gpio_set_level(_green, 0);
    gpio_set_level(_red, 1);
    beep(500);
    gpio_set_level(_buzzer, 0);
}

void Indicators::registered()
{
    gpio_set_level(_green, 1);
    gpio_set_level(_red, 0);
    beep(80);
    vTaskDelay(pdMS_TO_TICKS(100));
    beep(80);
    gpio_set_level(_buzzer, 0);
}

void Indicators::cardScanned()
{
    beep(60);
}

void Indicators::off()
{
    gpio_set_level(_green, 0);
    gpio_set_level(_red, 0);
    gpio_set_level(_buzzer, 0);
}

void Indicators::beep(uint32_t ms)
{
    gpio_set_level(_buzzer, 1);
    vTaskDelay(pdMS_TO_TICKS(ms));
    gpio_set_level(_buzzer, 0);
}
