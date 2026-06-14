# Hardware & Wiring

## Bill of materials

| Qty | Part                       | Role                            |
|-----|----------------------------|---------------------------------|
| 2   | ESP32 dev board            | Sensor node + fan controller    |
| 1   | Senseair S8 (LP)           | CO2 sensor (Modbus RTU)         |
| 1   | Sensirion SHT35            | Temperature / humidity (I2C)    |
| 2   | DC fan motor + driver      | Ventilation actuator (PWM)      |
| 1   | Raspberry Pi 4             | MQTT broker (Mosquitto) + Node-RED |

## Sensor node — pinout

| Signal            | ESP32 pin | Notes                              |
|-------------------|-----------|------------------------------------|
| S8 UART2 RX (RXD2)| GPIO16    | to S8 TX                           |
| S8 UART2 TX (TXD2)| GPIO17    | to S8 RX, 9600 baud, `SERIAL_8N1`  |
| SHT35 SDA         | GPIO21    | I2C data, address `0x44` (ADDR low)|
| SHT35 SCL         | GPIO22    | I2C clock                          |

- The S8 is read with Modbus RTU (function 0x04, register 0x08) with CRC-16 validation.
- SHT35 address is `0x44` by default; tie ADDR high for `0x45`.

## Fan controller — pinout

| Signal            | ESP32 pin | Notes                                  |
|-------------------|-----------|----------------------------------------|
| Motor R PWM       | GPIO27    | `analogWrite`, 0–255 (255 = off)       |
| Motor L PWM       | GPIO26    | `analogWrite`, 0–255 (255 = off)       |
| UART2 RX/TX       | GPIO16/17 | optional UART command path, 57600 baud |

PWM is **inverted** on this motor driver: `255` = no power, `0` = maximum speed. The
controller computes PWM as `speed × 20` from the received fan frame (see
[protocol.md](protocol.md)).

## Power & notes

- Power the fan motors from a supply appropriate for the driver, not the ESP32 3V3 rail.
- Common-ground the ESP32, sensors, and motor driver.
- WiFi/MQTT settings are provided per sketch via a git-ignored `secrets.h`
  (copy from `secrets.example.h`).
