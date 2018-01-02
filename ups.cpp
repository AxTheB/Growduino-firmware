#include "GrowduinoFirmware.h"
extern int ups_level;


int ups_init() {
#ifdef HAVE_UPS
  Serial3.begin(19200);
#endif
}

int ups_read_inner() {
  char line[64];
  int state;
  int energy;
  int counter;
  // = Serial3.readBytesUntil('s', line, 64);
  //counter = Serial3.readBytesUntil('\n', line, 64);  //throw away first line, as it may not be complete

  if (Serial3.available()) {
    strcpy(line, "................................................................");

    counter = Serial3.readBytesUntil('\n', line, 64);
#ifdef DEBUG_UPS
    SERIAL.print(F("Reading UPS: "));
    SERIAL.println(line);
#endif

    if (line[0] != 's') { // UPS stats start with 's'
      // UPS error handling may come here
#ifdef DEBUG_UPS
      SERIAL.println(F("Wrong line start"));
#endif
      return MINVALUE;
    }

    line[counter] = '\0';
    sscanf(line, "s:%i e:%i", &state, &energy);
    if (energy < 0 || energy > 100) {
#ifdef DEBUG_UPS
      SERIAL.println(F("Value out of bounds"));
#endif
      energy = MINVALUE;
    }
#ifdef DEBUG_UPS
    SERIAL.println(F("Value OK"));
#endif
    ups_level = state;
  } else {
#ifdef DEBUG_UPS
    SERIAL.println(F("Serial buffer empty"));
#endif
    delay(1000);
    energy = MINVALUE;
  }


  return energy;
}

int ups_read() {
#ifdef HAVE_UPS
  int retval;
  for (int i = 0; i < 10; i++) {
#ifdef DEBUG_UPS
    SERIAL.print(F("UPS reading try #"));
    SERIAL.println(i + 1);
#endif
#ifdef WATCHDOG
    SERIAL.println(F("Watchdog reset UPS"));
    wdt_reset();
#endif
    retval = ups_read_inner();
    if (retval != MINVALUE) {
      return retval;
    }
  }
#endif
  return MINVALUE;
}
