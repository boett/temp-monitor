//nodeMCU v1.0 (black) with Arduino IDE
//stream temperature data DS18B20 with 1wire on ESP8266 ESP12-E (nodeMCU v1.0)
//shin-ajaran.blogspot.com
//nodemcu pinout https://github.com/esp8266/Arduino/issues/584
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
//#include <OneWire.h>
//#include <DallasTemperature.h>
#include "HX711.h"

const int MY_VERSION = 6;

const int LOADCELL_DOUT_PIN = 4;
const int LOADCELL_SCK_PIN = 0;

const int PIR_PIN = 5;

void connectWifi();
void sayHello();
String build_full_json();
void sendJSON(const String &json);

HX711 scale;

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


template <typename T>
String build_json(const String name, T val, const String &id = String(""))
{
    String idStr = id;

    if(id.length() == 0) {
        char msg[32];
        sprintf(msg, "%08X", ESP.getChipId());
        idStr = name + msg;
    }

    String obj = "{ \"type\": \"" + name + "\", \"val\": " + String(val) + ", \"id\": \"" + idStr + "\"}";
    return obj;
}

struct Averager {
    double cur_val;
    int cnt;
    void submit(double val) {
        cur_val += val;
        cnt++;
    }
    void reset() {
        cnt = 0;
        cur_val = 0;
    }
    double val() {
        return cur_val / static_cast<double>(cnt);
    }
};

struct Sensor {
    String name;
    String id;
    Averager avg;
    bool present;

    Sensor() : present(true) {}
    bool is_present() const { return present; }
    virtual double get_cur_reading() = 0;
    void DoMeasure() {
        avg.submit(get_cur_reading());
    }
    String GetJSONObj(void) {
        return build_json(name, avg.val(), id);
    }
};

struct Scale : public Sensor {
    double get_cur_reading() override {
        if (scale.is_ready()) {
            long x = scale.read_average(8);
            Serial.println(x);
            return(static_cast<double>(x));
        } else {
            Serial.println("HX711 not found.");
            present = false;
        }
        return(0.0);
    }
};

struct PIR : public Sensor {
    double get_cur_reading() override {
        Serial.println(digitalRead(PIR_PIN));
        return static_cast<double>(digitalRead(PIR_PIN));
    }
};

std::vector<Sensor *> sensors;
int cntReading = 0;

int sent = 0;
void setup() {
    Serial.begin(115200);
    connectWifi();

    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.power_up();

    pinMode(PIR_PIN, INPUT);

    Scale *s = new Scale();
    s->name = "strain";
    sensors.push_back(s);

    PIR *p = new PIR();
    p->name = "pir";
    sensors.push_back(p);

  //DS18B20.begin();
  //nSensors = DS18B20.getDS18Count();

  /*
  Serial.println(String("found ") + String(nSensors) + " DS18B20 sensors:");
  for(uint8_t i=0; i<nSensors; i++) {
    bool ret = DS18B20.getAddress(addr[i], i);
    if(ret)
      Serial.println(String("addr: ") + genAddressString(addr[i]));
    else
      Serial.println("failure");
  }
  */
  sayHello();
}

void doOTAupdate()
{
    WiFiClient client;
    if (client.connect(server, 80))
    {
        Serial.println("Connected to server for update");
        t_httpUpdate_return ret = ESPhttpUpdate.update(client, server, 80, "/static/sensor.bin", "optional current version string here");
        int err = ESPhttpUpdate.getLastError();
        String errStr = ESPhttpUpdate.getLastErrorString();
        Serial.println(errStr + " " + String(err));

        switch (ret)
        {
        case HTTP_UPDATE_FAILED:
            Serial.println("[update] Update failed.");
            break;
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("[update] Update no Update.");
            break;
        case HTTP_UPDATE_OK:
            Serial.println("[update] Update ok."); // may not called we reboot the ESP
            break;
        }
    }
}

int totalCount = 0;
void loop() {

    for(auto &s : sensors) {
        s->DoMeasure();
    }

    if(++cntReading >= 60) {
        cntReading = 0;

        sendJSON(build_full_json());

        for(auto &s : sensors) {
            s->avg.reset();
        }

        totalCount++;

        if(totalCount >= 60) {
            ESP.reset();
        }
    }

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

String build_full_json()
{
    String json = "{ \"measurements\": [";
    bool first = true;
    for(auto &s : sensors) {
        if(!s->is_present())
            continue;
            
        if(!first) {
            json += ", ";
        }
        first = false;
        json += s->GetJSONObj();
    }
    json += " ]}";
    return json;
}

void sendJSON(const String &json)
{
    WiFiClient client;
    if (client.connect(server, 80)) { // use ip 184.106.153.149 or api.thingspeak.com
        Serial.println("WiFi Client connected ");
        
        client.print("POST /post_measurements HTTP/1.1\n");
        client.print(String("Host: ") + server + "\n");
        client.print("Connection: close\n");
        client.print("Content-Type: application/json\n");
        client.print("Content-Length: ");
        client.print(json.length());
        client.print("\n\n");
        client.print(json);
        delay(1000);
        client.read();
   }//end if
   sent++;
   client.stop();
}//end send

void sayHello()
{
    Serial.println("Version " + String(MY_VERSION));

    char msg[32];
    sprintf(msg, "%08X", ESP.getChipId());

    String postStr = String("{ \"chip\": \"") + msg + "\" }";
    Serial.println(postStr);

    uint8_t buf[256];

    WiFiClient client;
    int version = 0;

    if (client.connect(server, 80))
    { // use ip 184.106.153.149 or api.thingspeak.com
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
        int ret = client.read(buf, 256);
        (void) ret;

        String servermessage(reinterpret_cast<char *>(&buf[0]));
        //Serial.println(String(ret) + " " + servermessage);
        //int headerend = servermessage.indexOf("\r\n\r\n");
        //Serial.println(servermessage.substring(headerend));

        int veroffset = servermessage.indexOf("fwversion\": ");
        String version_partial = servermessage.substring(veroffset + 12);
        int verend = version_partial.indexOf("\n");

        version = version_partial.substring(0, verend).toInt();
        //Serial.println(String("***") + String(version) + String("***"));

        Serial.println("Server has version " + String(version));
    } //end if

    client.stop();

    if(version > MY_VERSION) {
        Serial.println("Server has version " + String(version) + ".  Doing update.");
        doOTAupdate();
    }

}//end send
