// Sensor node: reads a Senseair S8 CO2 sensor (Modbus RTU over UART2) and an
// SHT35 temperature/humidity sensor (I2C), publishes the readings to MQTT, and
// derives a CO2-driven fan-speed command. See ../../docs/protocol.md for the
// wire format and ../../docs/hardware.md for the pinout.
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArtronShop_SHT3x.h>

// Copy secrets.example.h to secrets.h and fill in your WiFi/MQTT settings.
#include "secrets.h"

// S8 CO2 sensor — UART2 pins
#define TXD2 17
#define RXD2 16

// SHT35 temp/humidity sensor — I2C pins
#define SDA_PIN 21
#define SCL_PIN 22
ArtronShop_SHT3x sht35(0x44, &Wire); // ADDR: 0 => 0x44, ADDR: 1 => 0x45

// Frame prefixes/suffix for the published messages (see docs/protocol.md)
String CO2_pub = "0x55, 0xD11704d, S8, ";
String temp_pub = "0x55, Temperature, SHT35, ";
String humid_pub = "0x55, Humidity, SHT35, ";
String ending = ", 0xED";
String fan_old = "None";

// S8 Modbus state
byte CO2req[] = {0xFE, 0X04, 0X00, 0X03, 0X00, 0X01, 0XD5, 0XC5};
byte Response[20];
uint16_t crc_02;
char variable[4];
int ASCII_WERT;
int int01, int02, int03;
unsigned long ReadCRC;      // CRC Control Return Code 

// Ausselesen ABC-Periode / FirmWare / SensorID 
byte ABCreq[] = {0xFE, 0X03, 0X00, 0X1F, 0X00, 0X01, 0XA1, 0XC3};   // 1f in Hex -> 31 dezimal
byte FWreq[] = {0xFE, 0x04, 0x00, 0x1C, 0x00, 0x01, 0xE4, 0x03};    // FW      ->       334       1 / 78
byte ID_Hi[] = {0xFE, 0x04, 0x00, 0x1D, 0x00, 0x01, 0xB5, 0xC3};    // Sensor ID hi    1821       7 / 29
byte ID_Lo[] = {0xFE, 0x04, 0x00, 0x1E, 0x00, 0x01, 0x45, 0xC3};    // Sensor ID lo   49124     191 / 228 e.g. in Hex 071dbfe4
//byte Tstreq[] = {0xFE, 0x03, 0x00, 0x01, 0x00, 0x01, 0xE4, 0x03};   // Read the holding register
//byte Tstreq[] = {0xFE, 0x04, 0x00, 0x01, 0x00, 0x01, 0xE4, 0x03}; // Read the input Register
byte Tstreq[] = {0xFE, 0x44, 0X00, 0X01, 0X02, 0X9F, 0X25};         // undocumented function 44
int Test_len = 7;               // length of Testrequest in case of function 44 only 7 otherwise 8
////////////////////

// WiFi and MQTT settings come from secrets.h (see secrets.example.h).
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
const char *mqtt_broker = MQTT_BROKER_IP;
const char *topic = MQTT_TOPIC;
const int mqtt_port = MQTT_PORT;

WiFiClient espClient;
PubSubClient client(espClient);

// Send request to S8-Sensor
void send_Request (byte * Request, int Re_len)
{
  while (!Serial1.available())
  {
    Serial1.write(Request, Re_len);   
    delay(50);
  }
}

// Read the S8 Modbus response into the Response[] buffer
void read_Response (int RS_len)
{
  int01 = 0;
  while (Serial1.available() < 7 ) 
  {
    int01++;
    if (int01 > 10)
    {
      while (Serial1.available())
        Serial1.read();
      break;
    }
    delay(50);
  }
  for (int02 = 0; int02 < RS_len; int02++)    // Empfangsbytes
  {
    Response[int02] = Serial1.read();
  }
}

unsigned short int ModBus_CRC(unsigned char * buf, int len)
{
  unsigned short int crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++) {
    crc ^= (unsigned short int)buf[pos];   // XOR byte into least sig. byte of crc
    for (int i = 8; i != 0; i--) {         // Loop over each bit
      if ((crc & 0x0001) != 0) {           // If the LSB is set
        crc >>= 1;                         // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // else LSB is not set
        crc >>= 1;                    // just shift right
    }
  }  // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
  return crc;  
}

