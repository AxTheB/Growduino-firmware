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
    states = RingBuffer(60, "sns");
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

int Output::flip(int slot){
    sensor_state ^= 1 << slot;
    return hw_update(slot);
}

int Output::hw_update(int slot){
    int bit;
    bit = get(slot);
    digitalWrite(RELAY_START + slot, bit);
    return bit;
}
