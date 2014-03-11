#include "GrowduinoFirmware.h"

#include "trigger.h"

#include "outputs.h"
extern Output outputs;

Trigger::Trigger(){
    init();
}

void Trigger::init(){
    t_since = NONE;
    t_until = NONE;
    on_value = 0;
    off_value = 0;
    on_cmp = '-';
    off_cmp = '-';
    important = false;
    sensor = NONE;
    output = NONE;
    _logger = NULL;
    idx = NONE;
}

void Trigger::load(aJsonObject *msg, Logger * loggers[], int index){
    //Init new trigger from aJSON
    //extract the ajson from ajson using
    //aJsonObject* msg = aJson.getObjectItem(root, "trigger");

    init();
    if (idx != NONE && output != NONE) {
        Serial.print(F("Clean up trigger "));
        Serial.println(idx, DEC);
        outputs.revive(output, idx);
        outputs.set(output, 0, idx);
    }
    idx = index;


    aJsonObject * cnfobj = aJson.getObjectItem(msg, "t_since");
    if (cnfobj && cnfobj->type == aJson_Int) {
        t_since = cnfobj->valueint;
    }

    cnfobj = aJson.getObjectItem(msg, "t_until");
    if (cnfobj && cnfobj->type == aJson_Int) {
        t_until = cnfobj->valueint;
    }

    cnfobj = aJson.getObjectItem(msg, "on_value");
    if (cnfobj) {
        if (cnfobj->type == aJson_Int) {
            on_value = cnfobj->valueint;
            on_cmp = '>';
        } else if (cnfobj->type == aJson_String)  {
            on_value = atoi(cnfobj->valuestring + 1);
            on_cmp = cnfobj->valuestring[0];

            if (strchr(cnfobj->valuestring, '!') != NULL)
                important = true;

        } else {
            on_value = MINVALUE;
        }
    }

    cnfobj = aJson.getObjectItem(msg, "off_value");
    if (cnfobj) {
        if (cnfobj->type == aJson_Int) {
            off_value = cnfobj->valueint;
            off_cmp = '<';
        } else if (cnfobj->type == aJson_String)  {
            off_value = atoi(cnfobj->valuestring + 1);
            off_cmp = cnfobj->valuestring[0];

            if (strchr(cnfobj->valuestring, '!') != NULL)
                important = true;

        } else {
            off_value = MINVALUE;
        }
    }

    cnfobj = aJson.getObjectItem(msg, "sensor");
    if (cnfobj && cnfobj->type == aJson_Int) {
        sensor = cnfobj->valueint;
        if (sensor < -1 || sensor > LOGGERS)
            sensor = NONE;
    } else {
        sensor = NONE;
    }

    cnfobj = aJson.getObjectItem(msg, "output");
    if (cnfobj && cnfobj->type == aJson_Int) {
        output = cnfobj->valueint;
        if (output < -1 || output > OUTPUTS)
            output = NONE;

    } else {
        output = NONE;
    }

    if (sensor >=0 && sensor < LOGGERS) {
        _logger = loggers[sensor];
    } else {
        _logger = NULL;
    }
    outputs.revive(output, idx);
}


aJsonObject * Trigger::json(aJsonObject *cnfdata){
    //exports settings as aJson object into msg

    char val_buf[6];

    aJson.addNumberToObject(cnfdata, "t_since", t_since);
    aJson.addNumberToObject(cnfdata, "t_until", t_until);

    sprintf(val_buf, "%c%d", on_cmp, on_value);

    aJson.addStringToObject(cnfdata, "on_value", val_buf);

    if (important) {
        sprintf(val_buf, "%c%d!", off_cmp, off_value);
    } else {
        sprintf(val_buf, "%c%d", off_cmp, off_value);
    }

    aJson.addStringToObject(cnfdata, "off_value", val_buf);
    aJson.addNumberToObject(cnfdata, "sensor", sensor);
    aJson.addNumberToObject(cnfdata, "output", output);

    return cnfdata;
}

