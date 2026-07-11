#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include <time.h>
#include "LCDI2C.hpp"
#include "ServoManager.hpp"
#include "FingerprintManager.hpp"
#include "RFIDManager.hpp"
#include "WifiDriver.hpp"
#include "AccessControl.hpp"
#include "AccessDB.hpp"
#include "Indicators.hpp"
#include "secrets.h"

static const char *TAG = "MAIN";

extern "C" void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    LCDI2C lcd;
    lcd.init();
    lcd.print("Inicializando...  ", 0, 0);

    gpio_config_t bootBtn = {};
    bootBtn.pin_bit_mask = (1ULL << GPIO_NUM_0);
    bootBtn.mode = GPIO_MODE_INPUT;
    bootBtn.pull_up_en = GPIO_PULLUP_ENABLE;
    bootBtn.pull_down_en = GPIO_PULLDOWN_DISABLE;
    bootBtn.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&bootBtn);

    ServoManager servo(GPIO_NUM_13);  // SG90 a D13
    if (!servo.init()) {
        lcd.print("Servo init fail   ", 1, 0);
        ESP_LOGE(TAG, "Error al inicializar servo");
        return;
    }

    RFIDManager rfid(
        GPIO_NUM_23,  // MOSI
        GPIO_NUM_19,  // MISO
        GPIO_NUM_18,  // SCK
        GPIO_NUM_5,   // CS (SDA)
        SPI3_HOST
    );
    if (!rfid.init()) {
        lcd.print("RFID init fail    ", 1, 0);
        ESP_LOGE(TAG, "Error al inicializar RFID");
        return;
    }

    FingerprintManager fingerprint(
        GPIO_NUM_17,  // TX2 (ESP→FP)
        GPIO_NUM_16,  // RX2 (ESP←FP)
        UART_NUM_2,
        57600
    );
    if (!fingerprint.init()) {
        lcd.print("FP init fail      ", 1, 0);
        ESP_LOGE(TAG, "Error al inicializar Fingerprint");
        return;
    }

    Indicators indicators(GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_25);  // verde D26, rojo D27, buzzer D25
    indicators.init();

    vTaskDelay(pdMS_TO_TICKS(500));
    lcd.clean(1, 0);
    lcd.print("Conectando WiFi...", 1, 0);

    WifiDriver wifi(WIFI_SSID, WIFI_PASS);
    wifi.init();
    if (!wifi.conect()) {
        lcd.print("WiFi FALLO!       ", 1, 0);
        ESP_LOGE(TAG, "WiFi no conectado — el sistema continuara sin red");
    } else {
        lcd.print("WiFi OK!          ", 1, 0);
    }

    lcd.clean(2, 0);
    lcd.print("Sync hora...      ", 2, 0);
    setenv("TZ", "COT5", 1);
    tzset();
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
    time_t now = 0;
    struct tm ti = {};
    int retries = 0;
    while (ti.tm_year < (2025 - 1900) && retries < 20) {
        vTaskDelay(pdMS_TO_TICKS(500));
        time(&now);
        localtime_r(&now, &ti);
        retries++;
    }
    lcd.clean(2, 0);
    if (ti.tm_year >= (2025 - 1900)) {
        char tbuf[16];
        strftime(tbuf, sizeof(tbuf), "%H:%M", &ti);
        lcd.print("Hora OK! ", 2, 0);
        lcd.print(tbuf, 2, 9);
        ESP_LOGI(TAG, "NTP sincronizado: %s", tbuf);
    } else {
        lcd.print("NTP fallo", 2, 0);
        ESP_LOGW(TAG, "NTP fallo — no se pudo sincronizar hora");
    }
    vTaskDelay(pdMS_TO_TICKS(1000));

    AccessDB accessDB(SUPABASE_URL, SUPABASE_KEY);
    AccessControl accessControl(lcd, servo, fingerprint, rfid, accessDB, indicators);

    bool registrationMode = false;
    bool lastWifiStatus = wifi.isConnected();

    lcd.clean(0, 0);
    lcd.clean(1, 0);
    lcd.clean(2, 0);
    lcd.clean(3, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd.putChar(LCDI2C::CUSTOM_UNLOCK, 0, 9);
    lcd.printCentered("SISTEMA LISTO", 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd.putChar(LCDI2C::CUSTOM_CARD, 2, 0);
    lcd.print(" BOOT=cambio", 2, 1);
    lcd.putChar(LCDI2C::CUSTOM_ARROW, 3, 0);
    lcd.print(" Login>", 3, 1);
    lcd.putChar(LCDI2C::CUSTOM_CARD, 3, 10);
    lcd.putChar(LCDI2C::CUSTOM_FINGER, 3, 12);
    if (lastWifiStatus) {
        accessControl.setWifiConnected(true);
    }
    ESP_LOGI(TAG, "Sistema listo. BOOT toggle modo.");
    vTaskDelay(pdMS_TO_TICKS(1500));

    while (true) {
        static uint64_t lastPoll = 0;
        uint64_t nowMs = esp_timer_get_time() / 1000;
        if (nowMs - lastPoll >= 5000) {
            lastPoll = nowMs;
            if (!registrationMode) {
                accessControl.checkRemoteCommands();
            }

            bool currentWifiStatus = wifi.isConnected();
            if (currentWifiStatus != lastWifiStatus) {
                lastWifiStatus = currentWifiStatus;
                accessControl.setWifiConnected(currentWifiStatus);
                if (!currentWifiStatus) {
                    ESP_LOGW(TAG, "WiFi desconectado, reconectando...");
                } else {
                    ESP_LOGI(TAG, "WiFi reconectado");
                    esp_sntp_init();
                }
            }
        }

        if (isBootToggled()) {
            accessControl.resetDisplay();
            registrationMode = !registrationMode;
            lcd.clean(0, 0);
            lcd.clean(1, 0);
            lcd.clean(2, 0);
            lcd.clean(3, 0);
            if (registrationMode) {
                lcd.putChar(LCDI2C::CUSTOM_CARD, 0, 0);
                lcd.print(" REGISTRO", 0, 1);
                lcd.putChar(LCDI2C::CUSTOM_ARROW, 1, 0);
                lcd.print(" Iniciar registro", 1, 1);
                lcd.putChar(LCDI2C::CUSTOM_FINGER, 2, 0);
                lcd.print(" BOOT 3s=borrar", 2, 1);
                lcd.putChar(LCDI2C::CUSTOM_ARROW, 3, 0);
                lcd.print(" BOOT=volver", 3, 1);

                int holdCount = 0;
                while (gpio_get_level(GPIO_NUM_0) == 0 && holdCount < 30) {
                    vTaskDelay(pdMS_TO_TICKS(100));
                    holdCount++;
                }
                if (holdCount >= 30) {
                    lcd.clean(0, 0);
                    lcd.clean(1, 0);
                    lcd.clean(2, 0);
                    lcd.clean(3, 0);
                    lcd.putChar(LCDI2C::CUSTOM_FINGER, 0, 9);
                    lcd.printCentered("BORRANDO", 1);
                    lcd.printCentered("huellas...", 2);
                    fingerprint.deleteAllFingers();
                    lcd.clean(0, 0);
                    lcd.clean(1, 0);
                    lcd.clean(2, 0);
                    lcd.clean(3, 0);
                    lcd.putChar(LCDI2C::CUSTOM_CHECK, 0, 9);
                    lcd.printCentered("HUELLAS BORRADAS", 1);
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    lcd.clean(0, 0);
                    lcd.clean(1, 0);
                    lcd.clean(2, 0);
                    lcd.clean(3, 0);
                    lcd.putChar(LCDI2C::CUSTOM_CARD, 0, 0);
                    lcd.print(" REGISTRO", 0, 1);
                    lcd.putChar(LCDI2C::CUSTOM_ARROW, 1, 0);
                    lcd.print(" Iniciar registro", 1, 1);
                    lcd.putChar(LCDI2C::CUSTOM_FINGER, 2, 0);
                    lcd.print(" BOOT 3s=borrar", 2, 1);
                    lcd.putChar(LCDI2C::CUSTOM_ARROW, 3, 0);
                    lcd.print(" BOOT=volver", 3, 1);
                }
            } else {
                lcd.putChar(LCDI2C::CUSTOM_CARD, 0, 0);
                lcd.print(" LOGIN", 0, 1);
                lcd.putChar(LCDI2C::CUSTOM_CARD, 1, 0);
                lcd.print(" Acerca tarjeta", 1, 1);
                lcd.putChar(LCDI2C::CUSTOM_ARROW, 2, 0);
                lcd.print(" BOOT=registro", 2, 1);
            }
            vTaskDelay(pdMS_TO_TICKS(800));
        }

        if (registrationMode) {
            accessControl.registrationFlow();
        } else {
            accessControl.startAccessFlow();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
