#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <EEPROM.h> 
#include <WiFiEspUdp.h>
#include <PubSubClient.h>
#include "GravityTDS.h" //Library TDS Sensor

#define TdsSensorPin A0 //menetapkan Pin TDS1 A0
#define TdsSensor2Pin A3 //menetapkan Pin TDS1 A0
GravityTDS gravityTds;
float tdsValue = 0; //output tds sensor

const char* server = "broker.hivemq.com";
char ssid[] = "12345";           // your network SSID (name)
char pass[] = "12345678";           // your network password
int status = WL_IDLE_STATUS;   // the Wifi radio's status

// Initialize the Ethernet client object
WiFiEspClient espClient;
PubSubClient client(espClient);
const int relayAir = 26; //relay u/ pompa air
const int relayNutA = 32; //relay untuk nutrisi A
const int relayNutB = 36; //Relay untuk nutrisi B
const int relayPengaduk = 40; //relay untuk pengaduk
int relayON = LOW; //relay nyala
int relayOFF = HIGH; //relay mati


void setup() {
  // initialize serial for debugging
  Serial.begin(9600);
  // initialize serial for ESP module
  Serial1.begin(9600);
  // initialize ESP module
  WiFi.init(&Serial1);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  // you're connected now, so print out the data
  Serial.println("You're connected to the network");
  Serial.println(ssid);
  printWifiStatus();
  
  //connect to MQTT server
  client.setServer(server, 1883);
  client.setCallback(callback);

  //sensor tds
  gravityTds.setPin(TdsSensorPin);
  gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
  gravityTds.begin();  //initialization
  //relay
  pinMode(relayAir, OUTPUT);
  pinMode(relayNutA, OUTPUT);
  pinMode(relayNutB, OUTPUT);
  pinMode(relayPengaduk, OUTPUT);
  digitalWrite(relayAir, relayOFF);
  digitalWrite(relayNutA, relayOFF);
  digitalWrite(relayNutB, relayOFF);
  digitalWrite(relayPengaduk, relayOFF);
}

//print any message received for subscribed topic
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  //sensor tds
   gravityTds.update();  //sample and calculate
   tdsValue = gravityTds.getTdsValue();  // then get the value
   String str_tdsValue = String(tdsValue);
   Serial.print(tdsValue,0.0);
   Serial.println(" PPM");
   delay(5000);
   //pub mqtt
   client.publish("hidroponik/sensor1", (char*)str_tdsValue.c_str());
   Serial.println("publish");
//relay config
if (tdsValue >= 50 && tdsValue <=100 ){
 digitalWrite(relayNutA, relayOFF);
 client.publish("hidroponik/relayA", "relay OFF");
 Serial.println(" Nutrisi A OFF");
 
 digitalWrite(relayNutB, relayOFF);
 client.publish("hidroponik/relayB", "relay OFF");
 Serial.println(" Nutrisi B OFF");
 
 client.publish("hidroponik/pengaduk", "relay OFF");
 digitalWrite(relayPengaduk, relayOFF);
 Serial.println(" Pengaduk OFF");
 
 digitalWrite(relayAir, relayOFF);
 client.publish("hidroponik/relayair", "relay OFF");
 Serial.println("Pompa Air OFF");
 Serial.println("");
 }
 
else if (tdsValue >= 0 && tdsValue < 50  ){
 digitalWrite(relayNutA, relayON);
  client.publish("hidroponik/relayA", "relay ON");
 Serial.println("Relay Nutrisi A ON");
 
  
 digitalWrite(relayNutB, relayON);
 client.publish("hidroponik/relayB", "relay ON");
 Serial.println("Relay Nutrisi B ON");
 
 digitalWrite(relayPengaduk, relayON);
 client.publish("hidroponik/pengaduk", "relay ON");
 Serial.println("Relay Pengaduk ON");
 
 digitalWrite(relayAir, relayOFF);
 client.publish("hidroponik/relayair", "relay OFF");
 Serial.println("Pompa Air OFF");
 Serial.println("");
}

else if (tdsValue >=120 && tdsValue <=1500 ){
 digitalWrite(relayNutA, relayOFF);
 client.publish("hidroponik/relayA", "relay OFF");
 Serial.println("Relay Nutrisi A OFF");
 
 digitalWrite(relayNutB, relayOFF);
 client.publish("hidroponik/relayB", "relay OFF");
 Serial.println("Relay Nutrisi B OFF");
 
 digitalWrite(relayPengaduk, relayOFF);
 client.publish("hidroponik/pengaduk", "relay OFF");
 Serial.println("Pengaduk OFF");
 digitalWrite(relayAir, relayON);
 client.publish("hidroponik/relayair", "relay ON");
 Serial.println("Pompa Air ON");
 Serial.println("");
}

else { 
 digitalWrite(relayNutA, relayOFF);
  Serial.println(" Nutrisi A OFF");
 digitalWrite(relayNutB, relayOFF);
 Serial.println(" Nutrisi B OFF");
 digitalWrite(relayPengaduk, relayOFF);
 Serial.println(" Pengaduk OFF");
 digitalWrite(relayAir, relayOFF);
 Serial.println("Pompa Air OFF");
 Serial.println("");
}
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect, just a name to identify the client
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      String str_tdsValue = String(tdsValue);
      client.publish("esp8266/hidroponikifa", (char*)str_tdsValue.c_str());
      // ... and resubscribe
      client.subscribe("AR26");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
//cetak wifi ke serial monitor
void printWifiStatus() {
  Serial.print("Dengan SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("Dengan Alamat IP: ");
  Serial.println(ip);
  Serial.println("");
}
