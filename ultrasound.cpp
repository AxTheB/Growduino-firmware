#include "GrowduinoFirmware.h"


unsigned long timeStart;
unsigned long timeIn;
unsigned long timeOut;
int state;

long timetocm(long mtime) {
  if (mtime > 0) {
    return mtime / 27 / 2 ;
  } else return 0;
}

int clip(long value) {
  if (value > 32000) value = 32000;
  return (int) lrint(value);
}


long ultrasound_ping_inner(int trigger, int echo) {
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);
  state = LOW;

  digitalWrite(trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);

  timeStart = micros();

  while (state == LOW && (micros() - timeStart < 40000)) {
    state = digitalRead(echo);
  }
  timeIn = micros();

  while (state == HIGH && (micros() - timeStart < 40000)) {
    state = digitalRead(echo);
  }
  timeOut = micros();

  if (timeOut - timeStart >= 40000) {
    return MINVALUE;
  }
  return timetocm(timeOut - timeIn);
}

int ultrasound_ping() {
    long r = ultrasound_ping(USOUND_TRG, USOUND_ECHO);
    if (r > 30000) return 30000;
    return (int) r;
}

long ultrasound_ping(int trigger, int echo) {
  long distance;
  distance = MINVALUE;
  for (int i = 0; (i < 3 && distance == MINVALUE) ; i++) {
    delay(10);
    distance = ultrasound_ping_inner(trigger, echo);
  }
  return distance;
}

