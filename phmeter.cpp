#include "GrowduinoFirmware.h"
#include "phmeter.h"
extern Config config;

int PH_read(){

    int ph_4_val = 4;
    int ph_7_val = 7.03;
    int ph_4 = config.ph_4;
    int ph_7 = config.ph_7;
    float slope = (float) (ph_7 - ph_4) / (ph_7_val - ph_4_val);
    float pH = ph_4_val + ((analogReadAvg(PH_DATA) - ph_4)/slope);

    return (int) 100 * pH;
}
