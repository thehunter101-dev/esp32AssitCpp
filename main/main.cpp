#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "LCDI2C.hpp"
#include "RFIDManager.hpp"
#include "ServoManager.hpp"
#include "FingerprintManager.hpp"

static const char *TAG = "MAIN";

void testFingerprint(LCDI2C& lcd, FingerprintManager& fp)
{
    lcd.clean(2, 0);
    lcd.print("Test FP: Coloque  ", 2, 0);
    lcd.print("dedo en sensor... ", 3, 0);
    ESP_LOGI(TAG, "=== TEST FP: Coloque el dedo ===");

    int attempts = 30; // ~15 segundos a 500ms por intento
    while (attempts--) {
        if (fp.detectFinger()) {
            lcd.clean(2, 0);
            lcd.print("DEDO DETECTADO!   ", 2, 0);
            lcd.clean(3, 0);
            lcd.print("Capturando...     ", 3, 0);
            ESP_LOGI(TAG, "Dedo detectado! Capturando template...");

            if (fp.captureTemplate()) {
                const auto& tmpl = fp.getLastTemplate();
                lcd.clean(2, 0);
                lcd.print("FP CAPTURADA OK!  ", 2, 0);
                lcd.clean(3, 0);
                char buf[16];
                snprintf(buf, sizeof(buf), "Tam: %d B", (int)tmpl.size());
                lcd.print(buf, 3, 0);
                ESP_LOGI(TAG, "Template capturado: %d bytes", (int)tmpl.size());
            } else {
                lcd.clean(2, 0);
                lcd.print("Fallo captura tmpl", 2, 0);
            }

            vTaskDelay(pdMS_TO_TICKS(3000));
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    lcd.clean(2, 0);
    lcd.print("TEST FP: Timeout  ", 2, 0);
    lcd.clean(3, 0);
    ESP_LOGW(TAG, "TEST FP: Timeout - No se detecto dedo");
    vTaskDelay(pdMS_TO_TICKS(2000));
}

extern "C" void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(ret);

    LCDI2C lcd;
    lcd.init();
    lcd.print("Inicializando...  ", 0, 0);

    ServoManager servo(GPIO_NUM_13);
    if (!servo.init()) {
        lcd.print("Servo init fail   ", 1, 0);
        ESP_LOGE(TAG, "Error al inicializar servo");
        return;
    }

    RFIDManager rfid(
        GPIO_NUM_23,
        GPIO_NUM_19,
        GPIO_NUM_18,
        GPIO_NUM_5,
        GPIO_NUM_4,
        SPI3_HOST
    );

    FingerprintManager fingerprint(
        GPIO_NUM_12,  // TX (ESP32 -> AS608 RX)
        GPIO_NUM_14,  // RX (ESP32 <- AS608 TX)
        UART_NUM_1
    );

    if (!fingerprint.init()) {
        lcd.print("FP init fail      ", 1, 0);
        ESP_LOGE(TAG, "Error al inicializar FingerprintManager");
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(1000)); // estabilizacion del sensor

    // --- TEST DE HUELLA ---
    testFingerprint(lcd, fingerprint);

    // --- FLUJO PRINCIPAL ---
    lcd.clean(1, 0);
    lcd.clean(2, 0);
    lcd.clean(3, 0);

    bool tagActivo = false;

    rfid.setTagCallback([&lcd, &servo, &fingerprint, &tagActivo](bool tagPresent, const rc522_picc_t* picc) {
        if (tagPresent && !tagActivo) {
            tagActivo = true;

            lcd.clean(2, 0);
            lcd.print("Tag cerca!        ", 2, 0);
            lcd.clean(3, 0);

            char uidStr[RC522_PICC_UID_STR_BUFFER_SIZE_MAX] = {};
            rc522_picc_uid_to_str(&picc->uid, uidStr, sizeof(uidStr));
            lcd.print(uidStr, 3, 0);

            ESP_LOGI(TAG, "Tag cerca! UID: %s", uidStr);

            servo.turnRight(30);
            lcd.print("Acerca tu dedo!   ", 2, 0);
            lcd.clean(3, 0);

            if (fingerprint.captureTemplate()) {
                const auto& tmpl = fingerprint.getLastTemplate();
                lcd.print("Huella OK!        ", 3, 0);
                ESP_LOGI(TAG, "Huella capturada - Tam: %d bytes", (int)tmpl.size());
            } else {
                lcd.print("FP Fallo/Timeout  ", 3, 0);
                ESP_LOGW(TAG, "Fallo captura de huella");
            }

        } else if (!tagPresent && tagActivo) {
            tagActivo = false;

            lcd.clean(2, 0);
            lcd.print("Tag no detectado  ", 2, 0);
            lcd.clean(3, 0);

            ESP_LOGI(TAG, "Tag no detectado");
            servo.setAngle(90);
        }
    });

    if (!rfid.init()) {
        lcd.print("RFID init fail    ", 1, 0);
        ESP_LOGE(TAG, "Error al inicializar RFIDManager");
        return;
    }

    lcd.clean(1, 0);
    lcd.print("RFID OK!          ", 1, 0);
    lcd.clean(2, 0);
    lcd.print("Tag no detectado  ", 2, 0);
    ESP_LOGI(TAG, "Sistema listo.");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
