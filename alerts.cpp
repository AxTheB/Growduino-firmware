#include "GrowduinoFirmware.h"


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
    //extract the ajson from ajson using
    //aJsonObject* msg = aJson.getObjectItem(root, "trigger");

    init();
    idx = index;
    int json_strlen;

    char * buffer;
    buffer = (char *) malloc(BUFSIZE);

    aJsonObject * cnfobj = aJson.getObjectItem(msg, "on_message");
    if (cnfobj->type == aJson_String)  {
        json_strlen = strnlen(cnfobj->valuestring, ALARM_STR_MAXSIZE);
        on_message = (char *) malloc(json_strlen + 1);
        if (buffer == NULL) {
            Serial.println(F("OOM on alert load (on_message)"));
        } else {
            strlcpy(on_message, cnfobj->valuestring, json_strlen +1);

        }
    }

    cnfobj = aJson.getObjectItem(msg, "off_message");
    if (cnfobj->type == aJson_String)  {
        json_strlen = strnlen(cnfobj->valuestring, ALARM_STR_MAXSIZE);
        off_message = (char *) malloc(json_strlen + 1);
        if (buffer == NULL) {
            Serial.println(F("OOM on alert load (off_message)"));
        } else {
            strlcpy(off_message, cnfobj->valuestring, json_strlen +1);

        }
    }

    cnfobj = aJson.getObjectItem(msg, "target");
    if (cnfobj->type == aJson_String)  {
        json_strlen = strnlen(cnfobj->valuestring, ALARM_STR_MAXSIZE);
        target = (char *) malloc(json_strlen + 1);
        if (buffer == NULL) {
            Serial.println(F("OOM on alert load (target)"));
        } else {
            strlcpy(target, cnfobj->valuestring, json_strlen +1);

        }
    }
}

aJsonObject * Alert::json(aJsonObject *cnfdata){
    //exports settings as aJson object into msg

    aJson.addStringToObject(cnfdata, "on_message", on_message);
    aJson.addStringToObject(cnfdata, "off_message", off_message);
    aJson.addStringToObject(cnfdata, "target", target);


    return cnfdata;
}

