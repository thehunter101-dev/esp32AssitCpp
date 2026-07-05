# 🚀 GUÍA DE INICIO RÁPIDO

## 1. Compilar el Firmware

```bash
cd c:\Users\pasto\Downloads\esp32AssitCpp
idf.py build
```

**Resultado esperado:**
```
✅ esp32Test.bin generado exitosamente
Tamaño: ~216 KB
```

## 2. Flashear al ESP32

```bash
# Detectar puerto COM
idf.py -p COM_PORT flash

# Ejemplo:
idf.py -p COM3 flash
```

## 3. Configurar credenciales

```bash
cp .env.example .env
```

Editar `.env` con tus datos:

```env
WIFI_SSID=tu_red_wifi
WIFI_PASS=tu_contraseña
SUPABASE_URL=https://tu-proyecto.supabase.co
SUPABASE_KEY=tu_anon_key_supabase
```

> Las credenciales se inyectan en tiempo de compilación desde `.env`.
> El archivo `.env` está en `.gitignore` — no se sube al repositorio.
const char* PASSWORD = "TuContraseña";
const char* SERVER_URL = "http://192.168.1.100:8000/api/access";
```

## 4. Iniciar Servidor Backend

**Requisitos:**
```bash
pip install flask flask-cors
```

**Ejecutar:**
```bash
python backend_server.py
```

**Salida:**
```
🚀 Servidor de Control de Acceso iniciado
📊 API disponible en http://localhost:8000
   - POST   /api/access  - Registrar intentos
   - GET    /api/access  - Obtener historial
   - GET    /api/stats   - Estadísticas
   - GET    /health      - Health check
```

## 5. Probar Integraciones

### Verificar que ESP32 se conecte:
```bash
idf.py -p COM3 monitor
```

Buscar en logs:
```
I (523) MAIN: Sistema listo. Esperando acceso...
I (10000) AccessDB: Sincronización exitosa
```

### Obtener historial desde servidor:
```bash
curl http://localhost:8000/api/access
```

### Obtener estadísticas:
```bash
curl http://localhost:8000/api/stats
```

## 6. Conexiones de Hardware

```
ESP32 WROOM - 30 PINES
┌──────────────────────┐
│  GND ─ GND (power)   │
│  5V  ─ 5V  (power)   │
├──────────────────────┤
│  LCD 2004A (I2C)     │
│  GPIO 21 (SDA) ─ SDA │
│  GPIO 22 (SCL) ─ SCL │
│  GND  ─ GND          │
│  5V   ─ VCC          │
├──────────────────────┤
│  AS608 Fingerprint   │
│  GPIO 12 (TX) ─ RX   │
│  GPIO 14 (RX) ─ TX   │
│  GND  ─ GND          │
│  5V   ─ 5V           │
├──────────────────────┤
│  SG90 Servo          │
│  GPIO 13 (PWM) ─ PWM │
│  GND  ─ GND          │
│  5V   ─ 5V           │
├──────────────────────┤
│  RC522 RFID (SPI)    │
│  GPIO 23 (MOSI) ─ DIN│
│  GPIO 19 (MISO) ─ DOUT│
│  GPIO 18 (CLK)  ─ CLK│
│  GPIO 5  (CS)   ─ SDA│
│  GPIO 4  (RST)  ─ RST│
│  GND  ─ GND          │
│  3.3V ─ VCC          │
└──────────────────────┘
```

## 7. Diagrama de Flujo

```
┌──────────────────────┐
│  Tarjeta RFID        │ Lectura automática
│  (lectura de UID)    │
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│  Validar UID en DB   │ ¿Tarjeta autorizada?
└──────────┬───────────┘
           │
       ✅ │ (Si)
           │
           ▼
┌──────────────────────┐
│  Solicitar Huella    │ Presionar dedo
│  "Acerca tu dedo"    │ en sensor
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│  Capturar Plantilla  │ AS608 genera hash
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│  Comparar Huella     │ ¿Coincide BD?
└──────────┬───────────┘
           │
       ✅ │ (Si coincide)
           │
           ▼
┌──────────────────────┐
│  Girar Servo         │ 0° → 30° → 0°
│  (Abrir Puerta)      │ 1 seg puerta abierta
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│  Registrar Intento   │ NVS: JSON con datos
│  success: true       │
└──────────┬───────────┘
           │
    ┌──────┴──────┐
    │             │
    ▼             ▼
 Cada 60s    Envío HTTP
 si no       POST /api/access
 hay conexión
```

## 8. Estructura JSON de Intentos

**Guardado en ESP32 (NVS):**
```json
{
  "timestamp": 1688132400,
  "success": true,
  "reason": "Access granted",
  "fingerID": 1,
  "uid": "12345678"
}
```

**Enviado al servidor:**
```json
[
  { "timestamp": 1688132400, "success": true, ... },
  { "timestamp": 1688132450, "success": false, ... },
  ...
]
```

## 9. Troubleshooting

### "❌ WiFi connection failed"
- Verificar SSID/Password en main.cpp
- Revisar que el router esté en rango

### "❌ No hay intentos pendientes"
- Verificar que el flujo de acceso se completó (LCD debe mostrar "ACCESO CONCEDIDO")
- Revisar logs: `idf.py monitor`

### "❌ Servidor no recibe datos"
- Verificar URL en main.cpp (192.168.1.100 debe ser la IP del servidor)
- Probar conectividad: `ping 192.168.1.100`
- Ver logs del servidor Flask

### "❌ Servo no se mueve"
- Verificar conexión en GPIO 13
- Revisar voltaje (5V mínimo)
- Calibrar rango en ServoManager.cpp línea ~25

## 10. URLs Útiles

| Endpoint | Método | Propósito |
|----------|--------|-----------|
| `/api/access` | POST | Enviar intentos desde ESP32 |
| `/api/access` | GET | Obtener historial (últimos 100) |
| `/api/stats` | GET | Ver estadísticas (total, exitosos, etc) |
| `/health` | GET | Verificar que servidor está vivo |

---

**¿Problemas?** Revisar `ARQUITECTURA.md` para detalles técnicos.
