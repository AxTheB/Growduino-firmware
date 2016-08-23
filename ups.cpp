#include "GrowduinoFirmware.h"
extern int ups_level;


int ups_init(){
    Serial3.begin(19200);
}

int ups_read_inner(){
    char line[64];
    int state;
    int energy;
    int counter;
    // = Serial3.readBytesUntil('s', line, 64);
    counter = Serial3.readBytesUntil('\n', line, 64);  //throw away first line, as it may not be complete
    strcpy(line, "--------------------------------");

    counter = Serial3.readBytesUntil('\n', line, 64);

    if (line[0] != 's') { // UPS stats start with 's'
        // UPS error handling may come here
        return MINVALUE;
    }

    line[counter]='\0';
#ifdef DEBUG_UPS
    SERIAL.println(line);
#endif
    sscanf(line, "s:%i e:%i", &state, &energy);
    if (energy < 0 || energy > 100) {
        energy = MINVALUE;
    }
    ups_level = state;

    return energy;
}

int ups_read(){
    int retval;
    for (int i = 0; i < 4; i++) {
        retval = ups_read_inner();
        if (retval != MINVALUE) {
            return retval;
        }
    }
    return MINVALUE;
}
