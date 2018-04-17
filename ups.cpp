#include "GrowduinoFirmware.h"
extern int ups_level;


void ups_init() {
#ifdef HAVE_UPS
  Serial3.begin(19200);
#endif
}

void flush_serial(){
  while (Serial3.available() >0 ) {
#ifdef WATCHDOG
    wdt_reset();
#endif
    Serial3.read();
  }
}

int ups_read_inner2() {
  char line[64];
  int state;
  int energy;
  energy = MINVALUE;
  int counter;

  if (Serial3.available()) {
    strcpy(line, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");

    counter = Serial3.readBytesUntil('\n', line, 64);
    line[counter] = '\0';
#ifdef DEBUG_UPS
      SERIAL.print(F("D: UPS received: "));
      SERIAL.println(line);
#endif

    if (line[0] != 's') { // UPS stats start with 's'
      flush_serial();
      return MINVALUE;
    }

    sscanf(line, "s:%i e:%i", &state, &energy);
    if (energy < 0 || energy > 100) {
      energy = MINVALUE;
    }
    ups_level = state;
  }

  return energy;
}

int ups_read_inner(){
    int retval;
    for (int i = 0; i < 10; i++) {
#ifdef WATCHDOG
#ifdef DEBUG_WATCHDOG
        SERIAL.print(F("Watchdog reset UPS inner "));
        SERIAL.println(i);
#endif
        wdt_reset();
#endif
        retval = ups_read_inner2();
        if (retval != MINVALUE) {
#ifdef DEBUG_UPS
            SERIAL.print(F("D: UPS read returning "));
            SERIAL.print(retval);
            SERIAL.print(F(" on read #"));
            SERIAL.println(i);
#endif
            return retval;
        }
        delay(400);
    }
}

int ups_read() {
#ifdef WATCHDOG
#ifdef DEBUG_WATCHDOG
  SERIAL.println(F("Watchdog reset UPS"));
#endif
  wdt_reset();
#endif
  flush_serial();
  delay(1000);
#ifdef WATCHDOG
#ifdef DEBUG_WATCHDOG
  SERIAL.println(F("Watchdog reset after clear"));
#endif
  wdt_reset();
#endif
  int retval;
  return triple_read(&ups_read_inner);
}
