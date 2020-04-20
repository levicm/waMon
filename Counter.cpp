#include <Arduino.h>
#include "Counter.h"

Counter::Counter(Handler* handler) {
  _handler = handler;
}

void Counter::setTotal(long total) {
  _total = total;
}

void Counter::count() {
  _total++;
  _partialLastMinute++;
  _partialLastHour++;
  _partialLastDay++;      
}

void Counter::initTimers() {
  long now = millis();
  _nextMinute = now + T_MINUTE;
  _nextHour = now + T_HOUR;
  _nextDay = now + T_DAY;
}

void Counter::setup() {
  initTimers();
  _handler->setup();
}

void Counter::loop() {
  long now = millis();

  long lastMinuteValue = -1;
  long lastHourValue = -1;
  long lastDayValue = -1;

  if (now > _nextMinute) {
    lastMinuteValue = _partialLastMinute;
    if (now > _nextHour) {
      lastHourValue = _partialLastHour;
    }
    if (now > _nextDay) {
      lastDayValue = _partialLastDay;
    }

    if (now > (_lastFail + T_FAIL_DELAY)) {
      Serial.println("Vai tentar enviar..."); 
      //noInterrupts(); // Parece boa pratica suspender interrupcoes nos momentos de tratamento, mas gerou bug com o AsyncElegantOTA
      boolean sent = _handler->sendValues(_total, lastMinuteValue, lastHourValue, lastDayValue);
      //interrupts(); // Volta a receber interrupcoes
      if (sent) {
        _sendCount++;
        _lastSend = now / 1000;
        _sequenceFailCount = 0;
        // Se houve incremento durante o envio, recupera esse incremento
        if (_partialLastMinute > lastMinuteValue) {
          _partialLastMinute = _partialLastMinute - lastMinuteValue;
          _incsDuringSend++;
        } else 
          _partialLastMinute = 0;
        _nextMinute = now + T_MINUTE;

        if (lastHourValue > -1) {
          // Se houve incremento durante o envio, recupera esse incremento
          if (_partialLastHour > lastHourValue)
            _partialLastHour = _partialLastHour - lastHourValue;
          else
            _partialLastHour = 0;
          _nextHour = now + T_HOUR;
        }
        if (lastDayValue > -1) {
          // Se houve incremento durante o envio, recupera esse incremento
          if (_partialLastDay > lastDayValue)
            _partialLastDay = _partialLastDay - lastDayValue;
          else
            _partialLastDay = 0;
          _nextDay = now + T_DAY;
        }
      } else {
        _failCount++;
        _sequenceFailCount++;
        if (_sequenceFailCount > FAIL_COUNT_LIMIT) {
          Serial.println("Too many fails in sequence. Restarting..");
          ESP.restart();
        }
        _lastFail = now;
      }
    }
  }
  _handler->loop();
}

String Counter::toString() {
  String result = "";
  return result + "Total: " + _total + "\r\n" +
                  "Partial last minute: " + _partialLastMinute + "\r\n" +
                  "Partial last hour: " + _partialLastHour + "\r\n" +
                  "Partial last day: " + _partialLastDay + "\r\n" +
                  "Last send: " + _lastSend + "\r\n"
                  "Sends: " + _sendCount + "\r\n"
                  "Fails: " + _failCount + "\r\n"
                  "Sequence fails: " + _sequenceFailCount + "\r\n"
                  "Increments during send: " + _incsDuringSend;
}
