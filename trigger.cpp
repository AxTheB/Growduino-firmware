#include "GrowduinoFirmware.h"

#include "trigger.h"

#include "outputs.h"
extern Output outputs;
extern Trigger triggers[];


int trigger_tick(int idx){
    //int l_daymin = minute() + 60 * hour();
    int l_daymin = daymin();
    int sensor_val;
#ifdef DEBUG_TRIGGERS
    Serial.print(F("Ticking "));
    Serial.print(idx);
    Serial.print(F(" at time "));
    Serial.println(l_daymin);
#endif

    int time_ok = 0;

    if (triggers[idx].t_since == -1) {
        Serial.println(F("All day trigger"));
        time_ok = 1;
    }
    if (triggers[idx].t_since <= l_daymin && triggers[idx].t_until > l_daymin) {
        Serial.println(F("Hit normal trigger"));
        time_ok = 1;
    }
    if ((triggers[idx].t_since > triggers[idx].t_until) && (triggers[idx].t_since <= l_daymin || triggers[idx].t_until > l_daymin)) {
        Serial.println(F("Hit overnight trigger"));
        time_ok = 1;
    }

    if (time_ok == 1) {
#ifdef DEBUG_TRIGGERS
        Serial.print(F("time ok: "));
        Serial.println(l_daymin);
#endif
        if (triggers[idx]._logger == NULL || triggers[idx].sensor == NONE) {  // if there is no logger defined use minvalue (for unconditional time triggers)
            sensor_val = MINVALUE;
        } else {    // the logger is defined
            sensor_val = triggers[idx]._logger->peek();
        }
        //outputs.set(output, 0, idx);
#ifdef DEBUG_TRIGGERS
        Serial.print(F("Evaluating ON condition ("));
        Serial.print(sensor_val);
        Serial.print(triggers[idx].on_cmp);
        Serial.print(triggers[idx].on_value);
        Serial.print(F("):"));
#endif
        switch (triggers[idx].on_cmp) {
            case '<':
                if (sensor_val < triggers[idx].on_value || triggers[idx].sensor == NONE) {
                    if (triggers[idx].important) {
                        outputs.revive(triggers[idx].output, idx);
                    } else {
                        triggers[idx].state = S_ON;
                        outputs.set(triggers[idx].output, 1, idx);
                    }
#ifdef DEBUG_TRIGGERS
                    Serial.println(F("Hit on <"));
#endif
                } else {
#ifdef DEBUG_TRIGGERS
                    Serial.println(F("Miss on <"));
#endif
                }
                break;
            case '>':
                if (sensor_val > triggers[idx].on_value || triggers[idx].sensor == NONE) {
                    if (triggers[idx].important) {
                        outputs.revive(triggers[idx].output, idx);
                    } else {
                        outputs.set(triggers[idx].output, 1, idx);
                        triggers[idx].state = S_ON;
                    }
#ifdef DEBUG_TRIGGERS
                    Serial.println(F("Hit on >"));
#endif
                } else {
#ifdef DEBUG_TRIGGERS
                    Serial.println(F("Miss on >"));
#endif
                }
                break;
            case 'T':
            case 't':
#ifdef DEBUG_TRIGGERS
                Serial.print(F("output uptime "));
                Serial.println(outputs.uptime(triggers[idx].output));
                Serial.print(F("output state "));
                Serial.println(outputs.hw_get(triggers[idx].output));
#endif
                if ((outputs.hw_get(triggers[idx].output) == 0) && (outputs.uptime(triggers[idx].output) >= (time_t) (triggers[idx].on_value * 60) - 1)) {
                    if (triggers[idx].important) {
                        outputs.revive(triggers[idx].output, idx);
                    } else {
                        outputs.set(triggers[idx].output, 1, idx);
                        triggers[idx].state = S_ON;
                    }
#ifdef DEBUG_TRIGGERS
                    Serial.println(F("Hit on T"));
#endif
                } else {
#ifdef DEBUG_TRIGGERS
                    Serial.println(F("Miss on T"));
#endif
                }

                break;
        }
#ifdef DEBUG_TRIGGERS
        Serial.print(F("Evaluating OFF condition ("));
        Serial.print(sensor_val);
        Serial.print(triggers[idx].off_cmp);
        Serial.print(triggers[idx].off_value);
        Serial.print(F("):"));
#endif
        switch (triggers[idx].off_cmp) {
            case '<':
                if (sensor_val <= triggers[idx].off_value && triggers[idx].sensor != NONE) {
                    if (triggers[idx].important) {
                        outputs.kill(triggers[idx].output, idx);
                    } else {
                        outputs.set(triggers[idx].output, 0, idx);
                        triggers[idx].state = S_OFF;
                    }
#ifdef DEBUG_TRIGGERS
                    Serial.println(F("Hit on <"));
#endif
                } else {
#ifdef DEBUG_TRIGGERS
                    Serial.println(F("Miss on <"));
#endif
                }
                break;
            case '>':
                if (sensor_val >= triggers[idx].off_value && triggers[idx].sensor != NONE) {
                    if (triggers[idx].important) {
                        outputs.kill(triggers[idx].output, idx);
                    } else {
                        outputs.set(triggers[idx].output, 0, idx);
                        triggers[idx].state = S_OFF;
                    }
#ifdef DEBUG_TRIGGERS
                    Serial.println(F("Hit on >"));
#endif
                } else {
#ifdef DEBUG_TRIGGERS
                    Serial.println(F("Miss on >"));
#endif
                }
                break;
            case 'T':
            case 't':
#ifdef DEBUG_TRIGGERS
                Serial.print(F("output uptime "));
                Serial.println(outputs.uptime(triggers[idx].output));
                Serial.print(F(" output state "));
                Serial.println(outputs.get(triggers[idx].output));
#endif

                if ((outputs.hw_get(triggers[idx].output) == 1) && (outputs.uptime(triggers[idx].output) >= (time_t) (triggers[idx].off_value * 60) - 1)) {
                    if (triggers[idx].important) {
                        outputs.kill(triggers[idx].output, idx);
                    } else {
                        outputs.set(triggers[idx].output, 0, idx);
                        triggers[idx].state = S_OFF;
                    }
#ifdef DEBUG_TRIGGERS
                    Serial.print(F("Hit on T: state: "));
                    Serial.print(outputs.hw_get(triggers[idx].output));
                    Serial.print(F(" uptime: "));
                    Serial.print(outputs.uptime(triggers[idx].output));
                    Serial.print(F(" off value:"));
                    Serial.println(triggers[idx].off_value * 60);
#endif
                } else {
#ifdef DEBUG_TRIGGERS
                    Serial.print(F("Miss on T: state: "));
                    Serial.print(outputs.hw_get(triggers[idx].output));
                    Serial.print(F(" uptime: "));
                    Serial.print(outputs.uptime(triggers[idx].output));
                    Serial.print(F(" off value:"));
                    Serial.println(triggers[idx].off_value * 60);

#endif
                }
                break;
        }

    } else {
        trigger_set_default_state(idx);
#ifdef DEBUG_TRIGGERS
        Serial.print(F("Wrong time: "));
        Serial.println(l_daymin);
#endif
        return false;
    }
    return true;
}

