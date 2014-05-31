#pragma once

#include "GrowduinoFirmware.h"

#include "RingBuffer.h"
#include <aJSON.h>


class Output
{
public:
    Output();
    Output(aJsonObject * json);

    int get(int slot);
    int hw_get(int slot);
    //int set(int slot, int state);
    //int set_delayed(int slot, int state);
    int flip(int slot);
    int save();
    void load();
    bool match(const char * request);
    char name[9];
    // aJsonObject * json(aJsonObject *msg);
    // aJsonObject * json();
    void json(Stream * msg);
    void log();
    int hw_update(int slot);
    time_t uptime(int slot);
    char * dir_name(char * dirname);
    char * file_name(char * filename);
    int set(int slot, int val, int trigger);
    int breakme(int slot, int val, int trigger);
    void kill(int slot, int trigger);
    void revive(int slot, int trigger);
    void common_init();

private:
    unsigned char sensor_state;
#if TRIGGERS < 17
    int state[OUTPUTS];
#else
    long state[OUTPUTS];
#endif
    int hw_state[OUTPUTS];
    time_t ctimes[OUTPUTS];
    int broken[OUTPUTS];
    int log_states[LOGSIZE];
    time_t log_times[LOGSIZE];
    int log_index;
    int log_file_index;

    int pack_states();
    int bitget(int value, int slot);
    int bitset(int value, int slot);
    int bitclr(int value, int slot);
};

