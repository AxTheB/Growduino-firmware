#include "GrowduinoFirmware.h"

#include "trigger.h"

#include "outputs.h"
extern Output outputs;

Trigger::Trigger(){
    t_since = 0;
    t_until = 0;
    on_value = 255;
    off_value = 512;
    lt = true;
    sensor = -1;
    output = 7;
    _logger = NULL;
    idx = 0;
}

void Trigger::load(aJsonObject *msg, Logger * loggers[]){
    //Init new trigger from aJSON
    //extract the ajson from ajson using
    //aJsonObject* msg = aJson.getObjectItem(root, "trigger");

    aJsonObject * cnfobj = aJson.getObjectItem(msg, "t_since");
    if (cnfobj && cnfobj->type == aJson_Int) {
        t_since = cnfobj->valueint;
    } else {
        t_since = -1;
    }

    cnfobj = aJson.getObjectItem(msg, "t_until");
    if (cnfobj && cnfobj->type == aJson_Int) {
        t_until = cnfobj->valueint;
    } else {
        t_until = -1;
    }

    cnfobj = aJson.getObjectItem(msg, "on_value");
    if (cnfobj && cnfobj->type == aJson_Int) {
        on_value = cnfobj->valueint;
    } else {
        on_value = -1;
    }

    cnfobj = aJson.getObjectItem(msg, "off_value");
    if (cnfobj && cnfobj->type == aJson_Int) {
        off_value = cnfobj->valueint;
    } else {
        off_value = -1;
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

    cnfobj = aJson.getObjectItem(msg, "lt");
    if (cnfobj && cnfobj->type == aJson_False) {
        lt = false;
    } else {
        lt= true;
    }

    if (sensor != -1) {
        _logger = loggers[sensor];
    } else {
        _logger = NULL;
    }
}

aJsonObject * Trigger::json(aJsonObject *cnfdata){
    //exports settings as aJson object into msg

    aJson.addNumberToObject(cnfdata, "t_since", t_since);
    aJson.addNumberToObject(cnfdata, "t_until", t_until);
    aJson.addNumberToObject(cnfdata, "on_value", on_value);
    aJson.addNumberToObject(cnfdata, "off_value", off_value);
    aJson.addNumberToObject(cnfdata, "sensor", sensor);
    aJson.addNumberToObject(cnfdata, "output", output);
    if (lt) {
        aJson.addTrueToObject(cnfdata, "lt");
    } else {
        aJson.addFalseToObject(cnfdata, "lt");
    }
}

void Trigger::tick(){
    int sensor_val = _logger->peek();
    int daymin = minute() * 60 + second();
    Serial.print("Ticking ");
    Serial.println(idx);

    // if t_since == t_until run all day.
    // if t_since > t_until run over midnight

    if (((t_since == t_until) && (t_since == 0)) ||
            (t_since <= daymin && t_until > daymin) ||
            (t_since >= daymin && t_until < daymin)
       ) {
        Serial.print("time ok");
        Serial.println(daymin);
        if (_logger == NULL) {  // if there is no logger defined just keep it on/off
            if (lt) {
                outputs.set_delayed(output, 1);
                Serial.print(output);
                Serial.print(" to 1 ");
                Serial.println("On always");
            } else {
                outputs.set_delayed(output, 0);
                Serial.print(output);
                Serial.print(" to 0 ");
                Serial.println("Off always");
            }
        } else {    // the logger is defined, time is right.
            if (lt) {   // if reading is lower than on_value, we should be on
                if (sensor_val <= on_value) {
                    outputs.set_delayed(output, 1);
                    Serial.print(output);
                    Serial.print(" to 1 ");
                    Serial.print("lt; s_val <= on ");
                    Serial.print(sensor_val);
                    Serial.print(" ");
                    Serial.println(on_value);

                } else if (sensor_val >= off_value) {
                    outputs.set_delayed(output, 0);
                    Serial.print(output);
                    Serial.print(" to 0 ");
                    Serial.print("lt; s_val >= off ");
                    Serial.print(sensor_val);
                    Serial.print(" ");
                    Serial.println(off_value);
                }
            } else {
                if (sensor_val >= on_value) {
                    outputs.set_delayed(output, 1);
                    Serial.print(output);
                    Serial.print(" to 1 ");
                    Serial.print("gt; s_val >= on ");
                    Serial.print(sensor_val);
                    Serial.print(" ");
                    Serial.println(on_value);
                } else if (sensor_val <= off_value) {
                    outputs.set_delayed(output, 0);
                    Serial.print(output);
                    Serial.print(" to 0 ");
                    Serial.print("gt; s_val <= off ");
                    Serial.print(sensor_val);
                    Serial.print(" ");
                    Serial.println(off_value);
                }
            }
        }
    } else if ((t_until == daymin) && (_logger == NULL) && (t_until != t_since)) {
        // if there is no logger defined and the trigger had duration, switch off at the end.
        if (lt) {
            outputs.set_delayed(output, 1);
            Serial.print(output);
            Serial.print(" to 1 ");
            Serial.println("Trg end 1");
        } else {
            outputs.set_delayed(output, 0);
            Serial.print(output);
            Serial.print(" to 0");
            Serial.println("Trg end 0");
        }
    }
}

int trigger_load(Trigger triggers[], Logger * loggers[], aJsonObject * cfile, int trgno){
    char trgname[] = "triggerXX";
    sprintf(trgname, "trigger%i", trgno);
    Serial.println(trgname);
    aJsonObject * msg = aJson.getObjectItem(cfile, trgname);
    if (msg != NULL) {
        triggers[trgno].load(msg, loggers);
    }
}

int triggers_load(Trigger triggers[], Logger * loggers[]){
    char fname[] = "/triggers/XX.jso";
    for (int i=0; i < TRIGGERS; i++) {
        triggers[i].idx = i;
        sprintf(fname, "/triggers/%i.jso", i);
        aJsonObject * cfile = file_read("", fname);
        if (cfile != NULL) {
            trigger_load(triggers, loggers, cfile, i);
            aJson.deleteItem(cfile);
        }
    }
    return TRIGGERS;
}

int triggers_save(Trigger triggers[]){

    char trgname[10];
    aJsonObject * cnfdata;

    aJsonObject * msg = aJson.createObject();

    if (msg != NULL) {
        for (int i=0; i < TRIGGERS; i++) {
            sprintf(trgname, "trigger%i", i);
            aJson.addItemToObject(msg, trgname, cnfdata = aJson.createObject());
            triggers[i].json(cnfdata);
        }
    } else {
        return -1;
    }
    file_write("", "trigger.jso", msg);
    aJson.deleteItem(msg);
}
