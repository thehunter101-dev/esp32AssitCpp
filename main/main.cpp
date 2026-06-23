#include <cstdio>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "LCDI2C.hpp"
static const char *TAG = "LCD_2004";

extern "C" void app_main() {
    ESP_LOGI(TAG, "Inicializando I2C Maestro...");
    LCDI2C lcd;
    ESP_LOGI(TAG, "Inicializando LCD 2004A...");
    lcd.init();


    // Imprimir el Hello World
    lcd.print("Como estamos",0,0);

    lcd.print("Holis Uwu",1,0);

    lcd.print("ya me dio hambre Owo",2,0);

    std::string basesaludo = "Contando hasta el: ";
    for (int i = 0; i<5;i++){
      std::string saludo = basesaludo + std::to_string(i);
          lcd.print(saludo.c_str(),3,0);
          vTaskDelay(pdMS_TO_TICKS(500));


    }

    lcd.clean(3,0);

    vTaskDelay(pdMS_TO_TICKS(500));
    lcd.print("Unu",3,0);
    while (true) {
        // El programa principal se queda aquí manteniendo el texto en pantalla
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
