#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <EEPROM.h> 
#include <WiFiEspUdp.h>
#include <PubSubClient.h>
#include "GravityTDS.h" //Library TDS Sensor

#define TdsSensorPin A0 //menetapkan Pin TDS1 A0
GravityTDS gravityTds;
float tdsValue = 0; //output tds sensor

const char* server = "broker.hivemq.com";
char ssid[] = "indihome";           // your network SSID (name)
char pass[] = "";           // your network password
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

   //pub mqtt
   client.publish("client_id/hidro/sen1", (char*)str_tdsValue.c_str());

//relay config
if (tdsValue >=600  && tdsValue <=900 ){
 digitalWrite(relayNutA, relayOFF);
 client.publish("client_id/hidro/reA", "OFF");
 Serial.println(" Nutrisi A OFF");
 
 digitalWrite(relayNutB, relayOFF);
 client.publish("client_id/hidro/reB", "OFF");
 Serial.println(" Nutrisi B OFF");
 
 client.publish("client_id/hidro/peng", "OFF");
 digitalWrite(relayPengaduk, relayOFF);
 Serial.println(" Pengaduk OFF");
 
 digitalWrite(relayAir, relayOFF);
 client.publish("client_id/hidro/reAir", "OFF");
 Serial.println("Pompa Air OFF");
 Serial.println("");
 }
 
else if (tdsValue >= 0 && tdsValue <=759  ){
 digitalWrite(relayNutA, relayON);
  client.publish("client_id/hidro/reA", "ON");
 Serial.println("Relay Nutrisi A ON");
 
  
 digitalWrite(relayNutB, relayON);
 client.publish("client_id/hidro/reB", "ON");
 Serial.println("Relay Nutrisi B ON");
 
 digitalWrite(relayPengaduk, relayON);
 client.publish("client_id/hidro/peng", "ON");
 Serial.println("Relay Pengaduk ON");
 
 digitalWrite(relayAir, relayOFF);
 client.publish("client_id/hidro/reAir", "OFF");
 Serial.println("Pompa Air OFF");
 Serial.println("");
}

else if (tdsValue >=901 ){
 digitalWrite(relayNutA, relayOFF);
 client.publish("client_id/hidro/reA", "OFF");
 Serial.println("Relay Nutrisi A OFF");
 
 digitalWrite(relayNutB, relayOFF);
 client.publish("client_id/hidro/reB", "OFF");
 Serial.println("Relay Nutrisi B OFF");
 
 digitalWrite(relayPengaduk, relayOFF);
 client.publish("client_id/hidro/peng", "OFF");
 Serial.println("Pengaduk OFF");
 digitalWrite(relayAir, relayON);
 client.publish("client_id/hidro/reAir", "ON");
 Serial.println("Pompa Air ON");
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
      loop();
      // ... and resubscribe
      client.subscribe("esp/hidro");
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
