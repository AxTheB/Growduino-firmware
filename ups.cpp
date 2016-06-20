#include "GrowduinoFirmware.h"


int ups_init(){
    Serial3.begin(19200);
}

int ups_read(){
    char line[64];
    int state;
    int energy;
    int counter;
    // = Serial3.readBytesUntil('s', line, 64);
    counter = Serial3.readBytesUntil('\n', line, 64);  //throw away first line, as it may not be complete
    strcpy(line, "--------------------------------");

    counter = Serial3.readBytesUntil('\n', line, 64);

    line[counter]='\0';
#ifdef DEBUG_UPS
    Serial.println(line);
#endif
    sscanf(line, "s:%i e:%i", &state, &energy);
    if (energy < 0 || energy > 100) {
        energy = MINVALUE;
    }
    return energy;

}
