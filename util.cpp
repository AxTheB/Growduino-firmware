#include "GrowduinoFirmware.h"

int analogReadAvg(int pin) {
  // introduce delay between mux switch and actual reading
  analogRead(pin);
  delay(ANALOG_READ_AVG_DELAY);
  analogRead(pin);

#ifdef DEBUG_CALIB
  int minval, maxval;
  minval = MINVALUE;
  maxval = MINVALUE;
  SERIAL.print(F("Analog read "));
  SERIAL.println(pin);
#endif
  long dataSum = 0L;
  int data;
#ifdef WATCHDOG
#ifdef DEBUG_WATCHDOG
  SERIAL.print(F("Analog read avg timer reset"));
#endif
  wdt_reset();
#endif

  for (int i = 0; i < ANALOG_READ_AVG_TIMES; i++) {
    data = analogRead(pin);
    dataSum += data;
#ifdef WATCHDOG
#ifdef DEBUG_WATCHDOG
    SERIAL.print(F("."));
#endif
    wdt_reset();
#endif

#ifdef DEBUG_CALIB
    SERIAL.println(data);
    if (minval == MINVALUE || minval > data) {
      minval = data;
    }
    if (maxval == MINVALUE || maxval < data) {
      maxval = data;
    }
#endif

    delay(ANALOG_READ_AVG_DELAY);
  }

#ifdef WATCHDOG
#ifdef DEBUG_WATCHDOG
  SERIAL.println(F(" done"));
#endif
  wdt_reset();
#endif

  int retval = (int) (dataSum / ANALOG_READ_AVG_TIMES);

#ifdef DEBUG_CALIB
  SERIAL.println("");
  SERIAL.print(F("min: "));
  SERIAL.print(minval);
  SERIAL.print(F(" max: "));
  SERIAL.print(maxval);
  SERIAL.print(F(" avg: "));
  SERIAL.println(retval);
#endif

  return retval;
}

int triple_read(int (* funct)()) {
  return return_middle((*funct)(), (*funct)(), (*funct)());
}

int triple_read(int (* funct)(int), int param1) {
  return return_middle((*funct)(param1), (*funct)(param1), (*funct)(param1));
}

int return_middle(int first_value, int second_value, int third_value) {
#ifdef WATCHDOG
  wdt_reset();
#endif
  int values[] = {first_value, second_value, third_value};
  int tmpval;
  if (values[0] > values[1]) {
    tmpval = values[1];
    values[1] = values[0];
    values[0] = tmpval;
  }

  if (values[1] > values[2]) {
    tmpval = values[2];
    values[2] = values[1];
    values[1] = tmpval;
  }

  if (values[0] > values[1]) {
    tmpval = values[1];
    values[1] = values[0];
    values[0] = tmpval;
  }

  if (values[0] != MINVALUE) {
    return values[1];  // middle value if there are not any MINVALUEs
  }

  if (values[1] == MINVALUE) {
    return values[2];
  } else {
    return (values[1] + values[2]) / 2;
  }
}


int perThousand(int pin) {
  int retval;
  retval = analogReadAvg(pin);
#ifdef ANALOG_DETECT
  if (retval > ADC_CUTOFF) {
    return MINVALUE;
  }
#endif
  retval = map(retval, 0, 1024, 0, 1000);
  return retval;
}
