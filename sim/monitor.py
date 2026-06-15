#!/usr/bin/env python3
"""Subscribe to the MQTT topic and print decoded frames — a tiny stand-in for
the Node-RED dashboard, so the whole pipeline can be seen without hardware.

    pip install paho-mqtt
    python monitor.py --host localhost
    python monitor.py --host test.mosquitto.org --topic demo/esp32/output
"""
import argparse

import paho.mqtt.client as mqtt

# Latest value per channel, refreshed as frames arrive.
latest = {"Temperature": "-", "Humidity": "-", "CO2": "-", "Fan": "-"}


def parse_frame(message: str):
    """Decode "START, id, type, value, END"; return (id, type, value) or None."""
    parts = [p.strip() for p in message.split(",")]
    if len(parts) != 5:
        return None
    if parts[0] not in ("0x55", "55") or parts[4] not in ("0xED", "ED"):
        return None
    return parts[1], parts[2], parts[3]


def on_connect(client, userdata, flags, reason_code, properties):
    topic = userdata["topic"]
    client.subscribe(topic)
    print(f"Subscribed to '{topic}' on {userdata['host']} (Ctrl-C to stop)\n")


def on_message(client, userdata, msg):
    decoded = parse_frame(msg.payload.decode(errors="replace"))
    if decoded is None:
        return
    device_id, _sensor_type, value = decoded
    if device_id == "Temperature":
        latest["Temperature"] = value
    elif device_id == "Humidity":
        latest["Humidity"] = value
    elif device_id == "0xD11704d":
        latest["CO2"] = value
    elif device_id == "Bike01":
        latest["Fan"] = value

    print(
        f"CO2 {latest['CO2']:>5} ppm | "
        f"Temp {latest['Temperature']:>6} C | "
        f"Humidity {latest['Humidity']:>6} % | "
        f"Fan {latest['Fan']:>3}"
    )


def main() -> None:
    parser = argparse.ArgumentParser(description="MQTT pipeline monitor")
    parser.add_argument("--host", default="localhost", help="MQTT broker host")
    parser.add_argument("--port", type=int, default=1883, help="MQTT broker port")
    parser.add_argument("--topic", default="esp32/output", help="MQTT topic")
    args = parser.parse_args()

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2,
                         userdata={"topic": args.topic, "host": args.host})
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(args.host, args.port, keepalive=60)
    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print("\nStopping.")
        client.disconnect()


if __name__ == "__main__":
    main()
