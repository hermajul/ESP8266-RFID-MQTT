/*
 * pin layout used:
 * -----------------------------------------------------------------------------------------
 *             Sparkfun ESP8266   MFRC522      
 *             Thing Dev          Reader/PCD   
 * Signal      Pin                Pin                  
 * -----------------------------------------------------------------------------------------
 * RST/Reset   4                  RST    
 * SPI SS      15                 SDA(SS)      
 * SPI MOSI    13                 MOSI
 * SPI MISO    12                 MISO
 * SPI SCK     14                 SCK
 */
 
//RFID Reader
#include <SPI.h>
#include <MFRC522.h>
//Wifi & Mqtt
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>        //JSON Lib

void setup_wifi();
void callback(char* topic, byte* payload, unsigned int length);
void reconnect() ;
String ipToStr(IPAddress);
bool newCardAvailable();
 
//RFID Reader
#define RST_PIN         4          // Configurable, see typical pin layout above
#define SS_PIN          15         // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
String uidstr="";
char msg[100];
int mfrc522Help = 0;
int iState = 0;
 
//Wifi & Mqtt
//fill in the ssid and the password of your wifi
const char* ssid = "";
const char* password = "";
//fill in the ip-address of your mqtt server
const char* mqtt_server = "";
String clientMac = "";
String clientIP = "";
char topic[100];
 
WiFiClient espClient;
PubSubClient client(espClient);
 
char json[100];
 
void setup() {
  clientMac = WiFi.macAddress();
  ("rfid/reader/"+clientMac).toCharArray(topic,100);
  Serial.begin(9600);   // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  Serial.println(clientMac);
  //RFID Reader
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, type, and data blocks..."));
   
  //Wifi & Mqtt
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(5,OUTPUT);
}
 
void loop() {
 
  if (!client.connected()) {
    digitalWrite(5,HIGH);
    reconnect();
  }else{
    digitalWrite(5,LOW);
  }
  if(WiFi.status() == 6 || WiFi.status() == 4){
    setup_wifi();
  }
  client.loop();
    switch(iState){
      case 0:{if(newCardAvailable()) {
                iState++;
              }else{
                delay(50);
              }
              }
              break;
      case 1: {uidstr="";             
              for (byte i = 0; i < mfrc522.uid.size; i++){
                uidstr = uidstr + String(mfrc522.uid.uidByte[i],HEX);                         
              }
              iState++; 
              }
              break;
      case 2:{ Serial.print("the card id is: ");
              Serial.println(uidstr);
              StaticJsonBuffer<200> jsonBuffer;
              JsonObject& root = jsonBuffer.createObject();
              root["uid"]         = uidstr;
              root["mac"]         = clientMac;
              root["ip"]          = clientIP;
              root.printTo(msg);
              client.publish(topic, msg, 0);
              iState = 0;;
              } 
              break;
      default:;
     
  }
  delay(100);
  
}
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  String tmp ="";
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    tmp = tmp + (char)payload[i];
  }
  Serial.println(tmp);
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 1 seconds");
      // Wait 5 seconds before retrying
      delay(1000);
    }
  }
}
void setup_wifi() {
 
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  clientIP = String(ipToStr(WiFi.localIP()));
}
bool newCardAvailable(){
 switch(mfrc522Help){
      case 0: if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
                mfrc522Help++;
                return true;
              }else{
                return false;
              }
              break;
      case 1: if (!mfrc522.PICC_IsNewCardPresent() && !mfrc522.PICC_ReadCardSerial()) {
                mfrc522Help++;
              }
              return false; 
              break;
      case 2: if (!mfrc522.PICC_IsNewCardPresent() && !mfrc522.PICC_ReadCardSerial()) {
                mfrc522Help=0;
              }else{
                mfrc522Help=1;
              }
              return false;
              break;
  }
  delay(100);
}
String macToStr(const uint8_t* mac){
  String result;
  for (int i = 0; i < 6; ++i) {
  result += String(mac[i], 16);
  if (i < 5)
  result += ':';
  }
  return result;
}
String ipToStr(IPAddress ip){
  String result;
  for (int i = 0; i < 4; i++) {
    if(i<3){
       result += String(ip[i]) + ".";
    }else{
       result += String(ip[i]);
    }
  }
  return result;
}