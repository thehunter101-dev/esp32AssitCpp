#ifndef ACCESSCONTROL_HPP_
#define ACCESSCONTROL_HPP_

#include <cstdint>
#include <string>
#include <vector>
#include "esp_log.h"
#include "LCDI2C.hpp"
#include "ServoManager.hpp"
#include "FingerprintManager.hpp"
#include "RFIDManager.hpp"
#include "AccessDB.hpp"
#include "Indicators.hpp"

class AccessControl {
public:
    AccessControl(LCDI2C& lcd, ServoManager& servo,
                  FingerprintManager& fingerprint, RFIDManager& rfid,
                  AccessDB& db, Indicators& indicators);

    bool startAccessFlow();
    bool registrationFlow();
    void checkRemoteCommands();
    void updateClock();
    void resetDisplay();
    void setWifiConnected(bool connected);

private:
    void uidToHex(const uint8_t* uid, uint8_t uidLen, char* hexOut);
    void unlockSequence(const char* uidHex, uint16_t fingerID, const char* name, const char* reason);

    LCDI2C& _lcd;
    ServoManager& _servo;
    FingerprintManager& _fingerprint;
    RFIDManager& _rfid;
    AccessDB& _db;
    Indicators& _indicators;

    uint64_t _cooldownUntil = 0;
    int _failedAttempts = 0;
    uint64_t _lockoutUntil = 0;
    int _lastClockSec = -1;
    bool _displayReady = false;
    bool _wifiConnected = false;
    bool _showWifiIcon = false;

    static constexpr const char* TAG = "ACCESS_CTRL";
};

bool isBootToggled();

#endif
