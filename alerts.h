#pragma once
#include <aJSON.h>

struct Alert {
        int trigger;
        int last_state;
};

void alert_load_target(int idx, aJsonObject *msg);
void alert_load(int idx, aJsonObject *msg, char * on_message, char * off_message, char * target);
int process_alert(int idx, int trigger_state);
int alert_send_message(int idx);
int alert_tick(int idx);
void alert_json(int idx, Stream * cnfdata, char * on_message, char * off_message, char * target);
void alert_passthru(int idx, Stream * source_stream);

void alerts_save();
int alerts_load();
void alert_save(int idx);
void alert_load(aJsonObject * cfile, int alert_no);
