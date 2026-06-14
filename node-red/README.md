# Node-RED

The Raspberry Pi runs Node-RED to parse the MQTT frames and drive a dashboard.

## Function nodes

Each file in [`functions/`](functions/) is the body of a Node-RED **function node** that
subscribes to `esp32/output` and extracts one value from the frame
(see [../docs/protocol.md](../docs/protocol.md)):

| File                      | Extracts                          |
|---------------------------|-----------------------------------|
| `functions/temperature.js`| temperature (°C) from SHT35 frames |
| `functions/humidity.js`   | humidity (%RH) from SHT35 frames   |
| `functions/co2.js`        | CO2 (ppm) from S8 frames           |
| `functions/fan.js`        | fan command, remapped for display  |

## Exporting the full flow

Only the function bodies are committed. To capture the complete wiring:

1. In the Node-RED editor: **menu → Export → all flows → Download**.
2. Save the file as `flows/flows.json` in this folder and commit it.
