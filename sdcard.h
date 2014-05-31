#pragma once
#include <SD.h>
#include <aJSON.h>

void sdcard_init();
bool file_exists(const char * dirname);
bool file_exists(const char * dirname, const char * filename);

void file_write(const char * dirname, const char * filename, aJsonObject * data);
int file_for_write(const char * dirname, const char * filename, File * dataFile);
void file_passthru(const char * dirname, const char * filename, Stream * input);

aJsonObject * file_read(const char * dirname, const char * filename);

