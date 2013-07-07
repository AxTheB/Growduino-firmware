#pragma once
#include "RingBuffer.h"

class Logger
{
    public:
        Logger();
        Logger(bool timed);
        Logger(const char * logger_name);
        void log(int value);
        void timed_log(int value);
        void setup(bool timed);
        bool available();
        // aJsonObject * json();
        RingBuffer l1, l2, l3;
        char name[9];
        char* dirname_l1(char *dirname);
        char* dirname_l2(char *dirname);
        char* dirname_l3(char *dirname);
        void load();
    private:
        bool timed;
        int l1_idx, l2_idx, l3_idx;

};
