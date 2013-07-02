#include "GrowduinoFirmware.h"
#include <DHT22.h>

#include <time.h>

#include <Wire.h>
#include "RTClib.h"
#include <stdio.h>

#include "Logger.h"
#include "daytime.h"

#include <SD.h>
#include "sdcard.h"

// DHT22 temp and humidity sensor. Treated as main temp and humidity source
#define DHT22_PIN 23
DHT22 myDHT22(DHT22_PIN);
Logger dht22_temp = Logger("Temp1");
Logger dht22_humidity = Logger("Humidity");
// light sensor on analog A15
#define LIGHT_SENSOR_PIN 15
Logger light_sensor = Logger("Light");


void setup(void)
{
    // start serial port
    Serial.begin(115200);
    Serial.println("Grow!");
    Serial.println("Initialising clock");
    // start real time clock
    daytime_init();
    // serial_buffer = (char*) malloc (1024 * sizeof(int));
    Serial.println("SDcard init");
    sdcard_init();
    if (SD.exists("/INDEX.HTM")) {
        Serial.println("Card ok");
    } else {
        Serial.println("INDEX.HTM not found");
    }
    Serial.println("Wasting time (3s)");
    delay(3000);
    // init temp/humidity logger 
    myDHT22.readData();
    Serial.print("DHT22 Sensor - temp: ");
    Serial.print(myDHT22.getTemperatureC());
    Serial.print(" humidity: ");
    Serial.println(myDHT22.getHumidity());
    Serial.print("Light sensor: ");
    Serial.println(analogRead(LIGHT_SENSOR_PIN));

}

void worker(){
    pinMode(13, OUTPUT);
    myDHT22.readData();
    dht22_temp.timed_log(myDHT22.getTemperatureCInt());
    dht22_humidity.timed_log(myDHT22.getHumidityInt());
    light_sensor.timed_log(analogRead(LIGHT_SENSOR_PIN));
    strcpy(serial_buffer, "");
    dht22_temp.l1.json(serial_buffer);
    Serial.print("dht22_temp=");
    Serial.println(serial_buffer);
    strcpy(serial_buffer, "");
    dht22_humidity.l1.json(serial_buffer);
    Serial.print("dht22_humidity=");
    Serial.println(serial_buffer);
    strcpy(serial_buffer, "");
    light_sensor.l1.json(serial_buffer);
    Serial.print("light_sensor=");
    Serial.println(serial_buffer);
    Serial.println();
}

void loop(void)
{
    if (dht22_temp.available()) {
        worker();
    }
    delay(1000);
    digitalWrite(13, 1 - digitalRead(13));
}
