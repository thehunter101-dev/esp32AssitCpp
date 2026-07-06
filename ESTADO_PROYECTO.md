# 📋 ESTADO DEL PROYECTO — Control de Acceso ESP32

## ✅ COMPLETADO

### 1. Hardware Drivers
- ✅ **LCD 2004A I2C** — caracteres custom CGRAM (lock, check, cross, card, finger, user, arrow), efectos typewrite/printCentered/wipeRow
- ✅ **AS608 Fingerprint (UART1)** — enroll, search, deleteAll (DEL_ALL 0x0D)
- ✅ **SG90 Servo (PWM LEDC)** — setAngleSmooth() con movimiento gradual 1°
- ✅ **RC522 RFID (SPI)** — vía librería abobija/rc522 2.6.1 (funcional con ESP-IDF 6.0.2)
- ✅ **Indicators (LEDs + buzzer)** — GPIO 25 (verde), 26 (rojo), 27 (buzzer activo)

### 2. Control de Acceso
- ✅ **Doble factor**: RFID → verificar en Supabase → esperar huella → matchear finger_id
- ✅ **Flujo de acceso completo** con pantallas animadas en LCD
- ✅ **Flujo de registro**: pasar tarjeta → poner dedo 2 veces → POST/PATCH a Supabase
- ✅ **Verificación en actualización**: si la tarjeta existe, verifica la huella actual — misma ID = SIN CAMBIOS, distinta ID = actualiza, sin match = enroll nueva
- ✅ **AuraLEDControl**: LED del AS608 apagado en idle vía comando 0x35
- ✅ **Borrado masivo de huellas**: hold BOOT button 3s en modo registro
- ✅ **Puerta servo-controlada**: apertura 30°, cierre a 90°, movimiento gradual
- ✅ **Cooldown 5s**: después de acceso exitoso, no acepta otra tarjeta por 5s
- ✅ **Lockout 30s**: 3 fallos de huella consecutivos bloquean el sistema 30s
- ✅ **Beep en detección de tarjeta**: buzzer hace un pitido corto al leer UID
- ✅ **Beep en captura de huella**: pitido cuando el sensor captura la imagen (sabés cuándo retirar el dedo), tanto en `searchFinger()` como en `enrollFinger()`

### 3. Backend Cloud (Supabase)
- ✅ **Tabla `authorized_cards`**: uid_hex (PK), finger_id, name, active, created_at
- ✅ **Tabla `access_logs`**: id, uid_hex, success, reason, finger_id, created_at
- ✅ **RLS anon**: `anon_all` en authorized_cards, `anon_insert`+`anon_select` en access_logs
- ✅ **HTTP client**: GET (checkCard), POST (registerCard), PATCH (update finger_id si 409), GET/PATCH en `pending_commands`
- ✅ **Logging**: todos los intentos se registran con razón ("Card not authorized", "Access granted", etc.)
- ✅ **created_at en timezone America/Bogota**: defaults a `(NOW() AT TIME ZONE 'America/Bogota')`
- ✅ **Tabla `pending_commands`**: id, command, params, created_at, executed — para desbloqueo remoto
- ✅ **Desbloqueo remoto**: ESP32 polling cada 5s, ejecuta `unlock` y marca como ejecutado

### 4. LCD — Overhaul Visual
- ✅ 8 caracteres CGRAM personalizados (lock, unlock, check, cross, card, finger, arrow, user)
- ✅ Efecto typewrite, printCentered, wipeRow
- ✅ Pantallas rediseñadas con iconos en todas las pantallas de acceso y registro
- ✅ Corrección de alineación (textos truncados por overflow de 20 columnas)
- ✅ **Reloj en vivo**: hora actual en fila 0 mientras espera tarjeta (NTP sync al boot)

### 5. Seguridad de Credenciales
- ✅ `.env` con credenciales (gitignorado)
- ✅ `.env.example` con placeholders (trackeado)
- ✅ `secrets.h` generado automáticamente desde `.env` en tiempo de build
- ✅ Ninguna credencial hardcodeada en el código fuente

### 6. Limpieza de Código Muerto
- ✅ Eliminado `backend_server.py` (Flask reemplazado por Supabase)
- ✅ Eliminado `JsonParser.cpp/.hpp` (JSON construido con `snprintf` manual)
- ✅ Eliminado `RFIDReader.cpp/.hpp` (template no usado, se usa `RFIDManager` con `abobija/rc522`)

### 7. Documentación
- ✅ `readme.md` — documentación principal con pinout y setup
- ✅ `ARQUITECTURA.md` — diseño técnico y diagramas
- ✅ `GUIA_INICIO_RAPIDO.md` — pasos para compilar, flashear y probar
- ✅ `supabase_setup.sql` — schema SQL para crear tablas en Supabase

## 🔧 COMPILACIÓN Y FLASH

```bash
cp .env.example .env   # Completar credenciales
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

## 📊 ARQUITECTURA

```
RFID (RC522) ─→ AccessControl ─→ AccessDB ─→ Supabase REST API
                   │
AS608 ─────────────┤
                   │
Servo SG90 ────────┘
                   │
LCD 2004A ─────────┤ ← Feedback visual con iconos CGRAM
                   │
Indicators ────────┤ ← LED verde/rojo + buzzer
```

## 🎯 PRÓXIMO (Opcional)

- Frontend web para visualizar logs
- Dashboard web con botón de desbloqueo remoto
- Sensor de puerta (reed switch) para detectar estado abierto/cerrado
- Tarea separada para polling de comandos remotos (no bloquear main loop con HTTP síncrono)

---

**Estado:** 🟢 Funcional completo — RFID + Fingerprint + Supabase + LCD