void trigger_init(int idx){
    triggers[idx].t_since = NONE;
    triggers[idx].t_until = NONE;
    triggers[idx].on_value = 0;
    triggers[idx].off_value = 0;
    triggers[idx].on_cmp = '-';
    triggers[idx].off_cmp = '-';
    triggers[idx].important = false;
    triggers[idx].sensor = NONE;
    triggers[idx].output = NONE;
    triggers[idx]._logger = NULL;
    triggers[idx].state = S_OFF;
}

void trigger_load(int idx, aJsonObject *msg, Logger * loggers[]){
    //Init new trigger from aJSON
    //extract the ajson from ajson using
    //aJsonObject* msg = aJson.getObjectItem(root, "trigger");
    if (idx != NONE && triggers[idx].output != NONE) {
        Serial.print(F("Clean up trigger "));
        Serial.println(idx, DEC);
        trigger_set_default_state(idx);
    }
    trigger_init(idx);

    aJsonObject * cnfobj = aJson.getObjectItem(msg, "t_since");
    if (cnfobj && cnfobj->type == aJson_Int) {
        triggers[idx].t_since = cnfobj->valueint;
    }

    cnfobj = aJson.getObjectItem(msg, "t_until");
    if (cnfobj && cnfobj->type == aJson_Int) {
        triggers[idx].t_until = cnfobj->valueint;
    }

    cnfobj = aJson.getObjectItem(msg, "on_value");
    if (cnfobj) {
        if (cnfobj->type == aJson_Int) {
            triggers[idx].on_value = cnfobj->valueint;
            triggers[idx].on_cmp = '>';
        } else if (cnfobj->type == aJson_String)  {
            triggers[idx].on_value = atoi(cnfobj->valuestring + 1);
            triggers[idx].on_cmp = cnfobj->valuestring[0];

            if (strchr(cnfobj->valuestring, '!') != NULL)
                triggers[idx].important = true;

        } else {
            triggers[idx].on_value = MINVALUE;
        }
    }

    cnfobj = aJson.getObjectItem(msg, "off_value");
    if (cnfobj) {
        if (cnfobj->type == aJson_Int) {
            triggers[idx].off_value = cnfobj->valueint;
            triggers[idx].off_cmp = '<';
        } else if (cnfobj->type == aJson_String)  {
            triggers[idx].off_value = atoi(cnfobj->valuestring + 1);
            triggers[idx].off_cmp = cnfobj->valuestring[0];

            if (strchr(cnfobj->valuestring, '!') != NULL)
                triggers[idx].important = true;

        } else {
            triggers[idx].off_value = MINVALUE;
        }
    }

    cnfobj = aJson.getObjectItem(msg, "sensor");
    if (cnfobj && cnfobj->type == aJson_Int) {
        triggers[idx].sensor = cnfobj->valueint;
        if (triggers[idx].sensor < -1 || triggers[idx].sensor > LOGGERS)
            triggers[idx].sensor = NONE;
    } else {
        triggers[idx].sensor = NONE;
    }

    cnfobj = aJson.getObjectItem(msg, "output");
    if (cnfobj && cnfobj->type == aJson_Int) {
        triggers[idx].output = cnfobj->valueint;
        if (triggers[idx].output < -1 || triggers[idx].output > OUTPUTS)
            triggers[idx].output = NONE;

    } else {
        triggers[idx].output = NONE;
    }

    if (triggers[idx].sensor >=0 && triggers[idx].sensor < LOGGERS) {
        triggers[idx]._logger = loggers[triggers[idx].sensor];
    } else {
        triggers[idx]._logger = NULL;
    }
    outputs.revive(triggers[idx].output, idx);
}

