## Monitor de Potencia con Pantalla LCD I2C

Este repositorio contiene dos programas para Arduino orientados al trabajo con pantallas LCD I2C (basadas en el chip **PCF8574**), junto a módulos de medición **PZEM004Tv30**. El objetivo es, poder ver parámetros eléctricos de corriente alterna (_trifásico_), en tiempo real.

---

### Estructura del repositorio

```
.
├── leerParámetros/
│   └── leerParámetros.ino # Muestra parámetros eléctricos en LCD I2C
├── buscarPantalla/
│   └── buscarPantalla.ino # Escanea el bus I2C para encontrar la dirección de la pantalla
```

---

### Hardware requerido

- 1 Arduino UNO
- 1 Pantalla LCD I2C 20x4 (PCF8574)
- 3 Módulos PZEM004Tv30

---

### Conexiones:
  - **LCD I2C**
    - SDA → A4
    - SCL → A5
  - **PZEM 1**: RX → D2, TX → D3
  - **PZEM 2**: RX → D4, TX → D5
  - **PZEM 3**: RX → D6, TX → D7

> Nota: Puede alterar los pines de cada PZEM por otro digital, al principio de ```leerParámetros/leerParámetros.ino``` pero, al estar usando Arduino UNO, la conexión con la pantalla I2C **debe ser sobre los pines nombrados**.
---

### Librerías necesarias

_Instalables desde el Administrador de Librerías del IDE de Arduino:_

- `LiquidCrystal_I2C` by Marco Schwartz
- `SoftwareSerial` by Arduino
- `PZEM004Tv30` by Jakub Mandula ([GitHub](https://github.com/mandulaj/PZEM-004T-v30))
- `Wire` by Arduino

---

### buscarPantalla.ino

Este script escanea el bus I2C para detectar la dirección de la pantalla LCD conectada. Útil si no conocés la dirección exacta (por defecto suele ser `0x27`, `0x3F`, o `0x20`).

#### Uso

1. Cargá el sketch en tu Arduino.
2. Abrí el monitor serie a 9600 baudios.
3. Verás la/s dirección I2C detectada (ej. `Dispositivo en 0x20`).

---

### leerParametros.ino

Este sketch muestra en la pantalla LCD los siguientes parámetros eléctricos de cada fase (R, S, T):

- Corriente en cada fase (A).
- Potencia activa total (W).
- Energía total acumulada (kWh).
- Voltaje en cada fase (V).
- Frecuencia en cada fase (Hz).
- Factor de potencia de cada fase.

---

### Sumario: Cómo usar

1. Usá `buscarPantalla.ino` para detectar la dirección I2C de tu pantalla.
2. Modificá `leerParametros.ino` si es necesario para ajustar la dirección:
   ```
   LiquidCrystal_I2C lcd(0x20, 20, 4); // Cambiá 0x20 si tu pantalla tiene otra dirección
   ```
3. Cargá `leerParametros.ino` en tu Arduino.
4. Conecta los sensores y pantallas como se nombró en [Conexiones](#Conexiones).
5. Ahora, tu pantalla va a mostrar los datos que mida cada sensor.
