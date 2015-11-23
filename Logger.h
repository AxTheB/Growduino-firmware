#pragma once
#include "RingBuffer.h"


class Logger
{
    public:
        char name[9];  //name, like "Temp1" or "Light"

        Logger(const char * logger_name);
        //void log(int value);
        void timed_log(int value);
        void setup();
        bool available();
        aJsonObject * json();
        void printjson(Stream * output);
        char* dirname_l1(char *dirname);
        char* dirname_l2(char *dirname);
        char* dirname_l3(char *dirname);
        void load();
        void load_all();
        bool match(const char * request);
        time_t time;  // from Time.h, included by GrowduinoFirmware.h
        int peek();

    private:
        int buf_min[60];

        int idx_min; // postiton of last save
        int idx_min_new; // postiton of new data
        int idx_h_new;
        int idx_day_new;
        bool timed;
        int peekval;

};
