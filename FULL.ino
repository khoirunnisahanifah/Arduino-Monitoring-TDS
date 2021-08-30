//library
#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <EEPROM.h> 
#include <WiFiEspUdp.h>
#include <PubSubClient.h>
#define SERIAL Serial
//ini yang longsc
//define kabel keypad 1-5 ke 1 GND, ke2 pad2,ke3 pad1, ke4 pad5, ke5 pad4 input) 
#define key1 4 //connect wire 1 to pin 5
#define key2 5 //connect wire 2 to pin 4
#define key3 6 //connect wire 3 to pin 7
#define key4 7 //connect wire 4 to pin 6

//define LED
const int ledmerah = 47 ; 
const int ledhijau = 51;

//define sensor TDS
#define TdsSensorPin A0 //menetapkan Pin TDS A0
#define VREF          3.3 // analog reference voltage(Volt) of the ADC
#define SCOUNT        5  // sum of sample point

int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0;
float averageVoltage = 0,tdsValue = 0,temperature = 25;
unsigned long int analogSampleTimepoint,printTimepoint;


//inisialisasi konek mqtt
const char* server = "broker.emqx.io";
const char* port_mqtt = 1883;

//inisialisasi konek wifi
char ssid[] = "indihome";           // your network SSID (name)
char pass[] = "SalahSatu";           // your network password
int status = WL_IDLE_STATUS;   // the Wifi radio's status

// inisialisasi the Ethernet client object
WiFiEspClient espClient;
PubSubClient client(espClient);

//inisialisasi relay
const int relayAir = 26; //relay u/ pompa air
const int relayNutA  = 24; //relay untuk nutrisi A
const int relayNutB =22 ; //Relay untuk nutrisi B
const int relayPengaduk = 28; //relay untuk pengaduk
int relayON = LOW; //relay nyala
int relayOFF = HIGH; //relay mati


void setup() {
  ledmerahnyala();
  initSerialm(); //initialize serial for debugging
  initWiFi(); // initialize serial for ESP module
  printWifiStatus(); //wifi status
  konekmqttset();//connect to MQTT server
  beginsensortds();
  pinrelayy();//relay pinmode
  dwrelayy(); //digiwrite relayy
  pmkeypad();//pinmode keypad
  pmled();//pinmode led
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
       client.publish("client_id/hidro/mingg", "Kedua");
       
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
     client.publish("client_id/hidro/mingg", "Ketiga");
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
      client.publish("client_id/hidro/mingg", "Keempat");
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

}//void loop

void reconnectMqtt() {
  relaysemuamati();
  // Loop until we're reconnected
  if ( status == WL_CONNECTED) {
  while (!client.connected()) {
    ledmerahnyala();
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect, just a name to identify the client
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
        ledhijaunyala();
      // Once connected, publish an announcement...
      //String str_tdsValue = String(tdsValue);
      //client.publish("client_id/hidro/sen1", (char*)str_tdsValue.c_str());
      
      // ... and resubscribe
      
    } //if client connect
    
      else {
        ledmerahnyala();
        relaysemuamati();
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
   ledmerahnyala();
}
void initWiFi(){
  ledmerahnyala();
Serial1.begin(9600);
  // initialize ESP module
  WiFi.init(&Serial1);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
       ledmerahnyala();
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }//ifwifistatus

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
     ledmerahnyala();
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
   ledhijaunyala();
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
  pinMode(TdsSensorPin,INPUT);
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
void pmled(){
  pinMode(ledmerah, OUTPUT);
  pinMode(ledhijau, OUTPUT);
}
void ledmerahnyala(){
  digitalWrite(ledmerah, HIGH);
  digitalWrite(ledhijau, LOW);
}

void ledhijaunyala(){
  digitalWrite(ledmerah, LOW);
  digitalWrite(ledhijau, HIGH);
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

  if(millis()-analogSampleTimepoint>40)     //read the analog value from the ADC
 {
   analogSampleTimepoint=millis();
   analogBuffer[analogBufferIndex]=analogRead(TdsSensorPin);    //read the analog value and store into the buffer
   analogBufferIndex++;
   if(analogBufferIndex==SCOUNT) analogBufferIndex=0;
 }   

 if(millis()-printTimepoint>1500)
 {
    printTimepoint=millis();
    for(copyIndex=0;copyIndex<SCOUNT;copyIndex++) analogBufferTemp[copyIndex]=analogBuffer[copyIndex];
    averageVoltage=getMedianNum(analogBufferTemp,SCOUNT)*(float)VREF/1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
    float compensationCoefficient=1.0+0.02*(temperature-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
    float compensationVolatge=averageVoltage/compensationCoefficient;  //temperature compensation
    tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge-255.86*compensationVolatge*compensationVolatge+857.39*compensationVolatge)*0.7; //convert voltage value to tds value
    String str_tdsValue = String(tdsValue);
    Serial.print("TDS Value:");
    Serial.print(tdsValue,0);
    Serial.println(" PPM");
    client.publish("client_id/hidro/sen1", (char*)str_tdsValue.c_str());
 }  
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
 
}

void relayairnyala(){
 digitalWrite(relayNutA, relayOFF);
 digitalWrite(relayNutB, relayOFF);
 digitalWrite(relayPengaduk, relayON);
 digitalWrite(relayAir, relayON);
  
  client.publish("client_id/hidro/reA", "OFF");
  client.publish("client_id/hidro/reB", "OFF");
  client.publish("client_id/hidro/peng", "ON");
  client.publish("client_id/hidro/reAir", "ON");
  
}

void keypadstop(){
  
  relaysemuamati();
  Serial.println("Stop");
  client.publish("client_id/hidro/mingg", "Stop");
}
int getMedianNum(int bArray[], int iFilterLen) 
{
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
  bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) 
  {
    for (i = 0; i < iFilterLen - j - 1; i++) 
    {
      if (bTab[i] > bTab[i + 1]) 
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if((iFilterLen&1)>0) bTemp = bTab[(iFilterLen - 1) / 2];
  else bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}
