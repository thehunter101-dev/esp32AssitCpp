
# Table of Contents

1.  [Mapa de Pines ESP32 (30 pines)](#org0c2ad55)
2.  [Periféricos](#org4025dfe)
3.  [Pines libres (16 disponibles en ESP32 de 30 pines)](#org84c7395)
    1.  [Notas](#org90f4690)



<a id="org0c2ad55"></a>

# Mapa de Pines ESP32 (30 pines)

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-right" />

<col  class="org-left" />

<col  class="org-left" />

<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-right">GPIO</th>
<th scope="col" class="org-left">Dispositivo</th>
<th scope="col" class="org-left">Señal</th>
<th scope="col" class="org-left">Periférico</th>
<th scope="col" class="org-left">Dirección</th>
</tr>
</thead>
<tbody>
<tr>
<td class="org-right">4</td>
<td class="org-left">RC522 RFID</td>
<td class="org-left">RST</td>
<td class="org-left">GPIO</td>
<td class="org-left">Salida</td>
</tr>

<tr>
<td class="org-right">5</td>
<td class="org-left">RC522 RFID</td>
<td class="org-left">CS</td>
<td class="org-left">SPI (VSPI)</td>
<td class="org-left">Salida</td>
</tr>

<tr>
<td class="org-right">12</td>
<td class="org-left">AS608 Huella</td>
<td class="org-left">TX (ESP→FP)</td>
<td class="org-left">UART1</td>
<td class="org-left">Salida</td>
</tr>

<tr>
<td class="org-right">13</td>
<td class="org-left">SG90 Servo</td>
<td class="org-left">PWM</td>
<td class="org-left">LEDC CH0</td>
<td class="org-left">Salida</td>
</tr>

<tr>
<td class="org-right">14</td>
<td class="org-left">AS608 Huella</td>
<td class="org-left">RX (ESP←FP)</td>
<td class="org-left">UART1</td>
<td class="org-left">Entrada</td>
</tr>

<tr>
<td class="org-right">18</td>
<td class="org-left">RC522 RFID</td>
<td class="org-left">SCK</td>
<td class="org-left">SPI (VSPI)</td>
<td class="org-left">Salida</td>
</tr>

<tr>
<td class="org-right">19</td>
<td class="org-left">RC522 RFID</td>
<td class="org-left">MISO</td>
<td class="org-left">SPI (VSPI)</td>
<td class="org-left">Entrada</td>
</tr>

<tr>
<td class="org-right">21</td>
<td class="org-left">LCD 2004A</td>
<td class="org-left">SDA</td>
<td class="org-left">I2C0</td>
<td class="org-left">Bidir.</td>
</tr>

<tr>
<td class="org-right">22</td>
<td class="org-left">LCD 2004A</td>
<td class="org-left">SCL</td>
<td class="org-left">I2C0</td>
<td class="org-left">Bidir.</td>
</tr>

<tr>
<td class="org-right">23</td>
<td class="org-left">RC522 RFID</td>
<td class="org-left">MOSI</td>
<td class="org-left">SPI (VSPI)</td>
<td class="org-left">Salida</td>
</tr>
</tbody>
</table>


<a id="org4025dfe"></a>

# Periféricos

-   I2C0: 100 kHz, address LCD 0x27, pull-ups internos habilitados
-   SPI3<sub>HOST</sub> (VSPI): 4 MHz, mode 0, DMA deshabilitado
-   UART1: 57600 baud, 8N1, sin flow control, pull-ups internos
-   LEDC: 50 Hz, 14-bit resolution, duty 410-2048 (0°-180°)


<a id="org84c7395"></a>

# Pines libres (16 disponibles en ESP32 de 30 pines)

0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 15, 16, 17, 25, 26, 27, 32, 33


<a id="org90f4690"></a>

## Notas

-   GPIO 1 (TX0) y GPIO 3 (RX0) = UART0 / monitor serie
-   GPIO 6-11 = flash interna en módulos WROOM (no usar)
-   GPIO 0, 2, 12, 15 = pines de strapping (usar con precaución)

