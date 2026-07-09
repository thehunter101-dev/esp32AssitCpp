#include "AccessControl.hpp"
#include "esp_timer.h"
#include "driver/gpio.h"
#include <cstdio>
#include <cstring>
#include <time.h>

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
    _fingerprint.setCaptureCallback([this](int step) {
        _indicators.cardScanned();
        if (step == 0) {
            _lcd.putChar(LCDI2C::CUSTOM_CHECK, 1, 15);
            _lcd.print("OK", 1, 17);
        }
        if (step == 1) {
            _lcd.clean(2, 0);
            _lcd.clean(3, 0);
            _lcd.putChar(LCDI2C::CUSTOM_CHECK, 2, 0);
            _lcd.print(" 1er SCAN OK  ", 2, 1);
            _lcd.putChar(LCDI2C::CUSTOM_ARROW, 3, 0);
            _lcd.print(" QUITA el dedo", 3, 1);
            vTaskDelay(pdMS_TO_TICKS(800));
            _lcd.clean(2, 0);
            _lcd.clean(3, 0);
            _lcd.putChar(LCDI2C::CUSTOM_FINGER, 2, 0);
            _lcd.print(" Pon dedo 2/2  ", 2, 1);
        }
        if (step == 2) {
            _lcd.clean(2, 0);
            _lcd.clean(3, 0);
            _lcd.putChar(LCDI2C::CUSTOM_CHECK, 2, 0);
            _lcd.print(" 2do SCAN OK!", 2, 1);
        }
    });
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

    uint64_t nowMs = esp_timer_get_time() / 1000;

    if (nowMs < _cooldownUntil) {
        return false;
    }

    if (_lockoutUntil > 0 && nowMs >= _lockoutUntil) {
        _lockoutUntil = 0;
        _displayReady = false;
    }

    if (nowMs < _lockoutUntil) {
        if (!_displayReady) {
            _lcd.clean(0, 0);
            _lcd.clean(1, 0);
            _lcd.clean(2, 0);
            _lcd.clean(3, 0);
            _lcd.putChar(LCDI2C::CUSTOM_LOCK, 0, 6);
            _lcd.printCentered("SISTEMA BLOQUEADO", 1);
            _lcd.printCentered("30s lockout...", 2);
            _displayReady = true;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        return false;
    }

    if (!_displayReady) {
        char row1[21] = "                    ";
        row1[0] = LCDI2C::CUSTOM_CARD;
        memcpy(row1 + 1, " Acerca tu tarjeta", 18);
        _lcd.print(row1, 1, 0);

        _lcd.print("                    ", 2, 0);

        _lcd.print("                    ", 3, 0);

        time_t now;
        struct tm ti;
        time(&now);
        localtime_r(&now, &ti);
        char tbuf[16];
        strftime(tbuf, sizeof(tbuf), "%H:%M", &ti);
        char row0[21] = "                    ";
        memcpy(row0 + 7, tbuf, 5);
        _lcd.print(row0, 0, 0);
        _lastClockSec = ti.tm_sec;
        _displayReady = true;
    } else {
        updateClock();
    }

    uint8_t uid[7];
    uint8_t uidLen = 0;
    bool gotCard = false;

    uint32_t startTime = esp_timer_get_time() / 1000;
    uint8_t dotCount = 0;
    while ((esp_timer_get_time() / 1000 - startTime) < 10000) {
        updateClock();
        if (bootToggleDetected()) {
            ESP_LOGI(TAG, "BOOT -> toggle modo");
            return false;
        }
        if (_rfid.isNewTagScanned()) {
            _rfid.getLastSerial(uid, uidLen);
            gotCard = true;
            _displayReady = false;
            break;
        }
        for (int i = 0; i < 6; i++) _lcd.putChar(i < dotCount ? '.' : ' ', 3, i);
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
    _indicators.cardScanned();

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
    _lcd.clean(3, 0);
    for (int i = 0; i < 6; i++) {
        _lcd.putChar('.', 3, i);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
    showBootToggleHint(_lcd, false);
    ESP_LOGI(TAG, "Tarjeta %s autorizada (%s), esperando fingerID=%u", uidHex, name.c_str(), expectedFingerID);

    uint16_t matchedID = 0;
    uint16_t score = 0;
    int fpAttempt = 0;

    while (fpAttempt < 3) {
        if (_fingerprint.searchFinger(matchedID, score) && matchedID == expectedFingerID) {
            break;
        }

        fpAttempt++;
        _indicators.denied();

        _lcd.clean(3, 0);
        char buf[20];
        snprintf(buf, sizeof(buf), "  Intento %d/3", fpAttempt);
        _lcd.putChar(LCDI2C::CUSTOM_CROSS, 2, 0);
        _lcd.print(" HUELLA NO VALIDA", 2, 1);
        _lcd.print(buf, 3, 0);
        showBootToggleHint(_lcd, false);

        _db.logAttempt(uidHex, false,
            matchedID != 0 ? "Fingerprint ID mismatch" : "Fingerprint no match",
            matchedID);
        ESP_LOGW(TAG, "Fallo huella intento %d/3 (scan=%u esp=%u)", fpAttempt, matchedID, expectedFingerID);

        if (fpAttempt >= 3) {
            _db.deactivateCard(uidHex);
            _lockoutUntil = esp_timer_get_time() / 1000 + 30000;
            _db.logAttempt(uidHex, false, "Card blocked - 3 failed attempts", 0);
            ESP_LOGW(TAG, "LOCKOUT 30s + tarjeta %s desactivada", uidHex);
            _lcd.clean(0, 0);
            _lcd.clean(1, 0);
            _lcd.clean(2, 0);
            _lcd.clean(3, 0);
            _lcd.putChar(LCDI2C::CUSTOM_LOCK, 0, 0);
            _lcd.print(" TARJETA BLOQUEADA", 0, 1);
            _lcd.printCentered("30s lockout...", 1);
            _lcd.printCentered(uidHex, 2);
            vTaskDelay(pdMS_TO_TICKS(3000));
            _indicators.off();
            _displayReady = true;
            return false;
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
        _indicators.off();

        _lcd.clean(2, 0);
        _lcd.clean(3, 0);
        _lcd.putChar(LCDI2C::CUSTOM_FINGER, 2, 0);
        _lcd.print(" Pon tu dedo", 2, 1);
        for (int i = 0; i < 6; i++) {
            _lcd.putChar('.', 3, i);
            vTaskDelay(pdMS_TO_TICKS(250));
        }
        showBootToggleHint(_lcd, false);
    }

    _failedAttempts = 0;
    unlockSequence(uidHex, matchedID, name.c_str(), "Access granted");
    _cooldownUntil = esp_timer_get_time() / 1000 + 5000;
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

    uint8_t uid[7];
    uint8_t uidLen = 0;
    bool gotCard = false;

    uint32_t startTime = esp_timer_get_time() / 1000;
    uint8_t dotCount = 0;
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
        for (int i = 0; i < 6; i++) _lcd.putChar(i < dotCount ? '.' : ' ', 3, i);
        dotCount = (dotCount + 1) % 6;
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    if (!gotCard) {
        ESP_LOGW(TAG, "Registro: timeout RFID");
        return false;
    }

    char uidHex[15] = {};
    uidToHex(uid, uidLen, uidHex);
    _indicators.cardScanned();
    _lcd.clean(1, 0);
    _lcd.putChar(LCDI2C::CUSTOM_CHECK, 1, 0);
    _lcd.print("UID:", 1, 1);
    _lcd.print(uidHex, 1, 5);
    ESP_LOGI(TAG, "Registro UID: %s", uidHex);

    _lcd.clean(2, 0);
    _lcd.clean(3, 0);

    uint16_t fingerID = 0;
    bool verifiedMatch = false;
    {
        uint16_t existingFingerID = 0;
        std::string existingName;
        bool existingActive = true;
        if (_db.checkCard(uidHex, existingFingerID, existingName, existingActive)) {
            _lcd.putChar(LCDI2C::CUSTOM_FINGER, 2, 0);
            _lcd.print(" Pon tu dedo", 2, 1);
            _lcd.print("(verificar)", 3, 0);

            uint32_t waitStart = esp_timer_get_time() / 1000;
            uint8_t dotCount = 0;
            bool gotFinger = false;
            uint16_t matchedID = 0;
            uint16_t score = 0;
            bool searchResult = false;
            while ((esp_timer_get_time() / 1000 - waitStart) < 10000) {
                if (_fingerprint.detectFinger()) {
                    gotFinger = true;
                    // searchFinger inmediato — el dedo sigue en el sensor
                    searchResult = _fingerprint.searchFinger(matchedID, score);
                    break;
                }
                for (int i = 0; i < 6; i++) _lcd.putChar(i < dotCount ? '.' : ' ', 3, 11 + i);
                dotCount = (dotCount + 1) % 6;
                vTaskDelay(pdMS_TO_TICKS(300));
            }

            if (!gotFinger) {
                _lcd.clean(0, 0);
                _lcd.clean(1, 0);
                _lcd.clean(2, 0);
                _lcd.clean(3, 0);
                _lcd.printCentered("CANCELADO", 1);
                vTaskDelay(pdMS_TO_TICKS(1500));
                _indicators.off();
                return false;
            }

            for (int i = 0; i < 6; i++) _lcd.putChar(' ', 3, 11 + i);
            _lcd.clean(2, 0);

            if (searchResult) {
                verifiedMatch = true;
                if (matchedID == existingFingerID) {
                    _lcd.clean(0, 0);
                    _lcd.clean(1, 0);
                    _lcd.clean(2, 0);
                    _lcd.clean(3, 0);
                    _lcd.putChar(LCDI2C::CUSTOM_CHECK, 0, 9);
                    _lcd.printCentered("SIN CAMBIOS", 1);
                    _lcd.printCentered("Misma huella", 2);
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    _indicators.off();
                    return false;
                }
                fingerID = matchedID;
                _lcd.putChar(LCDI2C::CUSTOM_ARROW, 2, 0);
                _lcd.print(" HUELLA DISTINTA", 2, 1);
                _lcd.print("Actualizando...", 3, 0);
                vTaskDelay(pdMS_TO_TICKS(1500));
                _lcd.clean(2, 0);
                _lcd.clean(3, 0);
            } else {
                _lcd.putChar(LCDI2C::CUSTOM_FINGER, 2, 0);
                _lcd.print(" NO COINCIDE", 2, 1);
                _lcd.print("Nueva: pon dedo", 3, 0);
                vTaskDelay(pdMS_TO_TICKS(1500));
                _lcd.clean(2, 0);
                _lcd.clean(3, 0);
            }
        }
    }

    if (fingerID == 0 && !verifiedMatch) {
        _lcd.clean(2, 0);
        _lcd.clean(3, 0);
        _lcd.putChar(LCDI2C::CUSTOM_FINGER, 2, 0);
        _lcd.typewrite(" Pon dedo 1/2", 2, 1, 20);
        for (int i = 0; i < 6; i++) {
            _lcd.putChar('.', 3, i);
            vTaskDelay(pdMS_TO_TICKS(250));
        }

        if (!_fingerprint.enrollFinger(fingerID)) {
            _lcd.clean(2, 0);
            _lcd.putChar(LCDI2C::CUSTOM_CROSS, 2, 0);
            _lcd.print(" REGISTRO FALLIDO", 2, 1);
            ESP_LOGE(TAG, "Registro huella fallo");
            vTaskDelay(pdMS_TO_TICKS(3000));
            return false;
        }
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
    _indicators.off();
    return true;
}

void AccessControl::unlockSequence(const char* uidHex, uint16_t fingerID, const char* name, const char* reason)
{
    _indicators.granted();
    _lcd.clean(0, 0);
    _lcd.clean(1, 0);
    _lcd.clean(2, 0);
    _lcd.clean(3, 0);
    _lcd.putChar(LCDI2C::CUSTOM_UNLOCK, 0, 8);
    _lcd.printCentered("ACCESO CONCEDIDO", 1);

    if (name && name[0]) {
        _lcd.putChar(LCDI2C::CUSTOM_USER, 2, 0);
        char nameBuf[20];
        snprintf(nameBuf, sizeof(nameBuf), " %s", name);
        _lcd.typewrite(nameBuf, 2, 1, 30);
    }

    _lcd.typewrite("Abriendo puerta", 3, 0, 40);
    for (int i = 0; i < 3; i++) {
        vTaskDelay(pdMS_TO_TICKS(300));
        _lcd.putChar('.', 3, 15 + i);
    }
    ESP_LOGI(TAG, "Unlock: %s fingerID=%u", uidHex, fingerID);

    _servo.setAngleSmooth(30, 20);
    vTaskDelay(pdMS_TO_TICKS(1000));
    _servo.setAngleSmooth(90, 20);

    _db.logAttempt(uidHex, true, reason, fingerID);

    _lcd.clean(3, 0);
    _lcd.putChar(LCDI2C::CUSTOM_CHECK, 3, 2);
    _lcd.print("Puerta abierta!", 3, 3);
    vTaskDelay(pdMS_TO_TICKS(2000));
    _indicators.off();
}

void AccessControl::resetDisplay()
{
    _displayReady = false;
}

void AccessControl::updateClock()
{
    time_t now;
    struct tm ti;
    time(&now);
    localtime_r(&now, &ti);

    if (ti.tm_sec == _lastClockSec) return;
    _lastClockSec = ti.tm_sec;

    char tbuf[16];
    strftime(tbuf, sizeof(tbuf), "%H:%M", &ti);
    _lcd.print(tbuf, 0, 7);
    _lcd.print("      ", 0, 12);
}

void AccessControl::checkRemoteCommands()
{
    std::string command;
    int64_t cmdId = 0;

    if (!_db.checkPendingCommands(command, cmdId)) {
        return;
    }

    ESP_LOGI(TAG, "Comando remoto: %s (id=%lld)", command.c_str(), (long long)cmdId);

    while (!command.empty() && (command.back() == ' ' || command.back() == '\n' || command.back() == '\r' || command.back() == '\t')) {
        command.pop_back();
    }

    if (command == "unlock") {
        _displayReady = false;
        _lcd.clean(0, 0);
        _lcd.clean(1, 0);
        _lcd.clean(2, 0);
        _lcd.clean(3, 0);
        _lcd.printCentered("DESBLOQUEO REMOTO", 1);
        _lcd.printCentered("Abriendo puerta...", 2);
        vTaskDelay(pdMS_TO_TICKS(1000));

        unlockSequence("REMOTE", 0, "", "Remote unlock");
        _cooldownUntil = esp_timer_get_time() / 1000 + 5000;
        _db.markCommandExecuted(cmdId);
        ESP_LOGI(TAG, "Comando remoto %lld ejecutado", (long long)cmdId);
    }
}