unsigned long get_Value(int RS_len)
/* Extract the Value from the response (byte 3 and 4)  
    and check the CRC if the response is compleat and correct */
{
  /* Loop only for test and developtment purpose */
//  for (int i=0; i<RS_len-2; i++)
//  {
//    int03 = Response[i];
//    Serial.printf("Wert[%i] : %i / ", i, int03);
//  }

// Check the CRC //
  ReadCRC = (uint16_t)Response[RS_len-1] * 256 + (uint16_t)Response[RS_len-2];
  if (ModBus_CRC(Response, RS_len-2) == ReadCRC) {
    // Read the Value //
    unsigned long val = (uint16_t)Response[3] * 256 + (uint16_t)Response[4];
    return val * 1;       // S8 = 1. K-30 3% = 3, K-33 ICB = 10
  }
  else {
    Serial.print("Error");
    return 99;
  }
}
// NOTE: the S8 Modbus access above is a candidate to move into a dedicated,
// unit-tested driver.


void setup() {
  // Set software serial baud to 115200;
 Wire.begin();
 Serial.begin(115200);
 
 
 // connecting to a WiFi network
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.println("Connecting to WiFi..");
 }
 Serial.println("Connected to the WiFi network");

  while (!sht35.begin()) {
    Serial.println("SHT35 not found !");
    delay(2000);
  }

 while (!sht35.measure()){
    Serial.println("SHT35 Measurement failed");
    delay(2000);
 }
 
 // Connect to the MQTT broker (client id derived from the ESP32 MAC address)
 client.setServer(mqtt_broker, mqtt_port);
 client.setCallback(callback);
 while (!client.connected()) {
     String client_id = "esp32-client-";
     client_id += String(WiFi.macAddress());
     Serial.printf("The client %s connects to the mqtt broker\n", client_id.c_str());
     if (client.connect(client_id.c_str())) {
         Serial.println("MQTT broker connected");
     } else {
         Serial.print("failed with state ");
         Serial.print(client.state());
         delay(2000);
     }
 }
 client.subscribe(topic);
 client.publish(topic, "Hi I'm ESP32 ^^");  // announce presence on connect
  Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);      // UART to Senseair S8 CO2 sensor

  // One-time S8 diagnostic dump: ABC period, sensor ID, firmware, and all
  // holding registers. Useful when bringing up the sensor; harmless at runtime.
  Serial.println();
  Serial.print("ABC-Tage: ");  
  send_Request(ABCreq, 8);                     // Request ABC-Information from the Sensor
  read_Response(7);
  Serial.printf("%02ld", get_Value(7));
  Serial.println("");

  Serial.print("Sensor ID : ");                // 071dbfe4
  send_Request(ID_Hi, 8);
  read_Response(7);
  Serial.printf("%02x%02x", Response[3], Response[4]);
  send_Request(ID_Lo, 8);
  read_Response(7);
  Serial.printf("%02x%02x", Response[3], Response[4]);
  Serial.println("");

  Serial.print("FirmWare  : ");
  send_Request(FWreq, 8);             // send Request for Firmware to the Sensor
  read_Response(7);                   // get Response (Firmware 334) from the Sensor
  Serial.printf("%02d%02d", Response[3], Response[4]);
  Serial.println("");

 // Read all Register 
  for (int i=1; i<=31; i++)
  {
    Tstreq[3] = i;
    crc_02 = ModBus_CRC(Tstreq,Test_len-2);     // calculate CRC for request
    Tstreq[Test_len-1] = (uint8_t)((crc_02 >> 8) & 0xFF);
    Tstreq[Test_len-2] = (uint8_t)(crc_02 & 0xFF);
    delay(100);
    Serial.print("Registerwert : ");
    Serial.printf("%02x", Tstreq[0]);
    Serial.printf("%02x", Tstreq[1]);
    Serial.printf("%02x", Tstreq[2]);
    Serial.print(" ");
    Serial.printf("%02x", Tstreq[3]);
    Serial.print(" ");
    Serial.printf("%02x", Tstreq[Test_len-3]);
    Serial.printf("%02x", Tstreq[Test_len-2]);
    Serial.printf("%02x", Tstreq[Test_len-1]);

    send_Request(Tstreq, Test_len);
    read_Response(7);
    // Check if CRC is OK 
    ReadCRC = (uint16_t)Response[6] * 256 + (uint16_t)Response[5];
    if (ModBus_CRC(Response, Test_len-2) == ReadCRC) {
      Serial.printf(" -> (d) %03d %03d", Response[3], Response[4]);
      Serial.printf(" -> (x) %02x %02x", Response[3], Response[4]);
      Serial.printf(" ->(Wert) %05d", (uint16_t)Response[3] * 256 + (uint16_t)Response[4]);
      ASCII_WERT = (int)Response[3];
      Serial.print(" -> (t) ");
      if ((ASCII_WERT <= 128) and (ASCII_WERT >= 47)) 
      {
        Serial.print((char)Response[3]);
        Serial.print(" ");
        ASCII_WERT = (int)Response[4];
        if ((ASCII_WERT <= 128) and (ASCII_WERT >= 47)) 
        {
          Serial.print((char)Response[4]);
        }
        else
        {
          Serial.print(" ");
        }
      }
      else 
      {
        Serial.print("   ");
      }
      if ((Response[1] == 68) and (Tstreq[3] == 8))
         Serial.print("   CO2");      
      Serial.println("  Ende ");
    }
    else
      Serial.print("CRC-Error");
  }
}

