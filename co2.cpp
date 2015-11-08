#include "GrowduinoFirmware.h"
#include "co2.h"
extern Config config;

int CO2_read(){
    // http://www.veetech.org.uk/Prototype_CO2_Monitor.htm
    float v400ppm = config.co2_400;
    float v40000ppm = config.co2_40k;
    float deltavs = v400ppm - v40000ppm;
    float A = deltavs/(log10(400) - log10(40000));
    float B = log10(400);
    
    float voltage;

    voltage = analogReadAvg(CO2_DATA) / 204.6;
    float power = ((voltage - v400ppm)/A) + B;
    float co2ppm = pow(10,power);
    int co2 = (int) co2ppm;

    return co2;
}
