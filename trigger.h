#pragma once
#include "Logger.h"

#include <aJSON.h>

struct Trigger {
        int active;
        int t_since;
        int t_until;
        int on_value;
        int off_value;
        char on_cmp;
        char off_cmp;
        int important;
        int sensor;
        int output;
        int state;
        Logger * _logger;
};

        void trigger_init(int idx);
        void load(int idx, aJsonObject *msg, Logger * loggers[]);
        int trigger_tick(int idx);
        void trigger_json(int idx, Stream * cnfdata);
        void trigger_set_default_state(int idx);

//void trigger_load(Trigger triggers[], Logger * loggers[], aJsonObject * cfile, int trgno);
void trigger_load(int idx, aJsonObject *msg, Logger * loggers[]);
int triggers_load(Trigger triggers[], Logger * loggers[]);
void triggers_save(Trigger triggers[]);
void trigger_save(Trigger triggers[], int idx);
