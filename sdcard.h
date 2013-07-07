#pragma once
#include <SD.h>
#include <aJSON.h>

void sdcard_init();

void file_write(char * dirname, char * filename, aJsonObject * data);
