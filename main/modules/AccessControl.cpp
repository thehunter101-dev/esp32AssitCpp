#include "AccessControl.hpp"
#include "esp_timer.h"
#include "driver/gpio.h"
#include <cstdio>

static bool g_bootToggled = false;

static bool bootToggleDetected()
{
    static bool prev = true;
    bool cur = gpio_get_level(GPIO_NUM_0) == 0;
    bool press = !prev && cur;
    prev = cur;
    if (press) g_bootToggled = true;
    return press;
}

bool isBootToggled()
{
    if (g_bootToggled) {
        g_bootToggled = false;
        return true;
    }
    return false;
}

AccessControl::AccessControl(LCDI2C& lcd, ServoManager& servo,
                             FingerprintManager& fingerprint, RFIDManager& rfid,
                             AccessDB& db, Indicators& indicators)
    : _lcd(lcd), _servo(servo), _fingerprint(fingerprint), _rfid(rfid), _db(db),
      _indicators(indicators)
{
}

void AccessControl::uidToHex(const uint8_t* uid, uint8_t uidLen, char* hexOut)
{
    for (int i = 0; i < uidLen && i < 7; i++) {
        snprintf(hexOut + i * 2, 3, "%02X", uid[i]);
    }
    hexOut[uidLen * 2] = 0;
}

static void showBootToggleHint(LCDI2C& lcd, bool regMode)
{
    lcd.putChar(regMode ? LCDI2C::CUSTOM_ARROW : LCDI2C::CUSTOM_USER, 3, 18);
    lcd.print(regMode ? "V" : "R", 3, 19);
}