void trigger_set_default_state(int idx){
        //default out-of-time state is disabled, not broken
        outputs.set(triggers[idx].output, 0, idx);
        triggers[idx].state = S_OFF;
        if (triggers[idx].important) outputs.revive(triggers[idx].output, idx);
}

int triggers_load(Trigger triggers[], Logger * loggers[]){
    char fname[] = "XX.jso";

    for (int i=0; i < TRIGGERS; i++) {
        sprintf(fname, "%i.jso", i);
        aJsonObject * cfile = file_read("/triggers", fname);
        if (cfile != NULL) {
            trigger_load(i, cfile, loggers);
            aJson.deleteItem(cfile);
        } else {
            trigger_init(i);
        }
    }
    return TRIGGERS;
}

void triggers_save(Trigger triggers[]){
    Serial.println(F("Save trigers"));
    for (int i=0; i < TRIGGERS; i++) {
        trigger_save(triggers, i);
    }
#ifdef DEBUG_TRIGGERS
    Serial.println(F("Saved."));
#endif
}

void trigger_save(Trigger triggers[], int idx){
    char fname[] = "XX.jso";

    sprintf(fname, "%i.jso", idx);
    file_for_write("/triggers", fname, &sd_file);
#ifdef DEBUG_TRIGGERS
    Serial.print(F("Preparing json "));
    Serial.println(idx, DEC);
#endif
    trigger_json(idx, &sd_file);
#ifdef DEBUG_TRIGGERS
    Serial.println(F("saving"));
#endif
    sd_file.close();
}


void trigger_json(int idx, Stream * cnfdata){
    //exports settings as aJson object into msg

    char val_buf[6];

    cnfdata->print(F("{"));

    cnfdata->print(F("\"t_since\":"));
    cnfdata->print(triggers[idx].t_since);
    cnfdata->print(F(","));
    cnfdata->print(F("\"t_until\":"));
    cnfdata->print(triggers[idx].t_until);
    cnfdata->print(F(","));

    sprintf(val_buf, "%c%d", triggers[idx].on_cmp, triggers[idx].on_value);

    cnfdata->print(F("\"on_value\":\""));
    cnfdata->print(val_buf);
    cnfdata->print(F("\","));

    if (triggers[idx].important) {
        sprintf(val_buf, "%c%d!", triggers[idx].off_cmp, triggers[idx].off_value);
    } else {
        sprintf(val_buf, "%c%d", triggers[idx].off_cmp, triggers[idx].off_value);
    }

    cnfdata->print(F("\"off_value\":\""));
    cnfdata->print(val_buf);
    cnfdata->print(F("\","));

    cnfdata->print(F("\"sensor\":"));
    cnfdata->print(triggers[idx].sensor);
    cnfdata->print(F(","));

    cnfdata->print(F("\"output\":"));
    cnfdata->print(triggers[idx].output);
    cnfdata->print(F("}"));
}
