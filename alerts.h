#pragma once

class Alert {
    public:
        int trigger;
        int idx;
        char * on_message;
        char * off_message;
        char * target;  // mail or phone number
        int last_state;

        Alert();
        void init();
        void load(aJsonObject *msg, int index);
        int tick();
        aJsonObject * json(aJsonObject *cnfdata);
        int send_message();
        int process_alert(int trigger_state);
};

int alerts_save(Alert alerts[]);
int alerts_load(Alert alerts[]);
int alert_save(Alert alerts[], int idx);
int alert_load(aJsonObject * cfile, int alert_no);
