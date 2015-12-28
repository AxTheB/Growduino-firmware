#include "GrowduinoFirmware.h"
#include "ec.h"
extern Config config;

void ec_enable(){
#ifdef USE_EC_SENSOR
    pinMode(EC_DATA, INPUT);
    pinMode(EC_ENABLE, OUTPUT);
#endif
}

int ec_read(){
    int ec = MINVALUE;
#ifdef USE_EC_SENSOR
    long lowPulseTime = 0;
    long highPulseTime = 0;
    long pulseTime;
    float ec_a, ec_b;

    float c_low = 1.278;
    float c_high = 4.523;

    ec_a =  (c_high - c_low) / (config.ec_high_ion - config.ec_low_ion);
    ec_b = c_low - ec_a * config.ec_low_ion;


    digitalWrite(EC_ENABLE, HIGH); // power up the sensor
    delay(100);

    for(unsigned int j=0; j<EC_SAMPLE_TIMES; j++){
        highPulseTime+=pulseIn(EC_DATA, HIGH);
        if (j == 0 and highPulseTime == 0)
            return MINVALUE;
        lowPulseTime+=pulseIn(EC_DATA, LOW);
    }
    lowPulseTime = lowPulseTime/EC_SAMPLE_TIMES;
    highPulseTime = highPulseTime/EC_SAMPLE_TIMES;

    pulseTime = (lowPulseTime + highPulseTime)/2;

    ec = (int) 100 * (pulseTime * ec_a + ec_b);

#endif

#ifdef DEBUG_CALIB
    if (ec != MINVALUE) {
        Serial.print("EC pulse: ");
        Serial.println(pulseTime);
    }
#endif
    return ec;
}
