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
        //aJsonObject * json(aJsonObject *cnfdata);
        void json(Stream * cnfdata);
        int send_message();
        int process_alert(int trigger_state);
        void trash_strings();
};

void alerts_save(Alert alerts[]);
int alerts_load(Alert alerts[]);
void alert_save(Alert alerts[], int idx);
void alert_load(aJsonObject * cfile, int alert_no);
