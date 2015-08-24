#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <avr/pgmspace.h>
#include <avr/wdt.h>

//#define WATCHDOG 1

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <Time.h>


#define DHT22_NO_FLOAT 1

#define MINVALUE -999

#define GET 1
#define POST 2

// light sensor on analog A1
#define LIGHT_SENSOR_PIN_1 8
#define LIGHT_SENSOR_PIN_2 9
#define LIGHT_SENSOR_PIN_UPS 10

#define DHT22_PIN 22
#define ONEWIRE_PIN 23
#define ONEWIRE_PIN2 24
#define USOUND_TRG 25
#define USOUND_ECHO 26


#define RELAY_START 34


#define BUFSIZE 256

#define ALARM_STR_MAXSIZE 32

// TRIGGERS - ALERTS must be 31 or lower, or random crashes occur. Really.
#define TRIGGERS 42
#define ALERTS 12


#define ALERT_MSG_LEN 64
#define ALERT_TARGET_LEN 32

#define OUTPUTS 12

#define LOGGERS 7

#define DEBUG 1

#define LCD_BUFFER_LINES 6

#define MEGA 1

//#define USE_GSM 1
//#define GSM_ENABLE 6

#ifdef DEBUG
#define DEBUG_OUTPUT 1
#define DEBUG_SDCARD 1
#define DEBUG_RB_DATA 1
#define DEBUG_TRIGGERS 1
#define DEBUG_ALERTS 1
#define DEBUG_HTTP 1
#define DEBUG_SMTP 1
#endif

// How many output chages do we keep in memory
#define LOGSIZE 25

// #include "config.h"
#include "Logger.h"
#include "RingBuffer.h"
#include "daytime.h"
#include "sdcard.h"
#include "config.h"
#include "outputs.h"
#include "trigger.h"
#include "ultrasound.h"
#include "ds.h"
#include "Lcd.h"
#include "alerts.h"
#include "smtp.h"

extern int ether;
void pFreeRam();
extern File sd_file;

#define NONE -1
#define STATE_ON 1
#define STATE_OFF 0
#define STATE_ON_ALWAYS 2
