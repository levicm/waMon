#define PRINT_DEBUG_MESSAGES
#include "ThingSpeak.h"
#include "Handler.h"
#include "Credentials.h"

const unsigned long tsChannelNumber = TS_CHANNEL_NUMBER;
const char * tsWriteAPIKey = TS_WRITE_API_KEY;
const char * tsReadAPIKey = TS_READ_API_KEY;

const byte ID_TOTAL = 1;
const byte ID_LAST_MINUTE = 2;
const byte ID_LAST_HOUR = 3;
const byte ID_LAST_DAY = 4;
const byte ID_SENDS = 7;
const byte ID_UPTIME = 8;

class ThingspeakHandler : public Handler {
  private:
    WiFiClient  _tsWifiClient;
  public:
    void setup() {
      ThingSpeak.begin(_tsWifiClient); 
    }
  
    void loop() {
    }

    long readTotal() {
      long result = 0;
      Serial.println("Getting total from ThingSpeak...");
      long field1 = ThingSpeak.readLongField(tsChannelNumber, ID_TOTAL, tsReadAPIKey);  
      int statusCode = ThingSpeak.getLastReadStatus();
      if(statusCode == 200) {
        result = field1;
        Serial.println("total: " + String(result));
      } else {
        Serial.println("Problem reading channel. HTTP error code " + String(statusCode)); 
      }
      return result;
    }

    bool sendValues(long total, long partialLastMinute, long partialLastHour, long partialLastDay) {
      bool result = false;
      // Contadores
      if (total > -1) ThingSpeak.setField(ID_TOTAL, total);
      if (partialLastMinute > -1) ThingSpeak.setField(ID_LAST_MINUTE, partialLastMinute);
      if (partialLastHour > -1) ThingSpeak.setField(ID_LAST_HOUR, partialLastHour);
      if (partialLastDay > -1) ThingSpeak.setField(ID_LAST_DAY, partialLastDay);
      // Telemetria e saúde
      if (partialLastHour > -1) {
        ThingSpeak.setField(ID_SENDS, _publishCount);
        _publishCount = 0;
      }
      ThingSpeak.setField(ID_UPTIME, (int)(millis() / 60000));

      int code = ThingSpeak.writeFields(tsChannelNumber, tsWriteAPIKey);
      if (code == 200) {
        Serial.println("Data sent to ThingSpeak");
        _publishCount++;
        result = true;
      } else {
        Serial.print("Error sending data to ThingSpeak: ");
        Serial.println(code);
      }
      return result;
    }
};
