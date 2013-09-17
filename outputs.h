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
    int flip(int slot);
    int save();

private:
    int hw_update(int slot);
    void common_init();
    unsigned char sensor_state;
    RingBuffer states;

};
