#pragma once
#include <SD.h>
#include <aJSON.h>

void sdcard_init();

void file_write(const char * dirname, const char * filename, aJsonObject * data);

aJsonObject * file_read(const char * dirname, const char * filename);

