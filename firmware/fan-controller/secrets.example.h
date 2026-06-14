// secrets.example.h
//
// Copy this file to "secrets.h" (same directory as the sketch) and fill in your
// own values. "secrets.h" is git-ignored so real credentials never get committed.
#ifndef SECRETS_H
#define SECRETS_H

// --- WiFi ---
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// --- MQTT broker ---
#define MQTT_BROKER_IP "192.168.X.X"
#define MQTT_PORT      1883
#define MQTT_TOPIC     "esp32/output"

// Leave both empty ("") for an anonymous broker. Set them when the broker
// (e.g. Mosquitto) requires authentication via a password file.
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""

#endif // SECRETS_H
