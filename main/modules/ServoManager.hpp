#ifndef SERVOMANAGER_H_
#define SERVOMANAGER_H_

#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"

class ServoManager {
public:
    ServoManager(gpio_num_t pin,
                 ledc_channel_t channel = LEDC_CHANNEL_0,
                 ledc_timer_t timer = LEDC_TIMER_0);

    bool init();
    void setAngle(uint8_t angle);
    void setAngleSmooth(uint8_t angle, uint16_t stepDelayMs = 15);
    void turnRight(uint8_t degrees);
    void turnLeft(uint8_t degrees);
    uint8_t getAngle() const;

private:
    gpio_num_t _pin;
    ledc_channel_t _channel;
    ledc_timer_t _timer;
    uint8_t _currentAngle;

    static constexpr const char* TAG = "SERVO_MGR";

    static constexpr uint32_t MIN_DUTY = 410;
    static constexpr uint32_t MAX_DUTY = 2048;
};

#endif
