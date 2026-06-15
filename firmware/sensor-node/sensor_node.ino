// Sensor node: reads a Senseair S8 CO2 sensor (Modbus RTU over UART2) and an
// SHT35 temperature/humidity sensor (I2C), publishes the readings to MQTT, and
// derives a CO2-driven fan-speed command. All framing, the CRC and the control
// law live in lib/protocol (host-unit-tested). See docs/protocol.md and
// docs/hardware.md.
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArtronShop_SHT3x.h>

#include "secrets.h"      // copy secrets.example.h -> secrets.h
#include "protocol.h"     // lib/protocol
#include "net_mqtt.h"     // lib/net

// S8 CO2 sensor — UART2 pins
#define TXD2 17
#define RXD2 16
// SHT35 temp/humidity sensor — I2C pins
#define SDA_PIN 21
#define SCL_PIN 22

static ArtronShop_SHT3x sht35(0x44, &Wire);  // ADDR low => 0x44, high => 0x45

static WiFiClient espClient;
static PubSubClient client(espClient);

// S8 Modbus "read CO2" request (function 0x04, register 0x03) incl. CRC.
static byte CO2req[] = {0xFE, 0x04, 0x00, 0x03, 0x00, 0x01, 0xD5, 0xC5};
static byte response[20];

static int lastFanSpeed = -1;  // publish a fan command only when it changes

// Send a Modbus request until the sensor starts replying.
static void sendRequest(byte* request, int len) {
    while (!Serial1.available()) {
        Serial1.write(request, len);
        delay(50);
    }
}

// Read `len` bytes of the Modbus response into response[].
static void readResponse(int len) {
    int waited = 0;
    while (Serial1.available() < 7) {
        if (++waited > 10) {
            while (Serial1.available()) Serial1.read();
            break;
        }
        delay(50);
    }
    for (int i = 0; i < len; i++) response[i] = Serial1.read();
}

// Return CO2 in ppm, or -1 if the response fails its CRC check.
static long readCO2() {
    sendRequest(CO2req, sizeof(CO2req));
    readResponse(7);
    const uint16_t readCrc = (uint16_t)response[6] * 256 + (uint16_t)response[5];
    if (protocol::modbus_crc(response, 5) != readCrc) return -1;
    return (uint16_t)response[3] * 256 + (uint16_t)response[4];
}

// MQTT subscribe callback (informational only for this node).
static void callback(char* topic, byte* payload, unsigned int length) {
    Serial.printf("MQTT [%s] ", topic);
    for (unsigned int i = 0; i < length; i++) Serial.print((char)payload[i]);
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);  // UART to Senseair S8

    net::wifiConnect(WIFI_SSID, WIFI_PASSWORD);

    while (!sht35.begin()) {
        Serial.println("SHT35 not found!");
        delay(2000);
    }

    client.setServer(MQTT_BROKER_IP, MQTT_PORT);
    client.setCallback(callback);
    net::mqttEnsureConnected(client, MQTT_TOPIC);
    client.publish(MQTT_TOPIC, "Hi I'm ESP32 ^^");  // announce presence
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) net::wifiConnect(WIFI_SSID, WIFI_PASSWORD);
    net::mqttEnsureConnected(client, MQTT_TOPIC);
    client.loop();

    if (!sht35.measure()) Serial.println("SHT35 measurement failed");
    const float temp = sht35.temperature();
    const float humid = sht35.humidity();
    const long co2 = readCO2();
    Serial.printf("CO2: %ld ppm, Temp: %.2f C, Humidity: %.2f %%\n", co2, temp, humid);

    char value[16];
    snprintf(value, sizeof(value), "%.2f", temp);
    client.publish(MQTT_TOPIC, protocol::encode_sensor_frame("Temperature", "SHT35", value).c_str());
    snprintf(value, sizeof(value), "%.2f", humid);
    client.publish(MQTT_TOPIC, protocol::encode_sensor_frame("Humidity", "SHT35", value).c_str());

    if (co2 >= 0) {
        snprintf(value, sizeof(value), "%ld", co2);
        client.publish(MQTT_TOPIC, protocol::encode_sensor_frame("0xD11704d", "S8", value).c_str());

        const int speed = protocol::co2_to_fan_speed((int)co2);
        if (speed != lastFanSpeed) {
            client.publish(MQTT_TOPIC, protocol::encode_fan_frame('R', speed).c_str());
            lastFanSpeed = speed;
        }
    }

    delay(2000);
}
