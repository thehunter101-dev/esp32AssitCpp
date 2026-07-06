# Control de Acceso ESP32 — Arquitectura

## Visión General

Sistema de **doble factor** (RFID + huella dactilar) con backend cloud **Supabase**.
El ESP32 orquesta la lectura de tarjeta, validación en la nube, captura de huella,
apertura de puerta con servo, y registro de cada intento.

## Diagrama de Módulos

```
RFID (RC522 SPI) ──┐
                   ├──→ AccessControl ──→ AccessDB ──→ Supabase REST API
AS608 (UART) ──────┘        │
                            │
  Servo SG90 (PWM) ─────────┤
                            │
  LCD 2004A (I2C) ─────────┤ ← feedback visual con iconos CGRAM
                            │
  Indicators (GPIO) ────────┤ ← LED verde/rojo + buzzer
```

## Módulos

### AccessControl (`main/modules/AccessControl.hpp/cpp`)
Orquesta el flujo completo:
1. Espera tarjeta RFID (10s timeout)
2. Valida contra Supabase vía `AccessDB::checkCard()`
3. Pide huella y la matchea contra `finger_id` de la BD
4. Abre puerta (servo 30°) si todo OK
5. Registra intento en Supabase
6. `registrationFlow()`: registro de nueva tarjeta + huella, o actualización de tarjeta existente con verificación de huella
7. `checkRemoteCommands()`: polling cada 5s de `pending_commands`, ejecuta `unlock` remoto

### AccessDB (`main/modules/AccessDB.hpp/cpp`)
Cliente HTTP para Supabase REST API:
- `checkCard()` → GET `/rest/v1/authorized_cards?uid_hex=eq.XYZ`
- `registerCard()` → POST `/rest/v1/authorized_cards` (si 409 → PATCH)
- `logAttempt()` → POST `/rest/v1/access_logs`
- `deactivateCard()` → PATCH para desactivar tarjeta tras 3 fallos
- `checkPendingCommands()` → GET `/rest/v1/pending_commands?executed=eq.false`
- `markCommandExecuted()` → PATCH para marcar comando como ejecutado
- Parseo manual de JSON (sin cJSON, usa `snprintf` + string search), soporte de escapes `\n`, `\r`, `\t`, `\"`, `\\`

### LCDI2C (`main/modules/LCDI2C.hpp/cpp`)
Driver I2C para LCD 2004A (20×4):
- 8 caracteres CGRAM personalizados (lock, unlock, check, cross, card, finger, arrow, user)
- Efectos: typewrite, printCentered, wipeRow
- Animaciones con iconos

### RFIDManager (`main/modules/RFIDManager.hpp/cpp`)
Wrapper de librería `abobija/rc522` v2.6.1 por SPI:
- `isNewTagScanned()` / `getLastSerial()`
- Inicialización en SPI3_HOST (GPIO 23/19/18/5)

### FingerprintManager (`main/modules/FingerprintManager.hpp/cpp`)
Sensor AS608 por UART1 (57600 baud):
- `searchFinger()`: busca huella en librería del sensor (con capture callback al encontrar match)
- `enrollFinger()`: registro de 2 capturas (capture callback después de cada scan)
- `deleteAllFingers()`: borrado masivo vía comando DEL_ALL (0x0D)
- `setLED(bool)`: control de LED vía AuraLEDControl (0x35)
- `setCaptureCallback()`: callback disparado en captura exitosa para feedback (beep)
- Timeouts configurables

### ServoManager (`main/modules/ServoManager.hpp/cpp`)
SG90 por PWM (LEDC, GPIO 13):
- `setAngleSmooth(angle, delayMs)`: movimiento gradual 1°
- Rango: 0°-90°

### Indicators (`main/modules/Indicators.hpp/cpp`)
Feedback físico:
- LED verde (GPIO 25) + LED rojo (GPIO 26) + buzzer activo (GPIO 27)
- Patrones: granted, denied, registered, off
- Buzzer con transistor (no drive directo)

### WifiDriver (`main/modules/WifiDriver.hpp/cpp`)
Conexión WiFi con manejo de eventos.

## Seguridad de Credenciales

- `.env` con credenciales reales (gitignorado)
- `.env.example` con placeholders (trackeado)
- `secrets.h` generado automáticamente desde `.env` en build (`main/CMakeLists.txt`)
- `#include "secrets.h"` para tener `WIFI_SSID`, `WIFI_PASS`, `SUPABASE_URL`, `SUPABASE_KEY`

## Base de Datos (Supabase)

### authorized_cards
| Columna | Tipo | Default |
|---------|------|---------|
| uid_hex | TEXT PK | — |
| finger_id | INTEGER | — |
| name | TEXT | '' |
| active | BOOLEAN | true |
| created_at | TIMESTAMP | now() AT TIME ZONE 'America/Bogota' |

### access_logs
| Columna | Tipo | Default |
|---------|------|---------|
| id | BIGINT (identity) | — |
| uid_hex | TEXT | — |
| success | BOOLEAN | — |
| reason | TEXT | '' |
| finger_id | INTEGER | 0 |
| created_at | TIMESTAMP | now() AT TIME ZONE 'America/Bogota' |

RLS: anon key puede leer/escribir en ambas tablas.

## Pines (ESP32 WROOM 30-pin)

| Componente | GPIO | Protocolo |
|------------|------|-----------|
| RFID MOSI | 23 | SPI3 |
| RFID MISO | 19 | SPI3 |
| RFID CLK | 18 | SPI3 |
| RFID CS | 5 | SPI3 |
| Servo PWM | 13 | LEDC |
| Fingerprint TX | 12 | UART1 |
| Fingerprint RX | 14 | UART1 |
| LCD SDA | 21 | I2C0 |
| LCD SCL | 22 | I2C0 |
| LED Verde | 25 | GPIO out |
| LED Rojo | 26 | GPIO out |
| Buzzer | 27 | GPIO out |

## Compilación

```bash
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```
