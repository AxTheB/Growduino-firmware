#include "GrowduinoFirmware.h"
#include "phmeter.h"

extern Config config;

int PH_read(){

    int ph_4_val = 4;
    int ph_7_val = 7.03;
    int ph_4 = config.ph_4;
    int ph_7 = config.ph_7;
    int raw_data = analogReadAvg(PH_DATA);
    float slope = (float) (ph_7 - ph_4) / (ph_7_val - ph_4_val);
    float pH = ph_4_val + ((raw_data - ph_4)/slope);
#ifdef DEBUG_CALIB
    Serial.print("pH sensor raw: ");
    Serial.println(raw_data);
#endif

    return (int) 100 * pH;
}
