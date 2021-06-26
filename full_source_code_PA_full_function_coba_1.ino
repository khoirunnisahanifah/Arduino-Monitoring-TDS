//library
#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <EEPROM.h> 
#include <WiFiEspUdp.h>
#include <PubSubClient.h>
#include "GravityTDS.h" //Library TDS Sensor

//define kabel keypad 1-5 ke 1 GND, ke2 pad2,ke3 pad1, ke4 pad5, ke5 pad4 input) 
#define key1 4 //connect wire 1 to pin 5
#define key2 5 //connect wire 2 to pin 4
#define key3 6 //connect wire 3 to pin 7
#define key4 7 //connect wire 4 to pin 6

//define sensor TDS
#define TdsSensorPin A0 //menetapkan Pin TDS1 A0
GravityTDS gravityTds;
float tdsValue = 0; //output tds sensor

//inisialisasi konek mqtt
const char* server = "broker.emqx.io";
const char* port_mqtt = 1883;

//inisialisasi konek wifi
char ssid[] = "xx";           // your network SSID (name)
char pass[] = "xx";           // your network password
int status = WL_IDLE_STATUS;   // the Wifi radio's status

// inisialisasi the Ethernet client object
WiFiEspClient espClient;
PubSubClient client(espClient);

//inisialisasi relay
const int relayAir = 22; //relay u/ pompa air
const int relayNutA  = 24; //relay untuk nutrisi A
const int relayNutB = 26; //Relay untuk nutrisi B
const int relayPengaduk = 28; //relay untuk pengaduk
int relayON = LOW; //relay nyala
int relayOFF = HIGH; //relay mati


void setup() {
  initSerialm(); //initialize serial for debugging
  initWiFi(); // initialize serial for ESP module
  printWifiStatus(); //wifi status
  konekmqttset();//connect to MQTT server
  beginsensortds();//sensor tds
  pinrelayy();//relay pinmode
  dwrelayy(); //digiwrite relayy
  pmkeypad();//pinmode keypad
} //end void setup

void loop() {
  if (!client.connected()) {
    reconnectMqtt();
  }//end if reconnect
  
  client.loop();

  //keypad config
  int key1S = digitalRead(key1);// read if key1 is pressed
  int key2S = digitalRead(key2);// read if key1 is pressed
  int key3S = digitalRead(key3);// read if key1 is pressed
  int key4S = digitalRead(key4);// read if key1 is pressed 
 
   if(!key1S){
    int x=1; 
    Serial.println("key 1 is 600-800");  
    while (x==1){
       client.publish("client_id/hidro/mingg", "600-800 PPM");
       tdsprosess();
       if (tdsValue >=600  && tdsValue <=800 ){
         relaysemuamati(); //semua relay dalam keadaan OFF
         }//end if 600-800
         
         else if (tdsValue >= 0 && tdsValue <=599  ){
           relayairmati(); // relay air mati dan nutrisiA,B pengaduk menyala
          }//0-599
  
          else if (tdsValue >=801 ){
           relayairnyala(); //relay air nyala dan nutrisi A, B pengaduk mati
          } //801
           
          if (!client.connected()) {
           reconnectMqtt();
          } //if reconnect
          
          int key4S = digitalRead(key4);// read if key1 is pressed     
           if(!key4S){
           x=4; 
           keypadstop();
         }//if key4
     }//while
   } //if key
     
     if(!key2S){
     int x=1;   
     Serial.println("key 2 is 800-1200 PPM ");   
     while (x==1){
     client.publish("client_id/hidro/mingg", "800-1200 PPM");
     tdsprosess();
      if (tdsValue >=800  && tdsValue <=1200 ){
         relaysemuamati(); //semua relay dalam keadaan OFF
         }//800-1200
         
         else if (tdsValue >= 0 && tdsValue <=799  ){
           relayairmati();
          }//0-799
  
          else if (tdsValue >=1201 ){
           relayairnyala();
          }//1201
          
          if (!client.connected()) {
           reconnectMqtt();
          }//if reconnect
          
         int key4S = digitalRead(key4);// read if key1 is pressed     
           if(!key4S){
           x=4; 
           keypadstop();
         }//if key4
         
      }//while
     }//if 

     
     if(!key3S){
      int x=1;
      Serial.println("key 3 is pressed 1200-1400 ");
      while (x==1){
      client.publish("client_id/hidro/mingg", "1200-1400 PPM");
      tdsprosess();
        if (tdsValue >=1200  && tdsValue <=1400 ){
           relaysemuamati();
           }
         
         else if (tdsValue >= 0 && tdsValue <=1199  ){
           relayairmati();
          }
  
          else if (tdsValue >=1401 ){
           relayairnyala();
          }
          if (!client.connected()) {
          reconnectMqtt();}
          
          int key4S = digitalRead(key4);// read if key1 is pressed     
           if(!key4S){
           x=4; 
           keypadstop();
         }//if key4
         
       }//while
   }//ifkey3
     delay(100);
}//void loop

