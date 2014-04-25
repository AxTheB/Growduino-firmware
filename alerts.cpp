#include "GrowduinoFirmware.h"
extern Alert alerts[];
extern Output outputs;
extern Trigger triggers[];
extern int ups_level;
extern Config config;

Alert::Alert(){
    on_message = NULL;
    off_message = NULL;
    target = NULL;
    init();
}

void Alert::init(){
    trigger = NONE;
    if (on_message != NULL) {
        free(on_message);
        on_message = NULL;
    }
    if (off_message != NULL) {
        free(off_message);
        off_message = NULL;
    }
    last_state = NONE;
    idx = NONE;
    if (target != NULL) {
        free(target);
        target = NULL;
    }
}

void Alert::load(aJsonObject *msg, int index){
    //Init new Alert from aJSON
    //extract the trigger from ajson using
    //aJsonObject* msg = aJson.getObjectItem(root, "trigger");

    init();
    idx = index;
    int json_strlen;

    aJsonObject * cnfobj = aJson.getObjectItem(msg, "on_message");
    if (cnfobj->type == aJson_String)  {
        json_strlen = strnlen(cnfobj->valuestring, ALARM_STR_MAXSIZE);
        on_message = (char *) malloc(json_strlen + 1);
        if (on_message == NULL) {
            Serial.println(F("OOM on alert load (on_message)"));
        } else {
            strlcpy(on_message, cnfobj->valuestring, json_strlen +1);
        }
    }

    cnfobj = aJson.getObjectItem(msg, "off_message");
    if (cnfobj->type == aJson_String)  {
        json_strlen = strnlen(cnfobj->valuestring, ALARM_STR_MAXSIZE);
        off_message = (char *) malloc(json_strlen + 1);
        if (off_message == NULL) {
            Serial.println(F("OOM on alert load (off_message)"));
        } else {
            strlcpy(off_message, cnfobj->valuestring, json_strlen +1);
        }
    }

    cnfobj = aJson.getObjectItem(msg, "target");
    if (cnfobj->type == aJson_String)  {
        json_strlen = strnlen(cnfobj->valuestring, ALARM_STR_MAXSIZE);
        target = (char *) malloc(json_strlen + 1);
        if (target == NULL) {
            Serial.println(F("OOM on alert load (target)"));
        } else {
            strlcpy(target, cnfobj->valuestring, json_strlen +1);

        }
    }

    cnfobj = aJson.getObjectItem(msg, "trigger");
    if (cnfobj && cnfobj->type == aJson_Int) {
        trigger = cnfobj->valueint;
    }
}

int Alert::process_alert(int trigger_state){
    if (last_state != trigger_state) {

#ifdef DEBUG_TRIGGERS
        Serial.print(F("Alarm - triger "));
        Serial.print(trigger);
        Serial.print(F(" changed state"));
        Serial.println(trigger_state);
#endif
        last_state = trigger_state;
        send_message();
    }
    return last_state;
}


int Alert::send_message() {
#ifdef DEBUG_TRIGGERS
        Serial.print(F("Alarm target "));
        Serial.print(target);
#endif

    if (strchr(target, '@') != NULL) {
#ifdef DEBUG_TRIGGERS
        Serial.print(F("Sending mail"));
#endif
        //send mail
        int size;
        char subject[32];
        char * body;
        char * line_end;
        if (last_state == S_OFF) {
            body = off_message;
        } else {
            body = on_message;
        }
        size = 32;
        line_end = strchrnul(body, '\r');
        if ((line_end - body) <  size) size = line_end - body;
        line_end = strchrnul(body, '\n');
        if ((line_end - body) <  size) size = line_end - body;

        strlcpy(subject, body, size);  // copy first line of body to subject
        send_mail(target, subject, body);
        return 0;
    } else {
        //send SMS
        return 1;
    }
}

int Alert::tick() {
    if (last_state == NONE) {
        if (trigger == -2)
            last_state = ups_level < config.ups_trigger_level;
        else
            last_state = triggers[trigger].state;
        return NONE;
    }
    if (trigger == -2) {
        last_state = process_alert(ups_level < config.ups_trigger_level);
    } else {
        last_state = process_alert(triggers[trigger].state);
    }
}

aJsonObject * Alert::json(aJsonObject *cnfdata){
    //exports settings as aJson object into msg

    aJson.addStringToObject(cnfdata, "on_message", on_message);
    aJson.addStringToObject(cnfdata, "off_message", off_message);
    aJson.addStringToObject(cnfdata, "target", target);
    aJson.addNumberToObject(cnfdata, "trigger", trigger);

    return cnfdata;
}

int alert_load(aJsonObject * cfile, int alert_no) {
    alerts[alert_no].load(cfile, alert_no);
}

int alert_save(Alert alerts[], int idx){
    char fname[] = "XX.jso";

    sprintf(fname, "%i.jso", idx);
    aJsonObject *msg = aJson.createObject();
#ifdef DEBUG_TRIGGERS
    Serial.print(F("Preparing json "));
    Serial.println(idx, DEC);
#endif
    alerts[idx].json(msg);
#ifdef DEBUG_TRIGGERS
    Serial.println(F("saving"));
#endif
    file_write("/alerts", fname, msg);
    aJson.deleteItem(msg);
}

int alerts_save(Alert alerts[]){
#ifdef DEBUG_TRIGGERS
        Serial.println(F("Save alerts"));
#endif
        for (int i=0; i < ALERTS; i++) {
            alert_save(alerts, i);
        }
#ifdef DEBUG_TRIGGERS
                Serial.println(F("Saved."));
#endif
}

int alerts_load(Alert alerts[]){
    char fname[] = "XX.jso";

    for (int i=0; i < ALERTS; i++) {
        alerts[i].idx = i;
        sprintf(fname, "%i.jso", i);
        aJsonObject * cfile = file_read("/alerts", fname);
        if (cfile != NULL) {
            alert_load(cfile, i);
            aJson.deleteItem(cfile);
        }
    }
    return ALERTS;
}
