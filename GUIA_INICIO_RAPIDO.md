# GUÍA DE INICIO RÁPIDO

## 1. Prerrequisitos

- ESP-IDF 6.0.2
- Python 3.11+ (para idf.py)

## 2. Configurar credenciales

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

> Las credenciales se inyectan en tiempo de compilación desde `.env`.
> `.env` está en `.gitignore` — no se sube al repositorio.

## 3. Configurar Supabase

1. Crear proyecto en [supabase.com](https://supabase.com)
2. Ir a **SQL Editor** y pegar `supabase_setup.sql`
3. Anotar `Project URL` y `anon public key` desde **Settings > API**

## 4. Compilar y flashear

```bash
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

## 5. Uso

### Modo Login (default)
1. Acercar tarjeta RFID al RC522
2. LCD muestra "Tarjeta OK!" y pide huella
3. Poner dedo en AS608
4. Si todo coincide: servo abre 3s, LCD muestra "ACCESO CONCEDIDO"

### Modo Registro
1. Presionar BOOT para cambiar a modo registro
2. Acercar tarjeta nueva → poner dedo 2 veces (enrollment)
3. Si la tarjeta ya existe → verifica la huella actual (misma = SIN CAMBIOS, distinta = actualiza, nueva = enroll)
4. Tarjeta + huella quedan registrados/actualizados en Supabase

### Desbloqueo Remoto
1. Desde SQL Editor de Supabase o REST API:
   ```sql
   INSERT INTO pending_commands (command) VALUES ('unlock');
   ```
2. ESP32 lo detecta en ~5s y abre la puerta

### Borrado masivo de huellas
1. Estando en modo registro, mantener BOOT 3 segundos

## 6. Conexiones de Hardware

```
ESP32 WROOM - 30 PINES

  LCD 2004A (I2C)
  GPIO 21 (SDA) ─ SDA
  GPIO 22 (SCL) ─ SCL
  GND ─ GND
  5V ─ VCC

  AS608 Fingerprint (UART)
  GPIO 12 (TX) ─ RX
  GPIO 14 (RX) ─ TX
  GND ─ GND
  5V ─ 5V

  SG90 Servo (PWM)
  GPIO 13 ─ PWM (naranja)
  GND ─ marrón
  5V ─ rojo

  RC522 RFID (SPI)
  GPIO 23 ─ MOSI
  GPIO 19 ─ MISO
  GPIO 18 ─ SCK
  GPIO 5  ─ SDA (CS)
  3.3V ─ VCC
  GND ─ GND

  Indicadores
  GPIO 25 ─ LED verde (ánodo)
  GPIO 26 ─ LED rojo (ánodo)
  GPIO 27 ─ Buzzer activo (+)
  GND ─ cátodos / buzzer (-)
```

> El buzzer activo en GPIO 27 requiere transistor (no drive directo del GPIO).

## 7. Troubleshooting

| Síntoma | Causa posible |
|---------|---------------|
| "RFID init fail" | Cableado SPI incorrecto o voltaje |
| "FP init fail" | UART invertido (TX/RX cruzados) |
| "WiFi connection failed" | Credenciales en `.env` incorrectas |
| Servo no se mueve | PWM mal calibrado en `ServoManager.cpp` |
| "Error al subir" | Supabase URL/key incorrecta o RLS mal configurada |
