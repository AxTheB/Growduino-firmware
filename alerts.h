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
};
