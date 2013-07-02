#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "RingBuffer.h"

#include <SD.h>

#include "sdcard.h"

const int chipSelect = 4;

void sdcard_init(){
    pinMode(53, OUTPUT);
    SD.begin(chipSelect);
}

void file_write(char * dirname, char * filename, char * data) {
    if (!SD.exists(dirname)) {
        Serial.print("Creating directory: ");
        SD.mkdir(dirname);
    } else {
        Serial.print("Directory exists: ");
    }
    Serial.println(dirname);

    char filepath[60];
    strcpy(filepath, dirname);
    strcat(filepath, "/");
    strcat(filepath, filename);

    File dataFile = SD.open(filepath, (O_WRITE | O_CREAT | O_TRUNC));
    if (dataFile) {
        dataFile.println(data);
        dataFile.close();
        Serial.print("Written file: ");
        Serial.println(filepath);
        Serial.println(data);
    } else {
        Serial.print("Failed to open file: ");
        Serial.println(filepath);
    }

}
