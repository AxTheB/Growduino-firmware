#include "GrowduinoFirmware.h"

#include <SD.h>

const int chipSelect = 4;

void sdcard_init(){
    pinMode(53, OUTPUT);
    SD.begin(chipSelect);
}

void file_write(char * dirname, char * filename, aJsonObject * data) {
    char filepath[60];

    if (!SD.exists(dirname)) {
        Serial.print("Creating directory: ");
        SD.mkdir(dirname);
    }

    strcpy(filepath, dirname);
    strcat(filepath, "/");
    strcat(filepath, filename);

    Serial.print("Writing file ");
    Serial.println(filepath);

    if (SD.exists(filepath)) {
        SD.remove(filepath);
    }

    File dataFile = SD.open(filepath, FILE_WRITE);
    if (dataFile) {
        aJsonStream sd_stream(&dataFile);
        aJson.print(data, &sd_stream);
        dataFile.close();
    } else {
        Serial.print("Failed to open ");
        Serial.println(filepath);
    }
}

aJsonObject * file_read(char * dirname, char * filename){
    char filepath[60];

    strcpy(filepath, dirname);
    strcat(filepath, "/");
    strcat(filepath, filename);

    Serial.print("opening file ");
    Serial.println(filepath);

    if (!SD.exists(filepath)) {
        Serial.println("File does not exist");
        return NULL;
    }
    File dataFile = SD.open(filepath, FILE_READ);
    if (dataFile) {
        aJsonStream sd_stream(&dataFile);
        aJsonObject * data = aJson.parse(&sd_stream);
        dataFile.close();
        return data;
    } else {
        Serial.println("File read failure");
        return NULL;
    }

}
