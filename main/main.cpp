#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "LCDI2C.hpp"
#include "RFIDManager.hpp"
#include "ServoManager.hpp"

static const char *TAG = "MAIN";

extern "C" void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(ret);

    LCDI2C lcd;
    lcd.init();
    lcd.print("RFID RC522 Test", 0, 0);

    ServoManager servo(GPIO_NUM_13);
    if (!servo.init()) {
        lcd.print("Servo init fail ", 1, 0);
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

    bool tagActivo = false;

    rfid.setTagCallback([&lcd, &servo, &tagActivo](bool tagPresent, const rc522_picc_t* picc) {
        if (tagPresent && !tagActivo) {
            tagActivo = true;

            lcd.clean(2, 0);
            lcd.print("Tag cerca!       ", 2, 0);
            lcd.clean(3, 0);
            lcd.print("UID:             ", 3, 0);

            char uidStr[RC522_PICC_UID_STR_BUFFER_SIZE_MAX] = {};
            rc522_picc_uid_to_str(&picc->uid, uidStr, sizeof(uidStr));
            lcd.print(uidStr, 3, 5);

            ESP_LOGI(TAG, "Tag cerca! UID: %s", uidStr);

            servo.turnRight(30);

        } else if (!tagPresent && tagActivo) {
            tagActivo = false;

            lcd.clean(2, 0);
            lcd.print("Tag no detectado ", 2, 0);
            lcd.clean(3, 0);

            ESP_LOGI(TAG, "Tag no detectado");

            servo.setAngle(90);
        }
    });

    if (!rfid.init()) {
        lcd.print("RFID init fail  ", 1, 0);
        ESP_LOGE(TAG, "Error al inicializar RFIDManager");
        return;
    }

    lcd.print("RFID OK!        ", 1, 0);
    lcd.print("Tag no detectado ", 2, 0);
    ESP_LOGI(TAG, "Sistema listo. Acerca un tag RFID...");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
