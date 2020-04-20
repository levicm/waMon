#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Handler.h"
#include "Credentials.h"

const int mqttPort = 1883;
const char * sensorName   = "waMon";
const char * mqttTopic    = "home/sensor/water";
const char * mqttUser     = MQTT_USER;
const char * mqttPassword = MQTT_PASSWORD;

const char * NAME_TOTAL = "total";
const char * NAME_LAST_MINUTE = "in_minute";
const char * NAME_LAST_HOUR = "in_hour";
const char * NAME_LAST_DAY = "in_day";

IPAddress mqttServerIp(192, 168, 1, 20);
WiFiClient mqttWifiClient;

class MqttHandler : public Handler {
  private:
    PubSubClient _mqttClient;
  
  public:
    MqttHandler(): _mqttClient(mqttServerIp, mqttPort, mqttWifiClient) {
    }

    void setup() {
      connectMqtt();
    }

    void loop() {
      // provavelmente serve apenas para receber mensagens
      _mqttClient.loop(); 
    }

    void connectMqtt() {
      Serial.print("Attempting MQTT connection...");
      if (_mqttClient.connect(sensorName, mqttUser, mqttPassword)) {
        Serial.println("connected");
      } else {
        Serial.print("failed, rc=");
        Serial.println(_mqttClient.state());
      }
    }

    bool sendValues(long total, long partialLastMinute, long partialLastHour, long partialLastDay) {
      bool result = false;
      if (!_mqttClient.connected()) {
        connectMqtt();
      }
      if (_mqttClient.connected()) {
        const int capacity = JSON_OBJECT_SIZE(14);
        StaticJsonDocument<capacity> doc;
        // Em todo envio, manda o total
        doc[NAME_TOTAL] = total;
        // Manda os acumuladores que vieram
        if (partialLastMinute > -1) doc[NAME_LAST_MINUTE] = partialLastMinute;
        if (partialLastHour > -1) doc[NAME_LAST_HOUR] = partialLastHour;
        if (partialLastDay > -1) doc[NAME_LAST_DAY] = partialLastDay;
        // Tempo de funcionamento
        JsonObject upt  = doc.createNestedObject("uptime");
        upt["d"] = uptime.days();
        upt["h"] = uptime.hours();
        upt["m"] = uptime.minutes();
        upt["ts"] = uptime.totalSeconds();
      
        int len = measureJson(doc) + 1;
        char buffer[len];
        serializeJson(doc, buffer, len);
      
        Serial.println(buffer);
        if (_mqttClient.publish(mqttTopic, buffer)) {
          _publishCount++;
          result = true;
          Serial.println("Data sent to MQTT");
        } else {
          Serial.print("Error sending data to MQTT. rc=");
          Serial.println(_mqttClient.state());
        }
      }
      return result;
    }
};
