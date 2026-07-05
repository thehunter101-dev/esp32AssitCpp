# 🔐 Sistema de Control de Acceso ESP32 - Documentación Técnica

## Arquitectura General

El sistema es un **control de acceso con 2 factores** (RFID + Huella Dactilar) que registra todos los intentos en una base de datos para auditoría web.

### Componentes Principales

```
┌─────────────────┐
│  ESP32 WROOM    │
│    (GPIO)       │
├─────────────────┤
│ ┌─────────────────────────────────────┐
│ │  LCDI2C (LCD 2004A)                 │ I2C0 (21=SDA, 22=SCL)
│ ├─────────────────────────────────────┤
│ │  ServoManager (SG90)                │ GPIO 13 (PWM LEDC)
│ ├─────────────────────────────────────┤
│ │  FingerprintManager (AS608)         │ UART1 (12=TX, 14=RX, 57600 baud)
│ ├─────────────────────────────────────┤
│ │  AccessControl                      │ Orquesta flujo (RFID→Huella→Servo)
│ ├─────────────────────────────────────┤
│ │  AccessDB                           │ NVS + HTTP POST al servidor
│ └─────────────────────────────────────┘
│         WiFi (env_provisioning)        │ Para sincronización
└─────────────────┘
```

## Flujo de Operación

### 1. **Lectura RFID** (Pendiente - Ver Sección 3)
   - ESP32 espera tarjeta cercana
   - Lee UID de 4-7 bytes
   - Valida contra lista de tarjetas autorizadas (TODO)

### 2. **Captura de Huella Dactilar** (✅ Implementado)
   - Se solicita huella al usuario
   - Sensor AS608 captura y procesa
   - Genera plantilla de 512 bytes

### 3. **Validación de Huella** (✅ Base implementada)
   - Compara plantilla capturada vs. plantilla registrada
   - Actualmente acepta cualquier huella válida (TODO: Agregar comparación real)

### 4. **Accionamiento de Servo** (✅ Implementado)
   - Si validación exitosa: servo gira a 30° (abre puerta)
   - Espera 1 segundo
   - Vuelve a 90° (cierra puerta)

### 5. **Registro de Intento** (✅ Implementado)
   - Guarda en NVS con timestamp
   - Estructura JSON: `{timestamp, success, reason, fingerID, uid}`
   - Máximo 100 intentos en memoria

### 6. **Sincronización con Servidor** (✅ Implementado)
   - Cada 60 segundos: intenta POST a servidor
   - Envía array JSON de intentos pendientes
   - Si HTTP 200/201: limpia NVS
   - Si falla: reintentos en próximo ciclo

## Clases Implementadas

### `AccessControl`
**Archivo:** `main/modules/AccessControl.hpp/cpp`

Orquesta el flujo de acceso:
```cpp
AccessControl(LCDI2C& lcd, ServoManager& servo, FingerprintManager& fingerprint);

bool startAccessFlow();                    // Inicia secuencia RFID→Huella→Servo
void processRFIDCard(uint8_t* uid, ...);  // Procesa tarjeta detectada
bool validateFingerprint(...);            // Valida huella
void unlockDoor();                        // Acciona servo
void recordAttempt(...);                  // Registra en NVS
void setAccessCallback(...);              // Notificación de intentos
```

### `AccessDB`
**Archivo:** `main/modules/AccessDB.hpp/cpp`

Gestiona almacenamiento y sincronización:
```cpp
AccessDB(const char* serverUrl);          // Inicializa con URL de servidor
esp_err_t init();                         // Abre NVS namespace
esp_err_t saveAttempt(...);               // Guarda en NVS
esp_err_t syncToServer();                 // POST a servidor
uint32_t getPendingCount();               // Intentos pendientes
esp_err_t clearAttempts();                // Limpia después de sync exitoso
```

## Configuración

### URL del Servidor
**Archivo:** `main/main.cpp` línea ~60
```cpp
AccessDB accessDB("http://192.168.1.100:8000/api/access");  // CAMBIAR IP
```