// MQTT subscribe callback: logs any message received on the subscribed topic.
void callback(char *topic, byte *payload, unsigned int length) {
 Serial.print("Message arrived in topic: ");
 Serial.println(topic);
 Serial.print("Message:");
 for (int i = 0; i < length; i++) {
     Serial.print((char) payload[i]);
 }

 Serial.println();
 Serial.println("-----------------------");
}

void loop() {
  send_Request(CO2req, 8);               // request CO2 data from the S8 sensor
  read_Response(7);                      // receive the response

  // Read SHT35 temperature & humidity
  if (!sht35.measure()){
    Serial.println("SHT35 Measurement failed");
  }
  float temp = sht35.temperature();
  float humid = sht35.humidity();

  // Read CO2 concentration (ppm) from the S8 response
  unsigned long CO2 = get_Value(7);
  delay(2000);

  // Log the latest readings
  String CO2s = "CO2: " + String(CO2);
  String Temperature = "Temperature: " + String(temp);
  String Humidity = "Humidity: " + String(humid);
  Serial.println(CO2s);
  Serial.println(Temperature);
  Serial.println(Humidity);

  // Build the sensor frames (see docs/protocol.md)
  temp_pub = temp_pub+String(temp)+ending;
  humid_pub = humid_pub+String(humid)+ending;
  if (CO2<1000){
    CO2_pub = CO2_pub+String(0)+String(CO2)+ending;
  }
  else{
    CO2_pub = CO2_pub+String(CO2)+ending;
  }

  // Map CO2 (ppm) to a fan speed (0 = full power ... 5 = lowest, 25 = off).
  // Must stay in sync with the fan-controller. See docs/protocol.md.
  String turn_fan_R;
  String turn_fan_L;
  if ((CO2 >= 2000) ){
   turn_fan_R = "55,Bike01,M_R,0,ED";
   turn_fan_L = "55,Bike01,M_L,0,ED"; // M_L is not published today; kept for future split control
   }
  else if ((CO2 >= 1700)){
   turn_fan_R = "55,Bike01,M_R,2,ED";
   turn_fan_L = "55,Bike01,M_L,2,ED";
   }
  else if ((CO2 >= 1300)){
   turn_fan_R = "55,Bike01,M_R,3,ED";
   turn_fan_L = "55,Bike01,M_L,3,ED";
   }
  else if ((CO2 >= 1000)){
   turn_fan_R = "55,Bike01,M_R,4,ED";
   turn_fan_L = "55,Bike01,M_L,4,ED";
   }
  else if ((CO2 >= 600)){
   turn_fan_R = "55,Bike01,M_R,5,ED";
   turn_fan_L = "55,Bike01,M_L,5,ED";
   }
  else{
   turn_fan_R = "55,Bike01,M_R,25,ED"; // 25 = motor off
   turn_fan_L = "55,Bike01,M_L,25,ED";
   }

  // Convert the String frames into char buffers for publishing
  char Buf_co2[33];
  char Buf_temp[38];
  char Buf_humid[38];
  char fan_R[35];
  char fan_L[35];
  
  CO2_pub.toCharArray(Buf_co2, 33);
  temp_pub.toCharArray(Buf_temp, 38);
  humid_pub.toCharArray(Buf_humid, 38);
  CO2_pub = "0x55, 0xD11704d, S8, ";
  temp_pub = "0x55, Temperature, SHT35, ";
  humid_pub = "0x55, Humidity, SHT35, ";
  turn_fan_R.toCharArray(fan_R, 35);
  turn_fan_L.toCharArray(fan_L, 35);
  
  // Publish temperature / humidity / CO2, and the fan command on change
  client.publish(topic, Buf_temp);
  client.publish(topic, Buf_humid);
  client.publish(topic, Buf_co2);
   if(turn_fan_R != fan_old){
    client.publish(topic, fan_R); // only publish a new fan command when the level changes
  }
  delay(500);
  fan_old = turn_fan_R;
  client.loop();
}
