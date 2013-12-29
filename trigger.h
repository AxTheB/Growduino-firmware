#pragma once
#include "Logger.h"

#include <aJSON.h>

class Trigger {
    public:

        int t_since;
        int t_until;
        int on_value;
        int off_value;
        bool lt;
        int sensor;
        int output;
        int idx;

        Trigger();
        void load(aJsonObject *msg, Logger * loggers[]);
        void tick();
        aJsonObject * json(aJsonObject *cnfdata);

    private:
        Logger * _logger;
};

int trigger_load(Trigger triggers[], Logger * loggers[], aJsonObject * cfile, int trgno);
int triggers_load(Trigger triggers[], Logger * loggers[]);
int triggers_save(Trigger triggers[]);
