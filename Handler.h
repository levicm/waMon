#ifndef HANDLER_H
#define HANDLER_H

class Handler {
  protected:
    int _publishCount = 0;

  public:
    virtual void setup();
    virtual void loop();
    virtual bool sendValues(long total, long partialLastMinute, long partialLastHour, long partialLastDay);
};

#endif
