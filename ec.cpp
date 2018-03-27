#include "GrowduinoFirmware.h"
#include "ec.h"
// #include <stdlib.h>
extern Config config;

void ec_enable() {
#ifdef USE_EC_SENSOR
  pinMode(EC_DATA, INPUT);
  pinMode(EC_ENABLE, OUTPUT);
#endif
}

long ec_read_raw() {

  long lowPulseTime_t = 0;
  long highPulseTime_t = 0;
  long lowPulseTime = 0;
  long highPulseTime = 0;
  long pulseTime;

  bool pulseError = false;

  digitalWrite(EC_ENABLE, HIGH); // power up the sensor
  delay(100);
#ifdef WATCHDOG
#ifdef DEBUG_WATCHDOG
  SERIAL.print(F("Watchdog reset: ec read"));
#endif
  wdt_reset();
#endif
  for (unsigned int j = 0; j < EC_SAMPLE_TIMES; j++) {
    highPulseTime_t = pulseIn(EC_DATA, HIGH, EC_TIMEOUT);
    if (j == 0 and highPulseTime_t == 0)  {//abort on timeout on first read
#ifdef WATCHDOG
#ifdef DEBUG_WATCHDOG
      SERIAL.println(F(" - abort "));
#endif
      wdt_reset();
#endif
      return MINVALUE;
    }

    highPulseTime += highPulseTime_t;
    if (highPulseTime_t == 0) {
      pulseError = true;
    }

    lowPulseTime_t = pulseIn(EC_DATA, LOW, EC_TIMEOUT);
    lowPulseTime += lowPulseTime_t;

    if (lowPulseTime_t == 0) {
      pulseError = true;
    }

#ifdef WATCHDOG
#ifdef DEBUG_WATCHDOG
    SERIAL.print(F("."));
#endif
    wdt_reset();
#endif
  }

  lowPulseTime = lowPulseTime / EC_SAMPLE_TIMES;
  highPulseTime = highPulseTime / EC_SAMPLE_TIMES;

  pulseTime = (lowPulseTime + highPulseTime) / 2;

  digitalWrite(EC_ENABLE, LOW); // power down the sensor

#ifdef WATCHDOG
#ifdef DEBUG_WATCHDOG
  if (pulseError) {
    SERIAL.println(F(" with errors"));
  } else {
    SERIAL.println(F(" OK"));
  }
#endif
    wdt_reset();
#endif

  if (pulseTime >= EC_CUTOFF) {
    return MINVALUE;
  }


  return pulseTime;
}

int ec_read() {
  int ec = MINVALUE;
#ifdef USE_EC_SENSOR
  long pulseTime;
  float ec_a, ec_b;


  float c_low = 1.278;
  float c_high = 4.523;
  float ec_high_ion = (float) (config.ec_high_ion + config.ec_offset);
  float ec_low_ion = (float) (config.ec_low_ion + config.ec_offset);

  ec_a =  (c_high - c_low) / (1 / ec_high_ion - 1 / ec_low_ion);
  ec_b = c_low - ec_a / (float) ec_low_ion;

  pulseTime = ec_read_raw();

  ec = (int) 100 * (ec_a / (pulseTime + config.ec_offset) + ec_b);
  if (pulseTime == MINVALUE) {
    ec = MINVALUE;
  }

#endif

#ifdef DEBUG_CALIB
  if (ec != MINVALUE) {
    SERIAL.print(F("EC pulse time: "));
    SERIAL.println(pulseTime);
  }
#endif
  return ec;
}

int compare(const void *p, const void *q) {
  long x = *(const int *)p;
  long y = *(const int *)q;

  /* Avoid return x - y, which can cause undefined behaviour
    because of signed integer overflow. */
  if (x < y)
    return -1;  // Return -1 if you want ascending, 1 if you want descending order.
  else if (x > y)
    return 1;   // Return 1 if you want ascending, -1 if you want descending order.

  return 0;
}

long ec_calib_raw() {
  int size  = 7;
  long rawdata[size];
  for (int i = 0; i < size; i++) {
    rawdata[i] = ec_read_raw();
  }
  qsort(rawdata, size, sizeof(long), compare);

  long sum = 0;

  for (int i = 1; i < size - 1; i++) {
    sum += rawdata[i];
  }
  return sum / (size - 2);
}
