#pragma once

#include <aJSON.h>

//#define MINVALUE -32768
#define MINVALUE -3

class RingBuffer
{
    public:
        RingBuffer(int size, const char* name);
        RingBuffer();
        void cleanup(int start, int end);
        void cleanup();
        int avg();
        int get_last_avg();
        aJsonObject * json();
        aJsonObject * json(aJsonObject *msg);
        bool store(int value, int slot);

    private:
        int * buffer;
        int i;
        int buf_len;
        int index;
        char name_[4];
        char jsontmp[10];
        int last_average;
};
