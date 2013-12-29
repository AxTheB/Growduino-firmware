#pragma once

#include "RingBuffer.h"
#include <aJSON.h>


class Output
{
public:
    Output();
    Output(aJsonObject * json);

    int get(int slot);
    int set(int slot, int state);
    int set_delayed(int slot, int state);
    int flip(int slot);
    int save();
    bool match(const char * request);
    char name[9];
    aJsonObject * json_dynamic();
    void log();
    int hw_update(int slot);


private:
    void common_init();
    unsigned char sensor_state;
    RingBuffer states;
};

