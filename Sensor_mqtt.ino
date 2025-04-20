//include必要函式庫
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArtronShop_SHT3x.h>

//Define S8腳位
#define TXD2 17
#define RXD2 16

//Define SHT35腳位
#define SDA_PIN 21 
#define SCL_PIN 22 
ArtronShop_SHT3x sht35(0x44, &Wire); // ADDR: 0 => 0x44, ADDR: 1 => 0x45

//publish用的全域變數
String CO2_pub = "0x55, 0xD11704d, S8, ";
String temp_pub = "0x55, Temperature, SHT35, ";
String humid_pub = "0x55, Humidity, SHT35, ";
String ending = ", 0xED";
String fan_old = "None";

//S8的全域變數
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

//WiFi和MQTT Broker id(註解可方便切換不同網域使用)
const char *ssid = "ATM-ideasky"; // Enter your WiFi name
//const char *ssid = "Payne";
const char *password = "ideasky@atm";  // Enter WiFi password
//const char *password = "00000000";
const char *mqtt_broker = "192.168.50.107";
//const char *mqtt_broker = "test.mosquitto.org";
const char *topic = "esp32/output";
//const char *mqtt_username = "emqx";
//const char *mqtt_password = "public";
const int mqtt_port = 1883;

///////////////

//通過Wifi連接至mqtt
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

//S8讀取request
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
//以上這段我貼別人的我也沒很懂，時間關係之後有機會再改


void setup() {
  // Set software serial baud to 115200;
 Wire.begin();
 Serial.begin(115200);
 
 
 // connecting to a WiFi network
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.println("Connecting to WiFi..");  //Check wifi連接狀態
 }
 Serial.println("Connected to the WiFi network");
 
  while (!sht35.begin()) {
    Serial.println("SHT35 not found !");  //check sht35連接狀態
    delay(2000);
  }

 while (!sht35.measure()){
    Serial.println("SHT35 Measurement failed");
    delay(2000);
 }
 
 //connecting to a mqtt broker 連接至mqtt broker
 client.setServer(mqtt_broker, mqtt_port);
 client.setCallback(callback);
 while (!client.connected()) {
     String client_id = "esp32-client-"; //設定client id
     client_id += String(WiFi.macAddress());
     Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
     if (client.connect(client_id.c_str())) {  //確認broker連接狀態
         
         Serial.println("Public mqtt broker connected");
     } else {
         Serial.print("failed with state ");
         Serial.print(client.state());
         delay(2000);
     }
 }
 // mqtt broker publish and subscribe
 client.subscribe(topic); //訂閱你要的topic
 client.publish(topic, "Hi I'm ESP32 ^^"); //有成功連接會先publish這段至broker
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);      // UART to Sensair CO2 Sensor


  //以下這段都是處理S8的set up，很冗，之後有時間再改
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
 //以上這段都是處理S8的set up，很冗，之後有時間再改

//mqtt broker的call back 函式(處理訂閱的broker接收到的訊息)
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
  send_Request(CO2req, 8);               // send request for CO2-Data to the Sensor
  read_Response(7);                      // receive the response from the Sensor
  
  //SHT35溫溼度讀取&定義接收到的資料值
  if (!sht35.measure()){
    Serial.println("SHT35 Measurement failed");
  }
  float temp = sht35.temperature();  
  float humid = sht35.humidity();

  //S8的co2濃度讀取
  unsigned long CO2 = get_Value(7);
  delay(2000);

  //print感測器資料
  String CO2s = "CO2: " + String(CO2);
  String Temperature = "Temperature: " + String(temp);
  String Humidity = "Humidity: " + String(humid);
  Serial.println(CO2s);
  Serial.println(Temperature);
  Serial.println(Humidity);

//  將接收到的訊息處理成字串
  temp_pub = temp_pub+String(temp)+ending;
  humid_pub = humid_pub+String(humid)+ending;
  if (CO2<1000){
    CO2_pub = CO2_pub+String(0)+String(CO2)+ending;
  }
  else{
    CO2_pub = CO2_pub+String(CO2)+ending;
  }

//SHT35 Version
// if ((co2_conc >= 2000) or (temp > 35)){
//   trun_fan = "0x55, 0xD11704d, 0xD2, fan_cw, 100";
//   }
//  else if ((co2_conc >= 1500) or (temp > 32)){
//   trun_fan = "0x55, 0xD11704d, 0xD2, fan_cw, 60";
//   }
//  else if ((co2_conc >= 1000) or (temp > 30)){
//   trun_fan = "0x55, 0xD11704d, 0xD2, fan_cw, 80";
//   }
//  else if ((co2_conc >= 600) or (temp > 27)){
//   trun_fan = "0x55, 0xD11704d, 0xD2, fan_cw, 20";
//   }
//  else{
//   trun_fan = "0x55, 0xD11704d, 0xD2, fan_cw, 0";
//   }


// if ((CO2 >= 2000) ){
//   trun_fan = "0x55, 0xD11704d, 0xD2, fan_cw, 100";
//   }
//  else if ((CO2 >= 1500)){
//   trun_fan = "0x55, 0xD11704d, 0xD2, fan_cw, 80";
//   }
//  else if ((CO2 >= 1000)){
//   trun_fan = "0x55, 0xD11704d, 0xD2, fan_cw, 60";
//   }
//  else if ((CO2 >= 600)){
//   trun_fan = "0x55, 0xD11704d, 0xD2, fan_cw, 20";
//   }
//  else{
//   trun_fan = "0x55, 0xD11704d, 0xD2, fan_cw, 0";
//   }

//根據CO2濃度決定風扇強度(0最強5最弱)，輸出格式可以改(需同時調整fan control)

  String turn_fan_R;
  String turn_fan_L;
  if ((CO2 >= 2000) ){
   turn_fan_R = "55,Bike01,M_R,0,ED";
   turn_fan_L = "55,Bike01,M_L,0,ED"; //fan_L目前不publish，需分開控制風扇時可再調整
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
   turn_fan_R = "55,Bike01,M_R,25,ED"; //不給電
   turn_fan_L = "55,Bike01,M_L,25,ED";
   }

  //將數據轉換為可publish的格式
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
  
  //publish溫濕度/CO2濃度/風扇控制訊息
  client.publish(topic, Buf_temp);
  client.publish(topic, Buf_humid);
  client.publish(topic, Buf_co2);
   if(turn_fan_R != fan_old){
    client.publish(topic, fan_R); //濃度達到門檻再publish新的風扇控制訊息
  }
  delay(500);
  fan_old = turn_fan_R;
  client.loop();
}
