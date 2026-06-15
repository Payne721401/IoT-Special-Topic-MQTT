// net_mqtt.h — shared WiFi + MQTT connection helpers for the ESP32 sketches.
//
// Header-only so both sketches can reuse it. Depends on the Arduino/ESP32 stack,
// so it is only compiled for the firmware environments (never the native tests).
#ifndef NET_MQTT_H
#define NET_MQTT_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

namespace net {

// Blocking WiFi connect (used once at startup). Returns when associated.
inline void wifiConnect(const char* ssid, const char* password) {
    if (WiFi.status() == WL_CONNECTED) return;
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print('.');
    }
    Serial.printf("\nWiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
}

// Non-blocking MQTT (re)connect. Attempts at most once every `retryMs`,
// re-subscribing to `topic` on success. Returns true while connected.
inline bool mqttEnsureConnected(PubSubClient& client, const char* topic,
                                const char* user = "", const char* pass = "",
                                unsigned long retryMs = 2000) {
    if (client.connected()) return true;

    static unsigned long lastAttempt = 0;
    const unsigned long now = millis();
    if (lastAttempt != 0 && now - lastAttempt < retryMs) return false;
    lastAttempt = now;

    const String clientId = "esp32-client-" + String(WiFi.macAddress());
    const bool ok = (user && user[0])
                        ? client.connect(clientId.c_str(), user, pass)
                        : client.connect(clientId.c_str());
    if (ok) {
        Serial.println("MQTT connected");
        client.subscribe(topic);
    } else {
        Serial.printf("MQTT connect failed, state=%d\n", client.state());
    }
    return ok;
}

}  // namespace net

#endif  // NET_MQTT_H
