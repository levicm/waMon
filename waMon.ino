#include "Counter.h"
#include "Credentials.h"

// Contabiliza o tempo de funcionamento
#include <Uptime.h>
Uptime uptime;

// Rede wifi
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
const char* ssid = W_SSID;
const char* key = W_KEY;
ESP8266WiFiMulti WiFiMulti;

// OTA sincrono
#include <ESP8266WebServer.h>
#include <ElegantOTA.h>
ESP8266WebServer webServer(80);
// OTA assincrono
//#include <ESPAsyncWebServer.h>
//#include <AsyncElegantOTA.h>
//AsyncWebServer webServer(80);

// MQTT
#include "MqttHandler.h"
MqttHandler mqttHandler;
Counter mqttCounter(&mqttHandler);

// ThingSpeak
#include "ThingspeakHandler.h"
ThingspeakHandler tsHandler;
Counter tsCounter(&tsHandler);

// constants
#define SENSOR_PIN D1

// global variables
int pinState = 0;
long debouncing_time = 200; //Debouncing Time in Milliseconds
volatile unsigned long last_millis;

ICACHE_RAM_ATTR void pulseCount() {
  if ((millis() - last_millis) > debouncing_time) {
    Serial.println("** Pulsecount **"); 
    last_millis = millis();
    mqttCounter.count();
    tsCounter.count();
  }
}

void setupWifi() {
  Serial.print("Connecting WiFi ");
  Serial.print(ssid);

  WiFiMulti.addAP(ssid, key);
  WiFi.mode(WIFI_STA);
  while(WiFiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void setupWebServer() {
  // ElegantOTA     
  webServer.on("/", []() {
    String page = "Hi! I am a Water Monitor.\r\n";
    //Check if GET parameter exists
    if(webServer.hasArg("total")) {
      String p = webServer.arg("total");
      Serial.println("tem param total. size: " + p);
      long total = p.toInt();
      if (total > 0) {
        Serial.println("total obtido");
        mqttCounter.setTotal(total);
        tsCounter.setTotal(total);
        page += "\r\nTotal updated to " + String(total);
      } else {
        page += "\r\nInvalid parameter";
      }
    } else {
      page += "\r\nMQTT:\r\n" + mqttCounter.toString() + "\r\n";
      page += "\r\nThingSpeak:\r\n" + tsCounter.toString() + "\r\n";
      page += "\r\nUptime: "; page += uptime.days(); page += "d"; page += uptime.hours(); page += "h"; page += uptime.minutes(); page += "m"; page += uptime.seconds(); page += "s\r\n";
    }
    webServer.send(200, "text/plain", page);
  });
  ElegantOTA.begin(&webServer);
/*  // AsyncElegantOTA
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String page = "Hi! I am a Water Monitor.\r\n";

    //Check if GET parameter exists
    if(request->hasParam("total")) {
      AsyncWebParameter* p = request->getParam("total");
      Serial.println("tem param total. size: " + p->value());
      long total = p->value().toInt();
      if (total > 0) {
        Serial.println("total obtido");
        mqttCounter.setTotal(total);
        tsCounter.setTotal(total);
        page += "\r\nTotal updated to " + String(total);
      } else {
        page += "\r\nInvalid parameter";
      }
    } else {
      page += "\r\nMQTT:\r\n" + mqttCounter.toString() + "\r\n";
      page += "\r\nThingSpeak:\r\n" + tsCounter.toString() + "\r\n";
      page += "\r\nUptime: "; page += uptime.days(); page += "d"; page += uptime.hours(); page += "h"; page += uptime.minutes(); page += "m"; page += uptime.seconds(); page += "s\r\n";
    }
    request->send(200, "text/plain", page);
  });
  AsyncElegantOTA.begin(&webServer);
*/
  webServer.begin();
  Serial.println("HTTP server started");
}

void readTotalFromThingSpeak() {
  long total = tsHandler.readTotal();  
  if (total > -1) {
    mqttCounter.setTotal(total);
    tsCounter.setTotal(total);
  }
}

void pulseLed() {
  int value = digitalRead(SENSOR_PIN);
  if (value != pinState) {
    pinState = value;
    if (pinState == 0) {
      digitalWrite(LED_BUILTIN, HIGH);  
      delay(250);
      digitalWrite(LED_BUILTIN, LOW);  
    }
  }  
}

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200); 
  Serial.println("** waMon - Water Monitor **"); 

  setupWifi();
  setupWebServer();

  mqttCounter.setup();
  tsCounter.setup();

  readTotalFromThingSpeak();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  digitalWrite(LED_BUILTIN, LOW);  
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), pulseCount, FALLING);
}

// the loop function runs over and over again forever
void loop() {
  webServer.handleClient(); // ElegantOTA
//  AsyncElegantOTA.loop(); // AsyncElegantOTA
  pulseLed();

  mqttCounter.loop();
  tsCounter.loop();
}
