#!/usr/bin/env python3
"""Simulate the ESP32 sensor node without any hardware.

Publishes realistic Temperature / Humidity / CO2 frames to the MQTT topic and,
like the real firmware, derives a fan-speed command from the CO2 level and
publishes it only when it changes. Frame format mirrors docs/protocol.md so the
Node-RED functions and the real fan-controller would accept these messages.

Works against any broker (local Mosquitto, the public test broker, etc.):

    pip install paho-mqtt
    python sensor_simulator.py --host localhost
    python sensor_simulator.py --host test.mosquitto.org --topic demo/esp32/output
"""
import argparse
import random
import time

import paho.mqtt.client as mqtt


def sensor_frame(device_id: str, sensor_type: str, value) -> str:
    """Build a "0x55, id, type, value, 0xED" frame (see docs/protocol.md)."""
    return f"0x55, {device_id}, {sensor_type}, {value}, 0xED"


def fan_frame(side: str, speed: int) -> str:
    """Build a "55,Bike01,M_<side>,<speed>,ED" fan command."""
    return f"55,Bike01,M_{side},{speed},ED"


def co2_to_fan_speed(ppm: int) -> int:
    """Control law: CO2 ppm -> fan speed (0 = full power .. 5 = low, 25 = off)."""
    if ppm >= 2000:
        return 0
    if ppm >= 1700:
        return 2
    if ppm >= 1300:
        return 3
    if ppm >= 1000:
        return 4
    if ppm >= 600:
        return 5
    return 25


def main() -> None:
    parser = argparse.ArgumentParser(description="ESP32 sensor-node simulator")
    parser.add_argument("--host", default="localhost", help="MQTT broker host")
    parser.add_argument("--port", type=int, default=1883, help="MQTT broker port")
    parser.add_argument("--topic", default="esp32/output", help="MQTT topic")
    parser.add_argument("--interval", type=float, default=2.0, help="seconds between samples")
    args = parser.parse_args()

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.connect(args.host, args.port, keepalive=60)
    client.loop_start()
    print(f"Publishing to {args.host}:{args.port} on '{args.topic}' (Ctrl-C to stop)\n")

    co2 = 500.0           # ppm, random-walks over time
    last_fan_speed = None
    try:
        while True:
            # Drift the readings to look like a real room.
            co2 = max(400.0, min(2500.0, co2 + random.uniform(-60, 90)))
            temp = 24.0 + random.uniform(-1.5, 1.5)
            humid = 55.0 + random.uniform(-5, 5)
            co2_ppm = int(co2)

            client.publish(args.topic, sensor_frame("Temperature", "SHT35", f"{temp:.2f}"))
            client.publish(args.topic, sensor_frame("Humidity", "SHT35", f"{humid:.2f}"))
            client.publish(args.topic, sensor_frame("0xD11704d", "S8", co2_ppm))

            speed = co2_to_fan_speed(co2_ppm)
            fan_note = ""
            if speed != last_fan_speed:
                client.publish(args.topic, fan_frame("R", speed))
                last_fan_speed = speed
                fan_note = f"  -> fan speed {speed}"

            print(f"CO2 {co2_ppm:>4} ppm | {temp:5.2f} C | {humid:5.2f} %{fan_note}")
            time.sleep(args.interval)
    except KeyboardInterrupt:
        print("\nStopping.")
    finally:
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
