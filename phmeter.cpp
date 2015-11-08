#include "GrowduinoFirmware.h"
#include "phmeter.h"
extern Config config;

int PH_read(){

    int ph_4 = config.ph_4;
    int ph_7 = config.ph_7;
    float slope = (float) (ph_7 - ph_4) / 3;
    float pH = 4 + ((analogReadAvg(CO2_DATA) - ph_4)/slope);

    return (int) 100 * pH;
}
