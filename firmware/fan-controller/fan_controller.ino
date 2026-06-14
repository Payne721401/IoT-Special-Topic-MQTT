/**
 * Fan controller node.
 *
 * Subscribes to the MQTT topic, decodes "55,Bike01,M_R/M_L,<speed>,ED" frames
 * (also accepted over UART2) and drives two DC fan motors via PWM. See
 * ../../docs/protocol.md for the wire format and ../../docs/hardware.md for the
 * pinout.
 *
 * The original dual-motor / UART skeleton is based on an example by
 * JeffHu of XY_ELECCONTROL.
 */
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <analogWrite.h>

// Copy secrets.example.h to secrets.h and fill in your WiFi/MQTT settings.
#include "secrets.h"

String Msg[32] =""; // parsed message fields: [0]=55 [1]=Bike01 [2]=M_R/M_L [3]=speed [4]=ED
String message,Message_Mqtt;
String UART_RX_Buff="Rx_Buff is empty..";
unsigned char UART_Recv_Len =0;
char c ='0';
int commaPosition=-1;
int Msg_index=0;
#define protocol SERIAL_8N1
#define RXD2        16
#define TXD2        17

// WiFi and MQTT settings come from secrets.h (see secrets.example.h).
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
const char *mqtt_broker = MQTT_BROKER_IP;
const char *topic = MQTT_TOPIC;
const char *mqtt_username = MQTT_USERNAME;
const char *mqtt_password = MQTT_PASSWORD;
const int mqtt_port = MQTT_PORT;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
 pinMode(26, OUTPUT);analogWrite(26,255); // motor on D26 — 255 = off
 pinMode(27, OUTPUT);analogWrite(27,255); // motor on D27 — 255 = off
 Serial.begin(57600,protocol,RXD2,TXD2);  // init UART2
 Serial.setDebugOutput(true);

 // Connect to WiFi
 Serial.println("Starting connecting WiFi.");
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.println(".");
 }
 Serial.print("Connected to the WiFi network");
 Serial.println("IP address: ");
 Serial.println(WiFi.localIP());

 // Connect to the MQTT broker
 client.setServer(mqtt_broker, mqtt_port);
 client.setCallback(callback);
 while (!client.connected()) {
     String client_id = "esp32-client-";
     client_id += String(WiFi.macAddress());
     Serial.printf("The client %s connects to the mqtt broker\n", client_id.c_str());
     if (client.connect(client_id.c_str())) {
         Serial.println("MQTT broker connected");
     } else {
         Serial.print("failed with state \r\n");
         Serial.print(client.state());
         delay(2000);
     }
 }
 client.publish(topic, "Hi ideaSky");
 client.subscribe(topic);
}

void callback(char *topic, byte *payload, unsigned int length) 
{
 Message_Mqtt =""; 
 Serial.print("Topic: ");
 Serial.println(topic);
 Serial.println("Message:");
 for (int i = 0; i < length; i++) 
 {
     Message_Mqtt += (char)payload[i];     
 }
 Serial.println(Message_Mqtt);
 UART_Recv_Decode(Message_Mqtt);
}

void loop() 
{
  client.loop();  
  char c ='0';
  if(Serial.available())
  {
    message ="";UART_RX_Buff="";  
    //Msg_index =0;
    while(c!='\n' && Serial.available())
    {
      c = (char) Serial.read();
      UART_RX_Buff += c;
      message += c;
      UART_Recv_Len++;
      //delay(1);
      delayMicroseconds(500);      
    }  
    //Serial.println(message);      
  } 
  if(UART_Recv_Len) {UART_Recv_Decode(message);UART_Recv_Len =0;}
//  delay(2000);
}

// Drive both fan motors from a decoded "55,Bike01,M_*,<speed>,ED" frame.
// A single command currently drives both motors together; per-motor control
// (M_R / M_L) can be reinstated by branching on Motor_Msg[2].
void MotorDeode(String* Motor_Msg)
{
  if(Motor_Msg[0] =="55" && Motor_Msg[1] =="Bike01" && Motor_Msg[4] =="ED")
  {
      // PWM range is 0..255 (255 = off, 0 = max speed); speed field is 0..255/20.
      uint16_t M_R = Motor_Msg[3].toInt()*20;
      analogWrite(27,M_R); // right motor RPM
      analogWrite(26,M_R); // left motor RPM
  }
}

void UART_Recv_Decode(String Buff)
{
  Msg_index =0;
  for(uint8_t I=0;I<16;I++) Msg[I]="";
  do
  {
    commaPosition = Buff.indexOf(',');
    if(commaPosition != -1){
        Msg[Msg_index]=Buff.substring(0,commaPosition);
        //Serial.println(Msg[Msg_index]);
        Buff = Buff.substring(commaPosition+1, Buff.length());
    }
    else{
      if(Buff.length() >0){
        Msg[Msg_index]=Buff;
        //Serial.println(Msg[Msg_index]);
      }
    }      
    if(Msg_index <32) Msg_index++;
    delayMicroseconds(10);   
  }while( commaPosition >0);
  if(Msg_index) 
  {
    for(uint8_t I=0;I<Msg_index;I++)
    {
      Serial.print(Msg[I] + "+");//Serial.print("+");
    } 
    Serial.print("");
    MotorDeode(&Msg[0]);
  }
}
