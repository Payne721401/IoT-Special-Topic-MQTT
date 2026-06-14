# Wire Protocol

All nodes communicate on a single MQTT topic, **`esp32/output`**, using a flat, CSV-style
text frame. It is human-readable (easy to debug with `mosquitto_sub`) and trivial to parse in
both the firmware and Node-RED.

## Frame format

```
<START>, <ID>, <TYPE>, <VALUE>, <END>
```

| Field   | Meaning                                   | Example        |
|---------|-------------------------------------------|----------------|
| START   | Frame start marker                        | `0x55` / `55`  |
| ID      | Source/device identifier                  | `Temperature`, `0xD11704d`, `Bike01` |
| TYPE    | Sensor or channel type                    | `SHT35`, `S8`, `M_R`, `M_L` |
| VALUE   | The measurement or command value          | `24.5`, `823`, `3` |
| END     | Frame end marker                          | `0xED` / `ED`  |

Fields are separated by commas. Sensor frames use spaced markers
(`0x55, ... , 0xED`); fan-command frames use compact markers (`55,...,ED`). Parsers
`trim()` each field, so both forms are accepted.

## Message catalogue

| Purpose      | Frame                                         | Notes |
|--------------|-----------------------------------------------|-------|
| Temperature  | `0x55, Temperature, SHT35, <°C>, 0xED`         | from SHT35 |
| Humidity     | `0x55, Humidity, SHT35, <%RH>, 0xED`           | from SHT35 |
| CO2          | `0x55, 0xD11704d, S8, <ppm>, 0xED`             | `0xD11704d` is the S8 device id |
| Fan (right)  | `55, Bike01, M_R, <speed>, ED`                 | drives the fan motors |
| Fan (left)   | `55, Bike01, M_L, <speed>, ED`                 | reserved; not published today |

## Fan speed scale

The firmware speed value is **inverted**: a lower number means more power.

| `speed` | Meaning            | Motor PWM (`speed × 20`, 0–255) |
|---------|--------------------|---------------------------------|
| 0       | Full power         | 0   (0 = strongest on this driver) |
| 2       | High               | 40  |
| 3       | Medium             | 60  |
| 4       | Low                | 80  |
| 5       | Lowest             | 100 |
| 25      | Off (no power)     | 255 (255 = motor off) |

The Node-RED `fan.js` function remaps this to an intuitive **0 = off … 5 = full power** scale
for display.

## CO2 → fan-speed control law

The sensor node maps CO2 concentration to a fan speed (see
[`firmware/sensor-node/sensor_node.ino`](../firmware/sensor-node/sensor_node.ino)):

| CO2 (ppm)        | `speed` |
|------------------|---------|
| ≥ 2000           | 0  (full power) |
| 1700 – 1999      | 2  |
| 1300 – 1699      | 3  |
| 1000 – 1299      | 4  |
| 600 – 999        | 5  |
| < 600            | 25 (off) |

A new fan command is only published when the resulting level **changes**, to reduce traffic.

## Modbus note (S8)

The CO2 value is read from the S8 sensor over Modbus RTU (function 0x04, register 0x08) with
CRC-16 validation before it is framed and published.
