#include "GrowduinoFirmware.h"
#include "co2.h"
extern Config config;

int CO2_read_raw(){
    int raw_data;
    raw_data = analogReadAvg(CO2_DATA);
    return raw_data;
}

int CO2_read(){
    int raw_data;
    float voltage;
    int co2 = MINVALUE;
#ifdef USE_CO2_SENSOR
    // http://www.veetech.org.uk/Prototype_CO2_Monitor.htm
    float v400ppm = (float) config.co2_400 / 204.6;
    float v40000ppm = (float) config.co2_40k / 204.6;
    float deltavs = v400ppm - v40000ppm;
    float A = deltavs/(log10(400) - log10(40000));
    float B = log10(400);

    raw_data = CO2_read_raw();

    if (raw_data > ADC_CUTOFF) {
        return MINVALUE;
    }
    voltage = raw_data / 204.6;

    float power = ((voltage - v400ppm)/A) + B;
    float co2ppm = pow(10,power) / 10;
    co2 = (int) co2ppm;
#endif

#ifdef DEBUG_CALIB
    if (co2 != MINVALUE){
        SERIAL.print(F("CO2 raw: "));
        SERIAL.println(raw_data);
    }
#endif

    return co2;
}
