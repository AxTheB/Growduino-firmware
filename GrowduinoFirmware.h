#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define MINVALUE -999


#include "config.h"
#include "Logger.h"
#include "RingBuffer.h"
#include "daytime.h"
#include "sdcard.h"
