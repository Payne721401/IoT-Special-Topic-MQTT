# `protocol` library

Hardware-independent C++ for the project's wire protocol and control logic. It has **no
Arduino, WiFi or hardware dependencies**, so the same code that runs on the ESP32 is compiled
and unit-tested on a host machine in CI.

## API (`protocol.h`)

| Function | Purpose |
|----------|---------|
| `split_fields` / `parse_frame` | decode `0x55, id, type, value, 0xED` frames (accepts `55`/`ED` too) |
| `encode_sensor_frame` / `encode_fan_frame` | build the on-the-wire strings |
| `co2_to_fan_speed` | CO2 ppm → fan speed control law |
| `fan_speed_to_display` | firmware speed → 0..5 dashboard scale |
| `modbus_crc` | CRC-16/MODBUS for the Senseair S8 sensor |

## Why it's a separate library

Pulling framing, the control law and the CRC out of the `.ino` sketches means:

- the logic can be **tested without hardware** (`pio test -e native`), and
- both ESP32 nodes and the Node-RED layer share one canonical, documented definition.

See [`../../test/test_protocol/`](../../test/test_protocol/) for the test suite and
[`../../docs/protocol.md`](../../docs/protocol.md) for the protocol spec.
