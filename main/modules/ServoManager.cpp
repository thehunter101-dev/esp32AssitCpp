#include "ServoManager.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

ServoManager::ServoManager(gpio_num_t pin, ledc_channel_t channel, ledc_timer_t timer)
    : _pin(pin), _channel(channel), _timer(timer), _currentAngle(140) {}

bool ServoManager::init()
{
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_timer.duty_resolution = LEDC_TIMER_14_BIT;
    ledc_timer.timer_num = _timer;
    ledc_timer.freq_hz = 50;
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;

    if (ledc_timer_config(&ledc_timer) != ESP_OK) {
        ESP_LOGE(TAG, "Error al inicializar temporizador LEDC");
        return false;
    }

    ledc_channel_config_t ledc_channel = {};
    ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_channel.channel = _channel;
    ledc_channel.timer_sel = _timer;
    ledc_channel.intr_type = LEDC_INTR_DISABLE;
    ledc_channel.gpio_num = _pin;
    ledc_channel.duty = 1696;
    ledc_channel.hpoint = 0;

    if (ledc_channel_config(&ledc_channel) != ESP_OK) {
        ESP_LOGE(TAG, "Error al configurar canal LEDC");
        return false;
    }

    setAngle(140);
    ESP_LOGI(TAG, "Servo inicializado en GPIO %d", _pin);
    return true;
}

void ServoManager::setAngle(uint8_t angle)
{
    if (angle > 180) angle = 180;
    _currentAngle = angle;

    uint32_t duty = MIN_DUTY + ((MAX_DUTY - MIN_DUTY) * angle) / 180;

    ledc_set_duty(LEDC_LOW_SPEED_MODE, _channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, _channel);
}

void ServoManager::setAngleSmooth(uint8_t angle, uint16_t stepDelayMs)
{
    if (angle > 180) angle = 180;
    int start = _currentAngle;
    int end = angle;
    int step = (end > start) ? 1 : -1;

    while (start != end) {
        start += step;
        uint32_t duty = MIN_DUTY + ((MAX_DUTY - MIN_DUTY) * start) / 180;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, _channel, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, _channel);
        vTaskDelay(pdMS_TO_TICKS(stepDelayMs));
    }
    _currentAngle = angle;
}

void ServoManager::turnRight(uint8_t degrees)
{
    int target = _currentAngle - degrees;
    if (target < 0) target = 0;
    setAngle(static_cast<uint8_t>(target));
}

void ServoManager::turnLeft(uint8_t degrees)
{
    int target = _currentAngle + degrees;
    if (target > 180) target = 180;
    setAngle(static_cast<uint8_t>(target));
}

uint8_t ServoManager::getAngle() const
{
    return _currentAngle;
}