### Pines RFID (TODO)
Cuando se implemente RFID:
```cpp
GPIO_NUM_23  // MOSI
GPIO_NUM_19  // MISO
GPIO_NUM_18  // CLK
GPIO_NUM_5   // CS
GPIO_NUM_4   // RST
```

## Problemas Pendientes & Soluciones

### ❌ **RFID No Funcional (RC522 v2.6.1)**

**Problema:** Librería RC522 v2.6.1 no compatible con ESP-IDF 6.0.2
- API esperada: `rc522_spi_create()`, `rc522_driver_install()`
- Realidad: Funciones no existen en v2.6.1

**Soluciones:**

**Opción A: Implementación Manual SPI (Recomendado)**
```cpp
// main/modules/RFIDReader.hpp (CREAR)
class RFIDReader {
public:
    void init(gpio_num_t mosi, miso, sclk, cs, rst);
    bool readUID(uint8_t* uid, uint8_t& uidLen);  // Retorna UID
private:
    void spiWrite(uint8_t byte);
    uint8_t spiRead();
    void chipSelect();
    void chipDeselect();
};
```

**Opción B: Usar librería MFRC522 alternativa**
```bash
# En idf_component.yml agregar:
dependencies:
  owntech/mfrc522: "^1.0.0"  # Buscar alternativa compatible
```

**Opción C: Leyenda de tarjetas preregistradas (Workaround)**
```cpp
// Simular lectura RFID para testing
const uint8_t VALID_CARDS[][4] = {
    {0x12, 0x34, 0x56, 0x78},
    {0xAA, 0xBB, 0xCC, 0xDD},
};

// En AccessControl::startAccessFlow():
// accessControl.processRFIDCard(VALID_CARDS[0], 4);  // Testing
```

## API del Servidor (Esperado)

**POST** `/api/access`

**Request:**
```json
[
  {
    "timestamp": 1688132400,
    "success": true,
    "reason": "Access granted",
    "fingerID": 1,
    "uid": "123456"
  },
  {
    "timestamp": 1688132450,
    "success": false,
    "reason": "Fingerprint mismatch",
    "fingerID": 0,
    "uid": "AABBCCDD"
  }
]
```

**Response Exitosa:**
```json
{ "status": "ok", "processed": 2 }
```

## Compilación & Flash

```bash
# Compilar (ya hecho)
idf.py build

# Flash a ESP32
idf.py -p COM3 flash

# Monitorear logs
idf.py -p COM3 monitor
```

## Logs Esperados (Console)

```
I (523) MAIN: Sistema listo. Esperando acceso...
I (5000) AccessControl: === FLUJO DE ACCESO INICIADO ===
I (5100) AccessControl: RFID detectado: 12345678
I (6000) AccessControl: === CAPTURANDO HUELLA ===
I (6200) FingerprintManager: Huella capturada
I (6300) AccessControl: Accionando servo para abrir puerta...
I (7500) AccessControl: Acceso concedido (HTTP 200)
I (7600) AccessDB: Sincronización exitosa - 1 intento enviado
```

## Estado Actual del Proyecto

✅ **Compilado exitosamente** para ESP-IDF 6.0.2
✅ **LCD 2004A** (I2C) - Funcional
✅ **AS608 Fingerprint** (UART) - Funcional (captura en demo)
✅ **SG90 Servo** (PWM) - Funcional
✅ **NVS Storage** - Funcional
✅ **HTTP Client** - Funcional (envío de datos)
❌ **RFID RC522** - Requiere integración manual

## Próximos Pasos

1. **Implementar RFIDReader.hpp/cpp** con protocolo SPI directo
2. **Integrar en AccessControl::startAccessFlow()** para detectar tarjeta
3. **Crear BD de tarjetas autorizadas** en NVS
4. **Implementar comparación real de huellas** (Img2Tz + Match en AS608)
5. **Backend Flask/Django** para registrar intentos
6. **Frontend web** para visualizar logs de acceso

---

**Fecha Actualización:** 2026-07-04
**ESP-IDF:** v6.0.2
**Compilador:** xtensa-esp32-elf v15.2.0_20251204
