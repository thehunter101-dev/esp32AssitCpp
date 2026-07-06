# 🔐 ESP32 Access Control — RFID + Fingerprint

Sistema de control de acceso de **2 factores** (RFID + huella dactilar) con ESP32, LCD 2004A I2C, servo SG90 y backend en Supabase.

## Características

- **Doble autenticación**: tarjeta RFID + huella dactilar (AS608)
- **LCD 2004A con iconos personalizados**: feedback visual con caracteres CGRAM (lock, check, cross, card, finger, etc.)
- **Supabase cloud backend**: registro de tarjetas, huellas, y log de intentos
- **Re-registro inteligente**: si una tarjeta ya existe, verifica si la huella coincide (misma → SIN CAMBIOS, distinta → actualiza `finger_id` vía PATCH, nueva → enroll + PATCH)
- **Pitido en captura de huella**: beep cuando el sensor captura la imagen (sabés cuándo retirar el dedo)
- **LED del sensor**: apagado en idle vía comando AuraLEDControl
- **Desbloqueo remoto**: polling cada 5s de `pending_commands` en Supabase, INSERT `command='unlock'` para abrir desde la web
- **Borrado masivo de huellas**: hold BOOT button 3s en modo registro
- **Puerta servo-controlada**: apertura/cierre gradual con `setAngleSmooth()`
- **Efectos LCD**: typewrite, printCentered, wipeRow, animaciones
- **Indicadores LED + buzzer**: verde = acceso, rojo = denegado, patrones de pitido

## Stack

| Componente | Tecnología |
|------------|------------|
| MCU | ESP32 WROOM (30 pines) |
| Framework | ESP-IDF 6.0.2 |
| LCD | 2004A I2C (0x27) — character display |
| RFID | RC522 (SPI) |
| Fingerprint | AS608 (UART) |
| Servo | SG90 (PWM LEDC) |
| Backend | Supabase (PostgreSQL + REST) |
| Build | ninja vía VS Code |

## Mapa de Pines

| GPIO | Dispositivo | Señal | Periférico | Dirección |
|------|-------------|-------|------------|-----------|
| 5  | RC522 RFID | CS | SPI (VSPI) | Salida |
| 12 | AS608 Huella | TX (ESP→FP) | UART1 | Salida |
| 13 | SG90 Servo | PWM | LEDC CH0 | Salida |
| 14 | AS608 Huella | RX (ESP←FP) | UART1 | Entrada |
| 18 | RC522 RFID | SCK | SPI (VSPI) | Salida |
| 19 | RC522 RFID | MISO | SPI (VSPI) | Entrada |
| 21 | LCD 2004A | SDA | I2C0 | Bidir. |
| 22 | LCD 2004A | SCL | I2C0 | Bidir. |
| 23 | RC522 RFID | MOSI | SPI (VSPI) | Salida |
| 25 | LED verde | Ánodo (+) | GPIO | Salida |
| 26 | LED rojo | Ánodo (+) | GPIO | Salida |
| 27 | Buzzer activo | Señal | GPIO | Salida |
| 0  | BOOT button | Entrada (pull-up) | GPIO | Entrada |

### Periféricos

- **I2C0**: 100 kHz, address LCD 0x27, pull-ups internos
- **SPI3_HOST (VSPI)**: 4 MHz, mode 0, sin DMA
- **UART1**: 57600 baud, 8N1, sin flow control
- **LEDC**: 50 Hz, 14-bit resolution, duty 410-2048 (0°-180°)

### Pines libres (ESP32 de 30 pines)

1, 2, 3, 4, 8, 9, 10, 11, 15, 16, 17, 32, 33

> **Notas**: GPIO 1/3 = UART0 (monitor serie), GPIO 6-11 = flash interna WROOM (no usar), GPIO 0/2/12/15 = strapping (usar con precaución)

## Setup

### 1. Credenciales

```bash
cp .env.example .env
```

Editar `.env`:

```env
WIFI_SSID=tu_red_wifi
WIFI_PASS=tu_contraseña
SUPABASE_URL=https://tu-proyecto.supabase.co
SUPABASE_KEY=tu_anon_key_supabase
```

### 2. Build

```bash
idf.py build
# o desde VS Code: Ctrl+Shift+P → ESP-IDF: Build Project
```

### 3. Flash

```bash
idf.py -p COM3 flash monitor
```

### 4. Base de datos Supabase

Ejecutar `supabase_setup.sql` en el SQL Editor de Supabase para crear las tablas:

- `authorized_cards` — UIDs autorizados con `finger_id`, `name`, `active`
- `access_logs` — historial de intentos
- `pending_commands` — comandos remotos (INSERT `command='unlock'` para abrir puerta)

## Uso

- **Pasar tarjeta RFID** → LCD muestra "Tarjeta OK!" y nombre (si tiene)
- **Poner dedo en sensor** → verifica huella contra el ID vinculado a la tarjeta (pitido al detectar)
- **Acceso concedido** → servo abre puerta, LED verde, log a Supabase
- **3 intentos fallidos** → tarjeta desactivada + lockout 30s
- **Modo registro** → tocar BOOT button cambia entre login/registro
- **Registrar tarjeta nueva** → en modo registro, pasar tarjeta + poner dedo 2 veces
- **Actualizar tarjeta existente** → al pasar una tarjeta ya registrada, podés verificar/matar la huella actual o registrar una nueva
- **Desbloqueo remoto** → `INSERT INTO pending_commands (command) VALUES ('unlock');` en Supabase
- **Borrar todas las huellas** → en modo registro, hold BOOT button 3s

## Estructura del Proyecto

```
main/
├── main.cpp                    # Entry point, boot, mode toggle
├── CMakeLists.txt              # Genera secrets.h desde .env
├── modules/
│   ├── AccessControl.cpp/.hpp  # Flujo de acceso y registro
│   ├── AccessDB.cpp/.hpp       # HTTP client para Supabase
│   ├── FingerprintManager.*    # Comandos AS608
│   ├── Indicators.*            # LED verde/rojo + buzzer
│   ├── LCDI2C.*                # LCD 2004A con CGRAM icons
│   ├── RFIDManager.*           # RC522 wrapper
│   ├── ServoManager.*          # PWM servo con movimiento gradual
│   └── WifiDriver.*            # Conexión WiFi
├── supabase_setup.sql          # Schema SQL para Supabase
└── .env.example                # Template de credenciales
```
