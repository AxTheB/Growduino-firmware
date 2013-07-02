#include <time.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#ifdef ARDUINO
#include <Wire.h>
#include "RTClib.h"
RTC_DS1307 RTC;
DateTime now;

#endif

#include <stdio.h>

long day_seconds() {
    //obsolete
#ifdef ARDUINO
    DateTime now = RTC.now();
    return (long) (now.hour() * 60 + now.minute()) * 60 + now.second();
#else
    time_t now;
    struct tm tm_now;
    time(&now);
    localtime_r(&now, &tm_now);
    return (long) (tm_now.tm_hour * 60 + tm_now.tm_min) * 60 + tm_now.tm_sec;
#endif

}

void daytime_init(){
#ifdef ARDUINO
    Wire.begin();
    RTC.begin();
    if (! RTC.isrunning()) {
        Serial.println("RTC not running");
        // following line sets the RTC to the date & time this sketch was compiled
        RTC.adjust(DateTime(__DATE__, __TIME__));
    }
    now = RTC.now();
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

#endif
}

int daytime_min(){
    now = RTC.now();
    return (int) now.minute();
}

int daytime_hour(){
    // time is set at daytime_min!
    return (int) now.hour();
}

int daytime_day(){
    // time is set at daytime_min!
    return (int) now.day();
}

int daytime_month(){
    // time is set at daytime_min!
    return (int) now.month();
}

int daytime_year(){
    // time is set at daytime_min!
     return (int) now.year();
}

#ifndef ARDUINO
main() {
    long ds = day_seconds();
    printf("Seconds since midnight %ld", ds);
}
#endif
