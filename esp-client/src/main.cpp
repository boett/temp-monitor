//nodeMCU v1.0 (black) with Arduino IDE
//stream temperature data DS18B20 with 1wire on ESP8266 ESP12-E (nodeMCU v1.0)
//shin-ajaran.blogspot.com
//nodemcu pinout https://github.com/esp8266/Arduino/issues/584
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>

void connectWifi();
void sendTemperatures(float *temp);
void sayHello();

//Def
#define myPeriodic 180 //in sec | Thingspeak pub is 15sec
#define ONE_WIRE_BUS 2  // DS18B20 on arduino pin2 corresponds to D4 on physical board
//#define ONE_WIRE_BUS 15  // DS18B20 on arduino pin2 corresponds to D4 on physical board

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

struct nw {
  const char *ssid;
  const char *pwd;
};

/*
In networks.h, put these definitions.  And don't check it into source control.

const char* server = "something.somewhere.com";
nw networks[] = { {"ssid1", "pw1"},
		  {"ssid2", "pw2"},
		  {NULL, NULL}};
*/
#include "networks.h"

DeviceAddress addr[8];
uint8_t nSensors=0;

String genAddressString(const DeviceAddress &addr)
{
  char str[18];
  String ret;
  for(int i=0; i<8; ++i) {
    //ret += String(addr[i], HEX);
    sprintf(str + 2*i, "%.2x", addr[i]);
  }
  return String(str);
}

int sent = 0;
void setup() {
  Serial.begin(115200);
  connectWifi();

  DS18B20.begin();
  nSensors = DS18B20.getDS18Count();

  Serial.println(String("found ") + String(nSensors) + " DS18B20 sensors:");
  for(uint8_t i=0; i<nSensors; i++) {
    bool ret = DS18B20.getAddress(addr[i], i);
    if(ret)
      Serial.println(String("addr: ") + genAddressString(addr[i]));
    else
      Serial.println("failure");
  }

  sayHello();
}

void loop() {
  float temps[8];

  DS18B20.requestTemperatures();
  for(int i=0; i<nSensors; ++i) {
    temps[i] = DS18B20.getTempF(addr[i]);
    Serial.println(String("Sensor ") + genAddressString(addr[i]) + ": " + String(temps[i]));
  }

  //Serial.printf(" ESP8266 Chip id = %08X\n", ESP.getChipId());
  
  sendTemperatures(temps);
  int count = myPeriodic;
  while(count--)
  delay(1000);
}

void connectWifi()
{
  int n = WiFi.scanNetworks();
  nw network = {NULL, NULL};
  for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");

      for (int j = 0; networks[j].ssid != NULL; ++j) {
	if(String(networks[j].ssid) == WiFi.SSID(i)) {
	  network = networks[j];
	}
      }
  }

  if(network.ssid != NULL) {
    Serial.print(String("Connecting to ") + network.ssid);
    WiFi.begin(network.ssid, network.pwd);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
    }
  }
  
  Serial.println("");
  Serial.println("Connected");
  Serial.println("");  
}//end connect

void sendTemperatures(float *temp)
{
  String postStr = "{ \"temperatures\": [ ";
  for(int i=0; i<nSensors; i++) {
    postStr += "{\"sn\": \"" + genAddressString(addr[i]) + "\", \"temp\": " + String(temp[i]) + "}";
    if(i<nSensors-1)
      postStr += ",";
  }
  postStr += "] }";

  Serial.println(postStr);
    
   WiFiClient client;
  
   if (client.connect(server, 80)) { // use ip 184.106.153.149 or api.thingspeak.com
   Serial.println("WiFi Client connected ");

   client.print("POST /post_temps HTTP/1.1\n");
   client.print(String("Host: ") + server + "\n");
   client.print("Connection: close\n");
   client.print("Content-Type: application/json\n");
   client.print("Content-Length: ");
   client.print(postStr.length());
   client.print("\n\n");
   client.print(postStr);
   delay(1000);
   
   }//end if
   sent++;
 client.stop();
}//end send

void sayHello()
{
  char msg[32];
  sprintf(msg, "%08X", ESP.getChipId());

  String postStr = String("{ \"chip\": \"") + msg + "\", \"sensors\": [ ";
  for(int i=0; i<nSensors; i++) {
    postStr += "\"" + genAddressString(addr[i]) + "\"";
    if(i<nSensors-1)
      postStr += ", ";
  }
  postStr += "] }";

  Serial.println(postStr);
    
   WiFiClient client;
  
   if (client.connect(server, 80)) { // use ip 184.106.153.149 or api.thingspeak.com
   Serial.println("WiFi Client connected ");

   client.print("POST /hello HTTP/1.1\n");
   client.print(String("Host: ") + server + "\n");
   client.print("Connection: close\n");
   client.print("Content-Type: application/json\n");
   client.print("Content-Length: ");
   client.print(postStr.length());
   client.print("\n\n");
   client.print(postStr);
   delay(1000);
   
   }//end if

 client.stop();
}//end send
