#include "GrowduinoFirmware.h"

#include <OneWire.h>

//byte addr[8];
byte data[12];
//byte present = 0;
byte type_s;
int celsius;

int ds_read(OneWire ds, byte * addr){
    if (!ds.search(addr)) {
        ds.reset_search();
        return MINVALUE;
    }

    ds.reset_search();
    if (addr[0] == 0) {
        type_s = 1;
    } else {
        type_s = 0;
    }
    ds.reset();
    ds.select(addr);
    ds.write(0x44, 1);        // start conversion, with parasite power on at the end
    delay(1000);
    ds.reset();
    ds.select(addr);
    ds.write(0xBE);
    for (int i = 0; i < 9; i++) {           // we need 9 bytes
        data[i] = ds.read();
    }
    int16_t raw = (data[1] << 8) | data[0];
    if (type_s) {
        raw = raw << 3; // 9 bit resolution default
        if (data[7] == 0x10) {
            // "count remain" gives full 12 bit resolution
            raw = (raw & 0xFFF0) + 12 - data[6];
        }
    } else {
        byte cfg = (data[4] & 0x60);
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
        //// default is 12 bit resolution, 750 ms conversion time
    }
    celsius = raw * 10 / 16;
    return celsius;
}
