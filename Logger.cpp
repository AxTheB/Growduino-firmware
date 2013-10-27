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
        l1 = RingBuffer(60, "min");
        l2 = RingBuffer(24, "h");
        l3 = RingBuffer(31, "day");
    } else {
        l1 = RingBuffer(10, "l1");
        l2 = RingBuffer(10, "l2");
        l3 = RingBuffer(10, "l3");
        l1_idx = -1;
        l2_idx = -1;
        l3_idx = -1;
    }
    //load();
}

void Logger::load() {
    Serial.println("Recovering state");
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
        Serial.println("fail");
    }


    dirname_l2(dirname);
    sprintf(filename, "%02d.jso", day());
    data = file_read(dirname, filename);
    if (data) {
        l2.load(data);
        aJson.deleteItem(data);
    } else {
        Serial.println("fail");
    }

    dirname_l3(dirname);
    sprintf(filename, "%02d.jso", month());
    data = file_read(dirname, filename);
    if (data) {
        l3.load(data);
        aJson.deleteItem(data);
    } else {
        Serial.println("fail");
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
    aJsonObject *msg;

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
        msg = l1.json();
        file_write(dirname, filename, msg);
        aJson.deleteItem(msg);
    }
    if (l1_idx == 59 || log_every_time) {
        // l2 - write at end of hour
        dirname_l2(dirname);
        sprintf(filename, "%02d.jso", day());
        msg = l2.json();
        file_write(dirname, filename, msg);
        aJson.deleteItem(msg);
        if (l2_idx == 23 || log_every_time) { 
            // at end of the day, write l3 buffer.
            dirname_l3(dirname);
            sprintf(filename, "%02d.jso", month());
            msg = l3.json();
            file_write(dirname, filename, msg);
            aJson.deleteItem(msg);
        }
    }
}

aJsonObject * Logger::json(){
    // create json with this roation only
    aJsonObject *msg = aJson.createObject();
    msg = l1.json(msg);
    msg = l2.json(msg);
    msg = l3.json(msg);
    return msg;
}

aJsonObject * Logger::json_dynamic(){
    // create json with unwinded buffers (all values, most recent last)
    aJsonObject *msg = aJson.createObject();
    // aJson.addNumberToObject(msg, "time", (double) time);
    aJson.addStringToObject(msg, "name", name);
    msg = l1.json_dynamic(msg);
    msg = l2.json_dynamic(msg);
    msg = l3.json_dynamic(msg);
    return msg;
}

void Logger::log(int value) {
    //zapise do l1 bufferu, pripadne buffery otoci.
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
