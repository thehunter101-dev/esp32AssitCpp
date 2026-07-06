#ifndef INDICATORS_HPP_
#define INDICATORS_HPP_

#include "driver/gpio.h"
#include "esp_log.h"

class Indicators {
public:
    Indicators(gpio_num_t greenLed, gpio_num_t redLed, gpio_num_t buzzer);
    void init();
    void granted();
    void denied();
    void registered();
    void cardScanned();
    void off();

private:
    gpio_num_t _green, _red, _buzzer;

    static constexpr const char* TAG = "INDICATORS";
    void beep(uint32_t ms);
};

#endif
