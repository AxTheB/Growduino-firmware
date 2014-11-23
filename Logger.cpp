#include "GrowduinoFirmware.h"

#include <string.h>

int log_every_time = true;

Logger::Logger(){
    setup(true);
}

Logger::Logger(const char * logger_name){
    setup(true);
    strncpy(name, logger_name, 8);
    name[8]='\0';
}

void Logger::setup(bool timed){
    if (timed) {
        //prepare buffers
        l1.init(60, "min");
        l2.init(24, "h");
        l3.init(31, "day");
    } else {
        l1.init(10, "l1");
        l2.init(10, "l2");
        l3.init(10, "l3");
        l1_idx = -1;
        l2_idx = -1;
        l3_idx = -1;
    }
}

void Logger::load() {
    Serial.println(F("Recovering state"));
    char dirname[64];
    char filename[13];
    aJsonObject * data;

    dirname_l1(dirname);
    sprintf(filename, "%02d.jso", hour());
    data = file_read(dirname, filename);
    if (data) {
        l1.load(data);
        aJson.deleteItem(data);
    } else {
        Serial.println(F("l1 fail"));
    }


    dirname_l2(dirname);
    sprintf(filename, "%02d.jso", day());
    data = file_read(dirname, filename);
    if (data) {
        l2.load(data);
        aJson.deleteItem(data);
    } else {
        Serial.println(F("l2 fail"));
    }

    dirname_l3(dirname);
    sprintf(filename, "%02d.jso", month());
    data = file_read(dirname, filename);
    if (data) {
        l3.load(data);
        aJson.deleteItem(data);
    } else {
        Serial.println(F("l3 fail"));
    }
}

bool Logger::available() {
    // for timed logging, return false if we already logged this minute
    if ( l1_idx == minute() && l2_idx == hour() && l3_idx == day()-1) {
        return false;
    }
    return true;
}

bool Logger::match(const char * request){
    // return true if we want this logger
    char * filename;
    char filebuf[13];

    filename = strrchr(request, '/');

    if (filename == NULL) {
    // something weird happened, we should have / in request
    // as we are sensors/sensor.jso
        return false;
    }

    // we can get weird things in filename
    // otoh name is safe, so make filename from name and compare that
    filename = filename + 1;
    sprintf(filebuf, "%s.jso", name);

    return (strcasecmp(filename, filebuf) == 0);
}

char * Logger::dirname_l1(char * dirname){
        sprintf(dirname, "/data/%s/%d/%02d/%02d", name, year(), month(), day());
        return dirname;
}

char * Logger::dirname_l2(char * dirname){
        sprintf(dirname, "/data/%s/%d/%02d", name, year(), month());
        return dirname;
}

char * Logger::dirname_l3(char * dirname){
            sprintf(dirname, "/data/%s/%d", name, year());
        return dirname;
}

void Logger::timed_log(int value) {
    //Write value to l1, recalculate l2 and l3 buffer
    peekval = value;

    time = now();

    l1_idx = minute();
    l2_idx = hour();
    l3_idx = day() - 1;
    l1.store(value, l1_idx);
    l2.store(l1.avg(), l2_idx);
    l3.store(l2.avg(), l3_idx);

    //write to card
    char dirname[40];
    char filename[13];
    if (l1_idx % 5 == 0 || log_every_time) {
        // l1 - write every 5 min
        dirname_l1(dirname);
        sprintf(filename, "%02d.jso", hour());
        file_for_write(dirname, filename, &sd_file);
        sd_file.print(F("{"));
        l1.printjson(&sd_file, false);
        sd_file.print(F("}"));
        sd_file.close();
        // l2
        dirname_l2(dirname);
        sprintf(filename, "%02d.jso", day());
        file_for_write(dirname, filename, &sd_file);
        sd_file.print(F("{"));
        l2.printjson(&sd_file, false);
        sd_file.print(F("}"));
        sd_file.close();
        // l3
        dirname_l3(dirname);
        sprintf(filename, "%02d.jso", month());
        file_for_write(dirname, filename, &sd_file);
        sd_file.print(F("{"));
        l3.printjson(&sd_file, false);
        sd_file.print(F("}"));
        sd_file.close();
    }
}

/*
aJsonObject * Logger::json(){
    // create json with this roation only
    aJsonObject *msg = aJson.createObject();
    msg = l1.json(msg);
    msg = l2.json(msg);
    msg = l3.json(msg);
    return msg;
}
*/

void Logger::printjson(Stream * output){
    output->print("{\"name\":\"");
    output->print(name);
    output->print("\",");
    l1.printjson(output);
    output->print("}");
}

void Logger::log(int value) {
    //zapise do l1 bufferu, pripadne buffery otoci.
    peekval = value;
    time = now();

    l1.store(value, ++l1_idx);
    int tmpavg = l1.get_last_avg();
    if (tmpavg != MINVALUE){
        l2.store(tmpavg, ++l2_idx);
        int tmpavg = l2.get_last_avg();
        if (tmpavg != MINVALUE){
            l3.store(tmpavg, ++l3_idx);
            //int tmpavg = l2.get_last_avg();
        }
    }
}

int Logger::peek() {
    return peekval;
}
