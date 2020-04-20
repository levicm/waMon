#ifndef COUNTER_H
#define COUNTER_H

#include <Arduino.h>
#include "Handler.h"

const unsigned long T_SECOND     = 1000;
const unsigned long T_MINUTE     = T_SECOND * 60;
const unsigned long T_HOUR       = T_MINUTE * 60;
const unsigned long T_DAY        = T_HOUR * 24;
const unsigned long T_FAIL_DELAY = T_SECOND * 5;

const unsigned long FAIL_COUNT_LIMIT = 60;

class Counter {
  private:
    // Momentos
    unsigned long _nextMinute = 0;
    unsigned long _nextHour = 0;
    unsigned long _nextDay = 0;
    
    // Acumuladores
    long _total = 0;
    long _partialLastMinute = 0;
    long _partialLastHour = 0;
    long _partialLastDay = 0;

    // Saúde do sensor
    long _incsDuringSend = 0;
    long _sendCount = 0;
    long _lastSend = 0;
    long _failCount = 0;
    long _lastFail = 0;
    long _sequenceFailCount = 0;

    Handler* _handler;

  public:
    Counter(Handler* handler);

    void setTotal(long total);

    void count();

    void initTimers();

    void setup();

    void loop();

    String toString();
};

#endif
