#include "GrowduinoFirmware.h"

#include "outputs.h"


Output::Output() {
    common_init();
}

Output::Output(aJsonObject * json) {
    common_init();
    aJsonObject * data;
    data = file_read("data", "sensor.jso");
    if (data) {
        states.load(data);
        aJson.deleteItem(data);
    } else {
        Serial.println("Unable to read sensor log data");
    }
};

void Output::common_init(){
    sensor_state = 0x00;
    states = RingBuffer(60, "min");
    strcpy(name, "outputs");
}

int Output::get(int slot){
    int bit;
    bit = !!(sensor_state & ( 1 << slot));
    return bit;
}

int Output::set(int slot, int state){
    if (state) {
        //set
        sensor_state |= 1 << slot;
    } else {
        //clear
        sensor_state &= ~(1 << slot);
    }
    return hw_update(slot);
}

int Output::set_delayed(int slot, int state){
    if (state) {
        //set
        sensor_state |= 1 << slot;
    } else {
        //clear
        sensor_state &= ~(1 << slot);
    }
}

int Output::flip(int slot){
    sensor_state ^= 1 << slot;
    return hw_update(slot);
}

int Output::hw_update(int slot){
    // this is only place where we touch hardware
    int bit;
    bit = get(slot);
    digitalWrite(RELAY_START + slot, bit);
    return bit;
}

bool Output::match(const char * request){
    // return true if we want this logger in http request
    char * filename;
    char filebuf[13];

    filename = strrchr(request, '/');

    if (filename == NULL) {
    // something weird happened, we should have / in request
    // as we are sensors/sensor.jso
        return false;
    }

    // we can get weird things in filename
    // otoh name is safe, so make filename from name and compare that
    filename = filename + 1;
    sprintf(filebuf, "%s.jso", name);

    return (strcasecmp(filename, filebuf) == 0);
}

aJsonObject * Output::json_dynamic(){
    // create json with unwinded buffers (all values, most recent last)
    aJsonObject *msg = aJson.createObject();
    // aJson.addNumberToObject(msg, "time", (double) time);
    aJson.addStringToObject(msg, "name", name);
    msg = states.json_dynamic(msg);
    return msg;
}

void Output::log(){
    states.store(sensor_state, minute());
}
