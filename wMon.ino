// Contabiliza o tempo de funcionamento
#include <Uptime.h>

// Rede wifi
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#define W_SSID1 "your-wifi-ssid" // Rede de casa
#define W_KEY1  "your-wifi-key"

// OTA
#include <ArduinoOTA.h>

// MQTT
#include <PubSubClient.h>
#include <ArduinoJson.h>
#define mqtt_server "hassio.local"
#define mqtt_port 1883
#define mqtt_user "mqtt-user" 
#define mqtt_password "mqtt-pass"
#define water_topic "home/sensor/water"
IPAddress mqttServer(192, 168, 1, 20);

// ThingSpeak
#define PRINT_DEBUG_MESSAGES
#include "ThingSpeak.h"
unsigned long myChannelNumber = 100;
const char * myWriteAPIKey = "5W...";
const char * myReadAPIKey = "9Y...";

// Constantes
#define SENSORNAME "wMon"
#define SENSOR_PIN D1
const unsigned long T_SECOND    = 1000;
const unsigned long T_MINUTE    = T_SECOND * 60;
const unsigned long T_HOUR      = T_MINUTE * 60;
const unsigned long T_DAY       = T_HOUR * 24;
const unsigned long T_WEEK      = T_DAY * 7;
const unsigned long T_MONTH     = T_DAY * 30;
const byte ID_TOTAL = 1;
const byte ID_LAST_MINUTE = 2;
const byte ID_LAST_HOUR = 3;
const byte ID_LAST_DAY = 4;
const byte ID_LAST_WEEK = 5;
const byte ID_LAST_MONTH = 6;
const char * NAME_TOTAL = "total";
const char * NAME_LAST_MINUTE = "last_minute";
const char * NAME_LAST_HOUR = "last_hour";
const char * NAME_LAST_DAY = "last_day";
const char * NAME_LAST_WEEK = "last_week";
const char * NAME_LAST_MONTH = "last_month";

// Variaveis globais
Uptime uptime;
ESP8266WiFiMulti WiFiMulti;
WiFiClient  wifiClientThing;
WiFiClient  wifiClientMqtt;
PubSubClient mqttClient(mqttServer, mqtt_port, wifiClientMqtt);

int pinState = 0;

// Momentos
unsigned long nextMinute = 0;
unsigned long nextHour = 0;
unsigned long nextDay = 0;
unsigned long nextWeek = 0;
unsigned long nextMonth = 0;

// Acumuladores
long liters = 2084; // Ultimo valor 
long litersLastMinute = 0;
long litersLastHour = 0;
long litersLastDay = 0;
long litersLastWeek = 0;
long litersLastMonth = 0;
long measuresLastDay = 0;

long now = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200); 
  Serial.println("** wMon - Water Monitor **"); 

  setupWifi();
  setupOTA();
  connectMqtt();
  ThingSpeak.begin(wifiClientThing); 
  readTotalFromThingSpeak();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), pulseCount, FALLING);

  initTimers();
}

// the loop function runs over and over again forever
void loop() {
  ArduinoOTA.handle();
  pulseLed();
  handleValues();
  // provavelmente serve apenas para receber mensagens
  mqttClient.loop(); 
}

void pulseCount() {
  liters++;
  litersLastMinute++;
  litersLastHour++;
  litersLastDay++;
  litersLastWeek++;
  litersLastMonth++;
}