bool AccessControl::startAccessFlow()
{
    ESP_LOGI(TAG, "=== FLUJO DE ACCESO ===");

    _lcd.clean(0, 0);
    _lcd.clean(1, 0);
    _lcd.clean(2, 0);
    _lcd.clean(3, 0);
    _lcd.putChar(LCDI2C::CUSTOM_CARD, 1, 0);
    _lcd.typewrite(" Acerca tu tarjeta", 1, 1, 20);
    showBootToggleHint(_lcd, false);

    uint8_t uid[7];
    uint8_t uidLen = 0;
    bool gotCard = false;

    uint32_t startTime = esp_timer_get_time() / 1000;
    uint8_t dotCount = 0;
    while ((esp_timer_get_time() / 1000 - startTime) < 10000) {
        if (bootToggleDetected()) {
            ESP_LOGI(TAG, "BOOT -> toggle modo");
            return false;
        }
        if (_rfid.isNewTagScanned()) {
            _rfid.getLastSerial(uid, uidLen);
            gotCard = true;
            break;
        }
        // pulsing dots on row 3
        _lcd.print("                    ", 3, 0);
        for (int i = 0; i < dotCount; i++) _lcd.putChar('.', 3, i);
        dotCount = (dotCount + 1) % 6;
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    if (!gotCard) {
        ESP_LOGW(TAG, "RFID timeout");
        return false;
    }

    char uidHex[15] = {};
    uidToHex(uid, uidLen, uidHex);
    ESP_LOGI(TAG, "RFID: %s", uidHex);

    // Fade out old, show card detected
    _lcd.clean(1, 0);
    _lcd.putChar(LCDI2C::CUSTOM_CARD, 1, 0);
    _lcd.typewrite(" ", 1, 1, 0);
    _lcd.print("Tarjeta detectada", 1, 1);
    _lcd.clean(3, 0);
    _lcd.print(uidHex, 3, 2);

    uint16_t expectedFingerID = 0;
    std::string name;
    bool active = true;

    _lcd.clean(0, 0);
    _lcd.typewrite("Verificando...", 0, 0, 20);

    if (!_db.checkCard(uidHex, expectedFingerID, name, active)) {
        _indicators.denied();
        _lcd.clean(0, 0);
        _lcd.clean(1, 0);
        _lcd.clean(2, 0);
        _lcd.clean(3, 0);
        _lcd.putChar(LCDI2C::CUSTOM_CROSS, 0, 8);
        _lcd.printCentered("NO AUTORIZADA", 1);
        _lcd.printCentered(uidHex, 2);
        _db.logAttempt(uidHex, false, "Card not authorized", 0);
        ESP_LOGW(TAG, "Tarjeta %s NO autorizada", uidHex);
        vTaskDelay(pdMS_TO_TICKS(2000));
        _indicators.off();
        return false;
    }

    if (!active) {
        _indicators.denied();
        _lcd.clean(0, 0);
        _lcd.clean(1, 0);
        _lcd.clean(2, 0);
        _lcd.clean(3, 0);
        _lcd.putChar(LCDI2C::CUSTOM_LOCK, 0, 8);
        _lcd.printCentered("DESACTIVADA", 1);
        _db.logAttempt(uidHex, false, "Card deactivated", 0);
        ESP_LOGW(TAG, "Tarjeta %s desactivada", uidHex);
        vTaskDelay(pdMS_TO_TICKS(2000));
        _indicators.off();
        return false;
    }

    _lcd.clean(0, 0);
    _lcd.clean(1, 0);
    _lcd.clean(2, 0);
    _lcd.clean(3, 0);
    _lcd.putChar(LCDI2C::CUSTOM_CHECK, 0, 0);
    _lcd.print("Tarjeta OK!", 0, 1);

    if (!name.empty()) {
        _lcd.putChar(LCDI2C::CUSTOM_USER, 1, 0);
        char nameBuf[20];
        snprintf(nameBuf, sizeof(nameBuf), " Hola %.12s!", name.c_str());
        _lcd.typewrite(nameBuf, 1, 1, 25);
    }

    _lcd.putChar(LCDI2C::CUSTOM_FINGER, 2, 0);
    _lcd.typewrite(" Pon tu dedo", 2, 1, 20);
    showBootToggleHint(_lcd, false);
    ESP_LOGI(TAG, "Tarjeta %s autorizada (%s), esperando fingerID=%u", uidHex, name.c_str(), expectedFingerID);

    uint16_t matchedID = 0;
    uint16_t score = 0;
    if (!_fingerprint.searchFinger(matchedID, score)) {
        _indicators.denied();
        _lcd.clean(2, 0);
        _lcd.putChar(LCDI2C::CUSTOM_CROSS, 2, 0);
        _lcd.print(" HUELLA NO VALIDA", 2, 1);
        _db.logAttempt(uidHex, false, "Fingerprint no match", 0);
        ESP_LOGW(TAG, "Huella no coincide");
        vTaskDelay(pdMS_TO_TICKS(2000));
        _indicators.off();
        return false;
    }

    if (matchedID != expectedFingerID) {
        _indicators.denied();
        _lcd.clean(2, 0);
        _lcd.putChar(LCDI2C::CUSTOM_CROSS, 2, 0);
        _lcd.print(" HUELLA NO VALIDA", 2, 1);
        _db.logAttempt(uidHex, false, "Fingerprint ID mismatch", matchedID);
        ESP_LOGW(TAG, "FingerID esperado=%u obtenido=%u", expectedFingerID, matchedID);
        vTaskDelay(pdMS_TO_TICKS(2000));
        _indicators.off();
        return false;
    }

    _indicators.granted();
    _lcd.clean(0, 0);
    _lcd.clean(1, 0);
    _lcd.clean(2, 0);
    _lcd.clean(3, 0);
    _lcd.putChar(LCDI2C::CUSTOM_UNLOCK, 0, 8);
    _lcd.printCentered("ACCESO CONCEDIDO", 1);

    if (!name.empty()) {
        _lcd.putChar(LCDI2C::CUSTOM_USER, 2, 0);
        char nameBuf[20];
        snprintf(nameBuf, sizeof(nameBuf), " %s", name.c_str());
        _lcd.typewrite(nameBuf, 2, 1, 30);
    }

    _lcd.typewrite("Abriendo puerta", 3, 0, 40);
    for (int i = 0; i < 3; i++) {
        vTaskDelay(pdMS_TO_TICKS(300));
        _lcd.putChar('.', 3, 15 + i);
    }
    ESP_LOGI(TAG, "ACCESO CONCEDIDO: %s fingerID=%u score=%u", uidHex, matchedID, score);

    _servo.setAngleSmooth(30, 20);
    vTaskDelay(pdMS_TO_TICKS(1000));
    _servo.setAngleSmooth(90, 20);

    _db.logAttempt(uidHex, true, "Access granted", matchedID);

    _lcd.clean(3, 0);
    _lcd.putChar(LCDI2C::CUSTOM_CHECK, 3, 2);
    _lcd.print("Puerta abierta!", 3, 3);
    vTaskDelay(pdMS_TO_TICKS(2000));
    _indicators.off();
    return true;
}

bool AccessControl::registrationFlow()
{
    ESP_LOGI(TAG, "=== MODO REGISTRO ===");

    _lcd.clean(0, 0);
    _lcd.clean(1, 0);
    _lcd.clean(2, 0);
    _lcd.clean(3, 0);
    _lcd.putChar(LCDI2C::CUSTOM_CARD, 0, 0);
    _lcd.typewrite(" MODO REGISTRO", 0, 1, 25);
    _lcd.putChar(LCDI2C::CUSTOM_ARROW, 1, 0);
    _lcd.typewrite(" Acerca tarjeta", 1, 1, 20);
    showBootToggleHint(_lcd, true);
    _lcd.typewrite("BOOT->", 3, 14, 0);
    _lcd.putChar(LCDI2C::CUSTOM_ARROW, 3, 13);

    uint8_t uid[7];
    uint8_t uidLen = 0;
    bool gotCard = false;

    uint32_t startTime = esp_timer_get_time() / 1000;
    while ((esp_timer_get_time() / 1000 - startTime) < 15000) {
        if (bootToggleDetected()) {
            ESP_LOGI(TAG, "BOOT -> salir de registro");
            return false;
        }
        if (_rfid.isNewTagScanned()) {
            _rfid.getLastSerial(uid, uidLen);
            gotCard = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (!gotCard) {
        ESP_LOGW(TAG, "Registro: timeout RFID");
        return false;
    }

    char uidHex[15] = {};
    uidToHex(uid, uidLen, uidHex);
    _lcd.clean(1, 0);
    _lcd.putChar(LCDI2C::CUSTOM_CHECK, 1, 0);
    _lcd.print("UID:", 1, 1);
    _lcd.print(uidHex, 1, 5);
    ESP_LOGI(TAG, "Registro UID: %s", uidHex);

    _lcd.clean(2, 0);
    _lcd.clean(3, 0);
    _lcd.putChar(LCDI2C::CUSTOM_FINGER, 2, 0);
    _lcd.typewrite(" Pon dedo 2 veces", 2, 1, 25);

    uint16_t fingerID = 0;
    if (!_fingerprint.enrollFinger(fingerID)) {
        _lcd.clean(2, 0);
        _lcd.putChar(LCDI2C::CUSTOM_CROSS, 2, 0);
        _lcd.print(" REGISTRO FALLIDO", 2, 1);
        ESP_LOGE(TAG, "Registro huella fallo");
        vTaskDelay(pdMS_TO_TICKS(3000));
        return false;
    }

    _lcd.clean(2, 0);
    _lcd.putChar(LCDI2C::CUSTOM_FINGER, 2, 0);
    _lcd.typewrite(" Huella ID ", 2, 1, 30);
    char idBuf[8];
    snprintf(idBuf, sizeof(idBuf), "%u", fingerID);
    _lcd.print(idBuf, 2, 13);

    _lcd.clean(3, 0);
    _lcd.typewrite("Subiendo a nube...", 3, 0, 20);

    if (!_db.registerCard(uidHex, fingerID)) {
        _lcd.clean(3, 0);
        _lcd.putChar(LCDI2C::CUSTOM_CROSS, 3, 0);
        _lcd.print(" ERROR AL SUBIR", 3, 1);
        ESP_LOGE(TAG, "No se pudo registrar en Supabase");
        vTaskDelay(pdMS_TO_TICKS(3000));
        return false;
    }

    _indicators.registered();
    _lcd.clean(0, 0);
    _lcd.clean(1, 0);
    _lcd.clean(2, 0);
    _lcd.clean(3, 0);
    for (int i = 0; i < 4; i++) {
        _lcd.putChar(LCDI2C::CUSTOM_CHECK, i, 9);
        vTaskDelay(pdMS_TO_TICKS(80));
    }
    _lcd.printCentered("REGISTRADO!", 0);
    char regBuf[32];
    snprintf(regBuf, sizeof(regBuf), "UID=%s", uidHex);
    _lcd.printCentered(regBuf, 1);
    snprintf(regBuf, sizeof(regBuf), "Huella ID=%u", fingerID);
    _lcd.printCentered(regBuf, 2);
    _lcd.printCentered("Name en Supabase", 3);
    ESP_LOGI(TAG, "Registrado: %s fingerID=%u", uidHex, fingerID);

    vTaskDelay(pdMS_TO_TICKS(3000));
    return true;
}
