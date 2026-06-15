// Fan controller node.
//
// Subscribes to the MQTT topic, decodes "55,Bike01,M_R/M_L,<speed>,ED" frames
// (also accepted over UART2) and drives two DC fan motors via PWM. Frame parsing
// lives in lib/protocol (host-unit-tested). See docs/protocol.md and
// docs/hardware.md.
//
// The original dual-motor / UART skeleton is based on an example by
// JeffHu of XY_ELECCONTROL.
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <analogWrite.h>

#include "secrets.h"      // copy secrets.example.h -> secrets.h
#include "protocol.h"     // lib/protocol
#include "net_mqtt.h"     // lib/net

#define RXD2 16
#define TXD2 17
#define MOTOR_R_PIN 27
#define MOTOR_L_PIN 26
#define MOTOR_OFF 255     // PWM is inverted: 255 = off, 0 = full power

static WiFiClient espClient;
static PubSubClient client(espClient);

// Decode a fan frame and drive both motors. Ignores anything that is not a
// well-formed "Bike01" command.
static void applyFanCommand(const String& message) {
    protocol::Frame frame;
    if (!protocol::parse_frame(message.c_str(), frame)) return;
    if (frame.id != "Bike01") return;

    int pwm = atoi(frame.value.c_str()) * 20;  // speed 0..(255/20) -> 0..255
    pwm = constrain(pwm, 0, 255);
    analogWrite(MOTOR_R_PIN, pwm);
    analogWrite(MOTOR_L_PIN, pwm);
}

static void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
    Serial.printf("MQTT [%s] %s\n", topic, message.c_str());
    applyFanCommand(message);
}

void setup() {
    pinMode(MOTOR_R_PIN, OUTPUT); analogWrite(MOTOR_R_PIN, MOTOR_OFF);
    pinMode(MOTOR_L_PIN, OUTPUT); analogWrite(MOTOR_L_PIN, MOTOR_OFF);
    Serial.begin(57600, SERIAL_8N1, RXD2, TXD2);  // UART2

    net::wifiConnect(WIFI_SSID, WIFI_PASSWORD);
    client.setServer(MQTT_BROKER_IP, MQTT_PORT);
    client.setCallback(callback);
    net::mqttEnsureConnected(client, MQTT_TOPIC, MQTT_USERNAME, MQTT_PASSWORD);
    client.publish(MQTT_TOPIC, "Hi ideaSky");
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) net::wifiConnect(WIFI_SSID, WIFI_PASSWORD);
    net::mqttEnsureConnected(client, MQTT_TOPIC, MQTT_USERNAME, MQTT_PASSWORD);
    client.loop();

    // Optionally accept the same frames over UART2.
    if (Serial.available()) {
        const String line = Serial.readStringUntil('\n');
        if (line.length()) applyFanCommand(line);
    }
}
