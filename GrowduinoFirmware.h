#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Time.h>

#define MINVALUE -999

#define GET 1
#define POST 2

//first relay is actually at RELAY_START+1 !!
#define RELAY_START 24

#define BUFSIZ 513


// #include "config.h"
#include "Logger.h"
#include "RingBuffer.h"
#include "daytime.h"
#include "sdcard.h"
#include "config.h"
#include "outputs.h"

extern int ether;
