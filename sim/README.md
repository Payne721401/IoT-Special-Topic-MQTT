# Simulation — run the pipeline without hardware

No ESP32 and no Raspberry Pi required. A Python **sensor simulator** publishes realistic
frames (same format as the firmware — see [../docs/protocol.md](../docs/protocol.md)) and a
**monitor** subscribes and prints the decoded values, standing in for the Node-RED dashboard.

```
sensor_simulator.py ──publish──▶  MQTT broker  ──▶  monitor.py (and/or Node-RED)
```

## 1. Install the client

```bash
pip install -r requirements.txt   # just paho-mqtt
```

## 2. Pick a broker (no Docker needed)

**Option A — local Mosquitto (recommended, works offline)**

Install the Mosquitto broker for your OS (Windows installer / `brew install mosquitto` /
`apt install mosquitto`), then start it:

```bash
mosquitto -v          # listens on localhost:1883
```

Use `--host localhost` (the default) for the scripts below.

**Option B — public test broker (zero install)**

Use `test.mosquitto.org`. It's shared and public, so use a unique topic so you don't collide
with other people:

```bash
--host test.mosquitto.org --topic <your-name>/esp32/output
```

## 3. Run it

In two terminals (pass the **same** `--host`/`--topic` to both):

```bash
python monitor.py            --host localhost
python sensor_simulator.py   --host localhost
```

The monitor prints a live line as CO2 drifts and the fan command changes:

```
CO2  1180 ppm | Temp  24.13 C | Humidity  53.80 % | Fan   4
```

You can also point the real Node-RED functions (in `../node-red/functions/`) at the same
broker/topic to drive the actual dashboard.

> Tip: `mosquitto_sub -t 'esp32/output' -v` (from the Mosquitto tools) is another quick way to
> watch the raw frames.