void setupWifi() {
  WiFiMulti.addAP(W_SSID1, W_KEY1);

  Serial.println();
  Serial.println();
  Serial.print("Connecting WiFi ");
  Serial.print(W_SSID1);

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

void setupOTA() {
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("OTA Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("OTA Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("OTA Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("OTA Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("OTA End Failed");
  });
  ArduinoOTA.begin();  
}

void connectMqtt() {
  Serial.print("Attempting MQTT connection...");
  if (mqttClient.connect(SENSORNAME, mqtt_user, mqtt_password)) {
    Serial.println("connected");
  } else {
    Serial.print("failed, rc=");
    Serial.println(mqttClient.state());
  }
}

void readTotalFromThingSpeak() {
  Serial.println("Getting total from ThingSpeak...");
  long field1 = ThingSpeak.readLongField(myChannelNumber, ID_TOTAL, myReadAPIKey);  
  int statusCode = ThingSpeak.getLastReadStatus();
  if(statusCode == 200) {
    liters = field1;
    Serial.println("total: " + String(liters));
  } else {
    Serial.println("Problem reading channel. HTTP error code " + String(statusCode)); 
  }
}

void initTimers() {
  long now = millis();
  nextMinute = now + T_MINUTE;
  nextHour = now + T_HOUR;
  nextDay = now + T_DAY;
  nextWeek = now + T_WEEK;
  nextMonth = now + T_MONTH;
}

void pulseLed() {
  int value = digitalRead(SENSOR_PIN);
  if (value != pinState) {
    pinState = value;
    Serial.println(value); 
    digitalWrite(LED_BUILTIN, value);  
    if (pinState == 0) {
      String message = "";
      message = message + "Liters: " + liters + ": " + micros();
      Serial.println(message); 
    }
  }  
}

void handleValues() {
  long value = 0;
  now = millis();

  // Envia a cada minuto os litros do ultimo minuto
  if (now > nextMinute) {
    nextMinute = now + T_MINUTE;
    value = litersLastMinute;
    litersLastMinute = 0;
    measuresLastDay++;
    Serial.print("MINUTE: "); 
    sendValue(ID_LAST_MINUTE, NAME_LAST_MINUTE, value);
  }
  // Envia a cada hora os litros da ultima hora
  if (now > nextHour) {
    nextHour = now + T_HOUR;
    value = litersLastHour;
    litersLastHour = 0;
    Serial.print("HOUR: "); 
    sendValue(ID_LAST_HOUR, NAME_LAST_HOUR, value);
  }
  // Envia a cada dia os litros do ultimo dia
  if (now > nextDay) {
    nextDay = now + T_DAY;
    value = litersLastDay;
    litersLastDay = 0;
    Serial.print("DAY: "); 
    sendValue(ID_LAST_DAY, NAME_LAST_DAY, value);
    // Todo dia, envia tambem a qtd de medidadas (por minuto) do dia (esperado 1440)
    sendValue(7, "measures_last_day", measuresLastDay);
    measuresLastDay = 0;
  }
  if (now > nextWeek) {
    nextWeek = now + T_WEEK;
    value = litersLastWeek;
    litersLastWeek = 0;
    Serial.print("WEEK: "); 
    sendValue(ID_LAST_WEEK, NAME_LAST_WEEK, value);
  }
  if (now > nextMonth) {
    nextMonth = now + T_MONTH;
    value = litersLastMonth;
    litersLastMonth = 0;
    Serial.print("MONTH: "); 
    sendValue(ID_LAST_MONTH, NAME_LAST_MONTH, value);
  }  
}

void sendValue(int id, String name, unsigned long value) {
  Serial.println(now); 
  showValues();
  sendValueToMqtt(name, value);
  sendValueToThingSpeak(id, value);
}

void sendValueToMqtt(String name, unsigned long value) {
  if (!mqttClient.connected()) {
    connectMqtt();
  }
  const int capacity = JSON_OBJECT_SIZE(7);
  StaticJsonDocument<capacity> doc;
  doc["unit"] = "l";
  // Em todo envio de campo, manda tambem o total
  doc["total"] = liters;
  doc[name] = value;
  // Em todo envio de campo, manda tambem o tempo de funcionamento
  uptime.compute();
  doc["upt_days"] = uptime.days();
  doc["upt_hours"] = uptime.hours();
  doc["upt_minutes"] = uptime.minutes();
  doc["upt_total_seconds"] = uptime.totalSeconds();
  int len = measureJson(doc) + 1;
  char buffer[len];
  serializeJson(doc, buffer, len);

  Serial.println(buffer);
  mqttClient.publish(water_topic, buffer, true);
}

void sendValueToThingSpeak(int id, long value) {
  // Em todo envio, manda o campo de id 'id' e o total
  ThingSpeak.setField(1, liters);
  ThingSpeak.setField(id, value);
  int code = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (code == 200) {
    Serial.println("Data sent to ThingSpeak");
  } else {
    Serial.print("Error sending data to ThingSpeak: ");
    Serial.println(code);
  }
}

void sendValues() {
  Serial.println("Sending data...");
  ThingSpeak.setField(1, liters);
  ThingSpeak.setField(2, litersLastMinute);
  ThingSpeak.setField(3, litersLastHour);
  ThingSpeak.setField(4, litersLastDay);
  ThingSpeak.setField(5, litersLastWeek);
  ThingSpeak.setField(6, litersLastMonth);
  int code = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (code == 200) {
    Serial.println("Data sent");
  } else {
    Serial.print("Error sending data: ");
    Serial.println(code);
  }
}

void showValues() {
  Serial.print("Liters: ");
  Serial.println(liters);
  Serial.print("Liters last minute: ");
  Serial.println(litersLastMinute);
  Serial.print("Liters last hour: ");
  Serial.println(litersLastHour);
  Serial.print("Liters last day: ");
  Serial.println(litersLastDay);
  Serial.print("Liters last week: ");
  Serial.println(litersLastWeek);
  Serial.print("Liters last month: ");
  Serial.println(litersLastMonth);
}

