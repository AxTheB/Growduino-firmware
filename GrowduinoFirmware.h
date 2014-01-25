#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <avr/pgmspace.h>

#include <Time.h>

#define MINVALUE -999

#define GET 1
#define POST 2

#define DHT22_PIN 23
#define ONEWIRE_PIN 22
#define RELAY_START 25

#define USOUND_TRG 34
#define USOUND_ECHO 35

#define BUFSIZ 256

#define TRIGGERS 10

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

extern int ether;
