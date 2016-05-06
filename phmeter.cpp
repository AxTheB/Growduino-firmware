#include "GrowduinoFirmware.h"
#include "phmeter.h"

extern Config config;

int PH_read_raw(){
    int raw_data = analogReadAvg(PH_DATA);
    return raw_data;
}

int PH_read(){

    float ph_4_val = 4;
    float ph_7_val = 7.03;
    float ph_4 = (float) config.ph_4;
    float ph_7 = (float) config.ph_7;
    float raw_data = (float) PH_read_raw();
    float slope = (ph_7 - ph_4) / (ph_7_val - ph_4_val);
    float pH = ph_4_val + ((raw_data - ph_4)/slope);
#ifdef DEBUG_CALIB
    Serial.print("pH sensor raw: ");
    Serial.println(raw_data);
#endif

    return (int) 100 * pH;
}
