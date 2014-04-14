#pragma once

#include <aJSON.h>


class RingBuffer
{
    public:
        RingBuffer(int size, const char* name);
        RingBuffer();
        void init(int size, const char* name);
        void cleanup(int start, int end);
        void cleanup();
        int avg();
        int get_last_avg();
        aJsonObject * json();
        aJsonObject * json(aJsonObject *msg);
        void printjson(Stream * output);
        bool store(int value, int slot);
        char buf_name[4];
        void load(aJsonObject * data);

    private:
        int * buffer;
        int i;
        int buf_len;
        int index;
        int last_average;
};
