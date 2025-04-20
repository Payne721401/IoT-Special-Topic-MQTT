/**
 * A simple Mqtt_Client example for Aerobox_on_Bike and Dual-Motor control.
 * Designed By JeffHu of XY_ELECCONTROL
*/

//原始架構是胡老師做的所以有些部分我也不是很清楚

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <analogWrite.h> 

String Msg[32] =""; //Msg[1]=Temp;Msg[1]=RH;Msg[1]=Speed;Msg[1]=PM2_5....;
String message,Message_Mqtt;
String UART_RX_Buff="Rx_Buff is empty..";
unsigned char UART_Recv_Len =0;
char c ='0';
int commaPosition=-1;
int Msg_index=0;
#define protocol SERIAL_8N1
#define RXD2        16
#define TXD2        17

// 設定WiFi
const char *ssid = "ATM-ideasky"; 
const char *password = "ideasky@atm";  //@ideasky-wifi
//const char *ssid = "XY_ELECC"; 
//const char *password = "WG992119"; 

// 設定MQTT Broker
//const char *mqtt_broker = "bike.mqtt";
const char *mqtt_broker ="192.168.50.107";//192.168.50.126
//const char *mqtt_broker = "test.mosquitto.org";
const char *topic = "esp32/output";

const char *mqtt_username = "pi";
const char *mqtt_password = "3212261";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
 // Set software serial baud to 115200;
 //Serial.begin(115200);
 pinMode(26, OUTPUT);analogWrite(26,255); //輸出至D26
 pinMode(27, OUTPUT);analogWrite(27,255); //輸出至D27
 Serial.begin(57600,protocol,RXD2,TXD2);//Initionize UART2 
 Serial.setDebugOutput(true);
 
 // connecting to a WiFi network
 //Serial.begin(115200);
 Serial.println("Starting connecting WiFi.");
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.println(".");
 }
 Serial.print("Connected to the WiFi network");
 Serial.println("IP address: ");
 Serial.println(WiFi.localIP());
  
 //connecting to a mqtt broker
 client.setServer(mqtt_broker, mqtt_port);
 client.setCallback(callback);
 while (!client.connected()) {
     String client_id = "esp32-client-";
     client_id += String(WiFi.macAddress());
     Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
     if (client.connect(client_id.c_str())) {
         Serial.println("Public emqx mqtt broker connected");
     } else {
         Serial.print("failed with state \r\n");
         Serial.print(client.state());
         delay(2000);
     }
 }
 // publish and subscribe
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

//馬達控制，目前改為以一組訊息控制進出風扇開啟，有需要可再改回雙邊分開控制
void MotorDeode(String* Motor_Msg)
{
  if(Motor_Msg[0] =="55" && Motor_Msg[1] =="Bike01" && Motor_Msg[4] =="ED")
  {
      uint16_t M_R=0,M_L=0;
//      if(Motor_Msg[2] == "M_R") 
//      {
          M_R=Motor_Msg[3].toInt()*20;          //M_R的range從0~255, 255不給電, 0風速最大

          analogWrite(27,M_R); //Adjust R_RPM 
          analogWrite(26,M_R); //Adjust L_RPM    
//      } 
//      else if(Motor_Msg[2] == "M_L") 
//      {
//          M_L=Motor_Msg[3].toInt()*10;    
//          analogWrite(26,M_L); //Adjust L_RPM          
//      }                  
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