int Trigger::tick(){
    //int l_daymin = minute() + 60 * hour();
    int l_daymin = daymin();
    int sensor_val;
#ifdef DEBUG_TRIGGERS
    Serial.print(F("Ticking "));
    Serial.print(idx);
    Serial.print(F(" at time "));
    Serial.println(l_daymin);
#endif

    if (output == NONE) {
#ifdef DEBUG_TRIGGERS
        Serial.println(F("Skipping trigger (no output)"));
#endif
        return false;
    }


    // if t_since == -1 run all day.
    // if t_since > t_until run over midnight
    
    if (t_since == -1) {
        Serial.println(F("All day trigger"));
    }
    if (t_since <= l_daymin && t_until > l_daymin) {
        Serial.println(F("Hit normal trigger"));
    }
    if ((t_since > t_until) && (t_since <= l_daymin || t_until > l_daymin)) {
        Serial.println(F("Hit overnight trigger"));
    }

    if ((t_since == -1) ||
            (t_since <= l_daymin && t_until > l_daymin) ||
            ((t_since > t_until) && (t_since <= l_daymin || t_until > l_daymin))
       ) {
#ifdef DEBUG_TRIGGERS
        Serial.print(F("time ok: "));
        Serial.println(l_daymin);
#endif
        if (_logger == NULL || sensor == NONE) {  // if there is no logger defined use minvalue (for unconditional time triggers)
            sensor_val = MINVALUE;
        } else {    // the logger is defined
            sensor_val = _logger->peek();
        }
        //outputs.set(output, 0, idx);
#ifdef DEBUG_TRIGGERS
        Serial.print(F("Evaluating ON condition ("));
        Serial.print(sensor_val);
        Serial.print(on_cmp);
        Serial.print(on_value);
        Serial.print(F("):"));
#endif
        switch (on_cmp) {
            case '<':
                if (sensor_val < on_value || sensor == NONE) {
                    outputs.set(output, 1, idx);
                    if (important) outputs.revive(output, idx);
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
                if (sensor_val > on_value || sensor == NONE) {
                    if (important) {
                        outputs.revive(output, idx);
                    } else {
                        outputs.set(output, 1, idx);
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
                Serial.println(outputs.uptime(output));
                Serial.print(F("output state "));
                Serial.println(outputs.hw_get(output));
#endif
                if ((outputs.hw_get(output) == 0) && (outputs.uptime(output) >= on_value * 60)) {
                    if (important) {
                        outputs.revive(output, idx);
                    } else {
                        outputs.set(output, 1, idx);
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
        Serial.print(off_cmp);
        Serial.print(off_value);
        Serial.print(F("):"));
#endif
        switch (off_cmp) {
            case '<':
                if (sensor_val <= off_value && sensor != NONE) {
                    if (important) {
                        outputs.kill(output, idx);
                    } else {
                        outputs.set(output, 0, idx);
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
                if (sensor_val >= off_value && sensor != NONE) {
                    if (important) {
                        outputs.kill(output, idx);
                    } else {
                        outputs.set(output, 0, idx);
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
                Serial.println(outputs.uptime(output));
                Serial.print(F("output state "));
                Serial.println(outputs.get(output));
#endif

                if ((outputs.hw_get(output) == 1) && (outputs.uptime(output) >= off_value * 60)) {
                    if (important) {
                        outputs.kill(output, idx);
                    } else {
                        outputs.set(output, 0, idx);
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

    } else {
        //default out-of-time state is disabled, not broken
        outputs.set(output, 0, idx);
        if (important) outputs.revive(output, idx);

#ifdef DEBUG_TRIGGERS
        Serial.print(F("Wrong time: "));
        Serial.println(l_daymin);
#endif
        return false;
    }
}


int trigger_load(Trigger triggers[], Logger * loggers[], aJsonObject * cfile, int trgno) {
    triggers[trgno].load(cfile, loggers, trgno);
}

int triggers_load(Trigger triggers[], Logger * loggers[]){
    char fname[] = "XX.jso";

    for (int i=0; i < TRIGGERS; i++) {
        triggers[i].idx = i;
        sprintf(fname, "%i.jso", i);
        aJsonObject * cfile = file_read("/triggers", fname);
        if (cfile != NULL) {
            trigger_load(triggers, loggers, cfile, i);
            aJson.deleteItem(cfile);
        }
    }
    return TRIGGERS;
}

int triggers_save(Trigger triggers[]){
    Serial.println(F("Save trigers"));
    for (int i=0; i < TRIGGERS; i++) {
        trigger_save(triggers, i);
    }
#ifdef DEBUG_TRIGGERS
    Serial.println(F("Saved."));
#endif
}


int trigger_save(Trigger triggers[], int idx){

    char fname[] = "XX.jso";

    sprintf(fname, "%i.jso", idx);
    aJsonObject *msg = aJson.createObject();
#ifdef DEBUG_TRIGGERS
    Serial.print(F("Preparing json "));
    Serial.println(idx, DEC);
#endif
    triggers[idx].json(msg);
#ifdef DEBUG_TRIGGERS
    Serial.println(F("saving"));
#endif
    file_write("/triggers", fname, msg);
    aJson.deleteItem(msg);
}

