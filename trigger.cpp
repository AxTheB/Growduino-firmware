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
    important = NONE;
    sensor = NONE;
    output = NONE;
    _logger = NULL;
    idx = 0;
}

void Trigger::load(aJsonObject *msg, Logger * loggers[], int index){
    //Init new trigger from aJSON
    //extract the ajson from ajson using
    //aJsonObject* msg = aJson.getObjectItem(root, "trigger");

    init();
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
                important = IMP_ON;

        } else {
            on_value = 0;
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
                important = IMP_OFF;

        } else {
            off_value = 512;
        }
    }

    cnfobj = aJson.getObjectItem(msg, "sensor");
    if (cnfobj && cnfobj->type == aJson_Int) {
        sensor = cnfobj->valueint;
    } else {
        sensor = -1;
    }

    cnfobj = aJson.getObjectItem(msg, "output");
    if (cnfobj && cnfobj->type == aJson_Int) {
        output = cnfobj->valueint;
    } else {
        output = -1;
    }

    /* cnfobj = aJson.getObjectItem(msg, "lt");
    if (cnfobj && cnfobj->type == aJson_False) {
        lt = false;
    } else {
        lt= true;
    }
    */

    if (sensor != -1) {
        _logger = loggers[sensor];
    } else {
        _logger = NULL;
    }
}


aJsonObject * Trigger::json(aJsonObject *cnfdata){
    //exports settings as aJson object into msg

    char val_buf[6];

    aJson.addNumberToObject(cnfdata, "t_since", t_since);
    aJson.addNumberToObject(cnfdata, "t_until", t_until);

    if (important == IMP_ON) {
        sprintf(val_buf, "%c%d!", on_cmp, on_value);
    } else {
        sprintf(val_buf, "%c%d", on_cmp, on_value);
    }

    aJson.addStringToObject(cnfdata, "on_value", val_buf);

    if (important == IMP_OFF) {
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
    int daymin = minute() + 60 * hour();
    int sensor_val;
#ifdef DEBUG_TRIGGERS
    Serial.print(F("Ticking "));
    Serial.println(idx);
#endif

    // if t_since == -1 run all day.
    // if t_since > t_until run over midnight

    if ((t_since == -1) ||
            (t_since <= daymin && t_until > daymin) ||
            (t_since >= daymin && t_until < daymin)
       ) {
#ifdef DEBUG_TRIGGERS
        Serial.print(F("time ok: "));
        Serial.println(daymin);
#endif
        if (_logger == NULL) {  // if there is no logger defined just keep it on/off
            sensor_val = MINVALUE;
        } else {    // the logger is defined, time is right.
            sensor_val = _logger->peek();
        }
        outputs.set(output, 0, idx);
#ifdef DEBUG_TRIGGERS
        Serial.print(F("On value compare: "));
#endif
        switch (on_cmp) {
            case '<':
                if (sensor_val <= on_value) {
                    outputs.set(output, 1, idx);
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
                if (sensor_val >= on_value) {
                    outputs.set(output, 1, idx);
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
                if ((outputs.get(output) == 0) && (outputs.uptime(output) > on_value)) {
                    outputs.set(output, 1, idx);
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
        Serial.println(F("off value compare:"));
        switch (off_cmp) {
            case '<':
                if (sensor_val <= off_value) {
                    outputs.set(output, 0, idx);
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
                if (sensor_val >= off_value) {
                    outputs.set(output, 0, idx);
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
                if ((outputs.get(output) == 1) && (outputs.uptime(output) > off_value)) {
                    outputs.set(output, 0, idx);
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

    char fname[] = "XX.jso";

    for (int i=0; i < TRIGGERS; i++) {
        sprintf(fname, "%i.jso", i);
        aJsonObject *msg = aJson.createObject();
#ifdef DEBUG_TRIGGERS
        Serial.print(F("Preparing json "));
        Serial.println(i, DEC);
#endif
        triggers[i].json(msg);
#ifdef DEBUG_TRIGGERS
        Serial.println(F("saving"));
#endif
        file_write("/triggers", fname, msg);
        aJson.deleteItem(msg);
    }

#ifdef DEBUG_TRIGGERS
    Serial.println(F("Saved."));
#endif
}