void reconnectMqtt() {
  // Loop until we're reconnected
  if ( status == WL_CONNECTED) {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect, just a name to identify the client
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      String str_tdsValue = String(tdsValue);
      client.publish("client_id/hidro/sen1", (char*)str_tdsValue.c_str());
      
      // ... and resubscribe
      client.subscribe("esp8266/hidro");
    } //if client connect
    
      else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
        }//else 
        
    }//while client
  }//if status
}//void reconnect

//cetak wifi ke serial monitor
void initSerialm(){
  Serial.begin(9600);
}
void initWiFi(){
Serial1.begin(9600);
  // initialize ESP module
  WiFi.init(&Serial1);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }//ifwifistatus

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }//while status

  // you're connected now, so print out the data
  Serial.println("You're connected to the network");
  Serial.println(ssid);
  }

void printWifiStatus() {
  Serial.print("Dengan SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("Dengan Alamat IP: ");
  Serial.println(ip);
  Serial.println("");
}

void konekmqttset(){
  client.setServer(server, port_mqtt);
  client.setCallback(callback);
}

void beginsensortds(){
  gravityTds.setPin(TdsSensorPin);
  gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
  gravityTds.begin();  //initialization
}

void pinrelayy(){
  pinMode(relayAir, OUTPUT);
  pinMode(relayNutA, OUTPUT);
  pinMode(relayNutB, OUTPUT);
  pinMode(relayPengaduk, OUTPUT);
}

void dwrelayy(){
  digitalWrite(relayAir, relayOFF);
  digitalWrite(relayNutA, relayOFF);
  digitalWrite(relayNutB, relayOFF);
  digitalWrite(relayPengaduk, relayOFF);
}

void pmkeypad(){
  pinMode(key1, INPUT_PULLUP);// set pin as input
  pinMode(key2, INPUT_PULLUP);// set pin as input
  pinMode(key3, INPUT_PULLUP);// set pin as input
  pinMode(key4, INPUT_PULLUP);// set pin as input
}

//print any message received for subscribed topic
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }//for
  Serial.println();
}//void callback

void tdsprosess(){
  gravityTds.update();  //sample and calculate
  tdsValue = gravityTds.getTdsValue();  // then get the value
  String str_tdsValue = String(tdsValue);
  Serial.print(tdsValue,0.0);
  Serial.println(" PPM");
  //pub mqtt
  client.publish("client_id/hidro/sen1", (char*)str_tdsValue.c_str());
}

void relaysemuamati(){
  digitalWrite(relayNutA, relayOFF);
  digitalWrite(relayNutB, relayOFF);
  digitalWrite(relayAir, relayOFF);
  digitalWrite(relayPengaduk, relayOFF);
  client.publish("client_id/hidro/reA", "OFF");
  client.publish("client_id/hidro/reB", "OFF");
  client.publish("client_id/hidro/peng", "OFF");
  client.publish("client_id/hidro/reAir", "OFF");
  Serial.println(" Nutrisi A OFF");
  Serial.println(" Nutrisi B OFF");
  Serial.println(" Pengaduk OFF");
  Serial.println("Pompa Air OFF");
  Serial.println("");
}

void relayairmati(){
  digitalWrite(relayNutA, relayON);
  digitalWrite(relayNutB, relayON);
  digitalWrite(relayPengaduk, relayON);
  digitalWrite(relayAir, relayOFF);
  client.publish("client_id/hidro/reA", "ON");
  client.publish("client_id/hidro/reB", "ON");
  client.publish("client_id/hidro/peng", "ON");
  client.publish("client_id/hidro/reAir", "OFF");
  Serial.println("Relay Nutrisi A ON");
  Serial.println("Relay Nutrisi B ON");
  Serial.println("Relay Pengaduk ON");
  Serial.println("Pompa Air OFF");
  Serial.println("");
}

void relayairnyala(){
  digitalWrite(relayNutA, relayOFF);
  digitalWrite(relayNutB, relayOFF);
  digitalWrite(relayPengaduk, relayOFF);
  digitalWrite(relayAir, relayON);
  client.publish("client_id/hidro/reA", "OFF");
  client.publish("client_id/hidro/reB", "OFF");
  client.publish("client_id/hidro/peng", "OFF");
  client.publish("client_id/hidro/reAir", "ON");
  Serial.println("Relay Nutrisi A OFF"); 
  Serial.println("Relay Nutrisi B OFF");
  Serial.println("Pengaduk OFF");
  Serial.println("Pompa Air ON");
  Serial.println("");
}

void keypadstop(){
  Serial.println("Stop");
  relaysemuamati();
  client.publish("client_id/hidro/mingg", "Stop");
}
