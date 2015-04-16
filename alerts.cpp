#include "GrowduinoFirmware.h"
#include <string.h>
extern Alert alerts[];
extern Output outputs;
extern Trigger triggers[];
extern int ups_level;
extern Config config;

void alert_load_trigger(int idx, aJsonObject *msg){
    // load trigger number
    aJsonObject * cnfobj = aJson.getObjectItem(msg, "trigger");
    if (cnfobj && cnfobj->type == aJson_Int) {
        alerts[idx].trigger = cnfobj->valueint;
    } else {
        alerts[idx].trigger = NONE;
    }
    alerts[idx].last_state = NONE;
}

void alert_load(aJsonObject *msg, char * on_message, char * off_message, char * target){
    // load strings from json
    int json_strlen;

    aJsonObject * cnfobj = aJson.getObjectItem(msg, "on_message");
    if (cnfobj->type == aJson_String)  {
        json_strlen = strnlen(cnfobj->valuestring, ALARM_STR_MAXSIZE);
        json_strlen = max(json_strlen, ALERT_MSG_LEN-1);
        strlcpy(on_message, cnfobj->valuestring, json_strlen +1);
    }

    cnfobj = aJson.getObjectItem(msg, "off_message");
    if (cnfobj->type == aJson_String)  {
        json_strlen = strnlen(cnfobj->valuestring, ALARM_STR_MAXSIZE);
        json_strlen = max(json_strlen, ALERT_MSG_LEN-1);
        strlcpy(off_message, cnfobj->valuestring, json_strlen +1);
    }

    cnfobj = aJson.getObjectItem(msg, "target");
    if (cnfobj->type == aJson_String)  {
        json_strlen = strnlen(cnfobj->valuestring, ALARM_STR_MAXSIZE);
        strlcpy(target, cnfobj->valuestring, json_strlen +1);
    }
}

int process_alert(int idx, int trigger_state){
    //pokud se zmenil prislusny trigger, posli alert
    if (alerts[idx].last_state != trigger_state) {
    #ifdef DEBUG_ALERTS
        Serial.print(F("Alarm ("));
        Serial.print(idx);
        Serial.print(F(") changed state to: "));
        Serial.println(trigger_state);
    #endif
        alerts[idx].last_state = trigger_state;
        alert_send_message(idx);
    }
    return alerts[idx].last_state;
}

int alert_send_message(int idx) {
    // send the alert message
    char target[ALERT_TARGET_LEN];
    char on_message[ALERT_MSG_LEN];
    char off_message[ALERT_MSG_LEN];
    //load config from sd
    char fname[] = "XX.jso";

    sprintf(fname, "%i.jso", idx);
    aJsonObject * cfile = file_read("/alerts", fname);
    alert_load(cfile, on_message, off_message, target);

    aJson.deleteItem(cfile);
    Serial.println(alerts[idx].last_state);

    #ifdef DEBUG_ALERTS
    Serial.print(F("Alarm target "));
    Serial.println(target);
    #endif

    if (strchr(target, '@') != NULL) {
    #ifdef DEBUG_ALERTS
        Serial.print(F("Sending mail"));
    #endif
        //send mail
        int size;
        char subject[32];
        char * body;
        char * line_end;
        Serial.print(F("Last state: "));
        Serial.println(alerts[idx].last_state);
        if (alerts[idx].last_state == STATE_OFF) {
            body = off_message;
            #ifdef DEBUG_ALERTS
            Serial.println(F("Sending off message"));
            #endif
        } else {
            body = on_message;
            #ifdef DEBUG_ALERTS
            Serial.println(F("Sending on message"));
            #endif
        }
        Serial.println(body);
        size = 32;
        line_end = strchrnul(body, '\r');
        if ((line_end - body) <  size) size = line_end - body;
        line_end = strchrnul(body, '\n');
        if ((line_end - body) <  size) size = line_end - body;

        strlcpy(subject, body, size);  // copy first line of body to subject
        pFreeRam();
        send_mail(target, subject, body);
        pFreeRam();
        return 0;
    }
    return 1;
}

int alert_tick(int idx) {
    if (alerts[idx].trigger != NONE) {
        if (alerts[idx].last_state == NONE) {
            if (alerts[idx].trigger == -2)
                alerts[idx].last_state = (ups_level < config.ups_trigger_level);
            else
                alerts[idx].last_state = triggers[idx].state;
            return NONE;
        }
        if (alerts[idx].trigger == -2) {
            alerts[idx].last_state = process_alert(idx, ups_level < config.ups_trigger_level);
        } else {
            alerts[idx].last_state = process_alert(idx, triggers[alerts[idx].trigger].state);
        }
        return alerts[idx].last_state;
    }
    return NONE;
}

void alert_json(int idx, Stream * cnfdata, char * on_message, char * off_message, char * target){
    //writes object data to stream

    cnfdata->print(F("{"));
    cnfdata->print(F("\"on_message\":\""));
    cnfdata->print(on_message);
    cnfdata->print(F("\", \"off_message\":\""));
    cnfdata->print(off_message);
    cnfdata->print(F("\", \"target\":\""));
    cnfdata->print(target);
    cnfdata->print(F("\", \"trigger\":"));
    cnfdata->print(alerts[idx].trigger, DEC);
    cnfdata->print(F("}"));
}

void alert_passthru(int idx, Stream * source_stream){
    char fname[] = "XX.jso";

    sprintf(fname, "%i.jso", idx);
    file_passthru("/alerts", fname, source_stream);
}

int alerts_load(){
    // on restart, load all alerts.
    //char target[ALERT_TARGET_LEN];
    //char on_message[ALERT_MSG_LEN];
    //char off_message[ALERT_MSG_LEN];
    char fname[] = "XX.jso";

    for (int i=0; i < ALERTS; i++) {
        sprintf(fname, "%i.jso", i);
        aJsonObject * cfile = file_read("/alerts", fname);
        if (cfile != NULL) {
            alert_load_trigger(i, cfile);
            aJson.deleteItem(cfile);
        } else {
            alerts[i].trigger = NONE;
            alerts[i].last_state = NONE;
        }
    }
    return ALERTS;
}
