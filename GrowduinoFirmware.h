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

#define DHT22_PIN 23
#define ONEWIRE_PIN 22
#define RELAY_START 25

#define USOUND_TRG 34
#define USOUND_ECHO 35

#define BUFSIZE 256

#define ALARM_STR_MAXSIZE 32

// max. 32
#define TRIGGERS 16

#define ALERTS 7

#define OUTPUTS 8

#define LOGGERS 6

#define DEBUG 1

#define LCD_RESET 36
#define LCD_ENABLE 37
#define LCD_D1 38
#define LCD_D2 39
#define LCD_D3 40
#define LCD_D4 41

#define LCD_BUFFER_LINES 5

#define GSM_ENABLE 6

#define UPS_READ_PIN 8


#ifdef DEBUG
#define DEBUG_OUTPUT 1
//#define DEBUG_SDCARD 1
#define DEBUG_RB_DATA 1
#define DEBUG_TRIGGERS 1
#define DEBUG_HTTP 1
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
#define S_ON 1
#define S_OFF 0
