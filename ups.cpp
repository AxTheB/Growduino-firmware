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
    //counter = Serial3.readBytesUntil('\n', line, 64);  //throw away first line, as it may not be complete

    if (Serial3.available()) {
        strcpy(line, "................................................................");

        counter = Serial3.readBytesUntil('\n', line, 64);
#ifdef DEBUG_UPS
        SERIAL.print("Reading UPS: ");
        SERIAL.println(line);
#endif

        if (line[0] != 's') { // UPS stats start with 's'
            // UPS error handling may come here
#ifdef DEBUG_UPS
            SERIAL.println("Wrong line start");
#endif
            return MINVALUE;
        }

        line[counter]='\0';
        sscanf(line, "s:%i e:%i", &state, &energy);
        if (energy < 0 || energy > 100) {
#ifdef DEBUG_UPS
            SERIAL.println("Value out of bounds");
#endif
            energy = MINVALUE;
        }
#ifdef DEBUG_UPS
        SERIAL.println("Value OK");
#endif
        ups_level = state;
    } else {
#ifdef DEBUG_UPS
        SERIAL.println("Serial buffer empty");
        delay(1000);
        energy = MINVALUE;
    }
#endif


    return energy;
}

int ups_read(){
    int retval;
    for (int i = 0; i < 10; i++) {
#ifdef DEBUG_UPS
        SERIAL.print("UPS reading try #");
        SERIAL.println(i+1);
#endif
        retval = ups_read_inner();
        if (retval != MINVALUE) {
            return retval;
        }
    }
    return MINVALUE;
}
