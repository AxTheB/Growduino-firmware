#include "GrowduinoFirmware.h"

#include <DHT22.h>

#include <Wire.h>
#include "RTClib.h"
#include <stdio.h>

#include "Logger.h"
#include <aJSON.h>
#include "daytime.h"

#include <SD.h>
#include "sdcard.h"

#include <SPI.h>
#include <Ethernet.h>
#include <string.h>
#include <stdio.h>


// DHT22 temp and humidity sensor. Treated as main temp and humidity source
#define DHT22_PIN 23
DHT22 myDHT22(DHT22_PIN);
Logger dht22_temp = Logger("Temp1");
Logger dht22_humidity = Logger("Humidity");
// light sensor on analog A15
#define LIGHT_SENSOR_PIN 15
Logger light_sensor = Logger("Light");

aJsonStream serial_stream(&Serial);

EthernetServer server(80);

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
    char index[] = "/INDEX.HTM";
    if (SD.exists(index)) {
        Serial.println("Card ok");
    } else {
        Serial.println("INDEX.HTM not found");
    }
    Serial.println("Initialising eth");
    byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x55, 0x44};
    Ethernet.begin(mac);  // use dhcp
    server.begin();
    Serial.print("server is at ");
    Serial.println(Ethernet.localIP());

    //load data from sd card
    dht22_temp.load();
    dht22_humidity.load();
    light_sensor.load();

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

    //cteni ze sensoru
    pinMode(13, OUTPUT);
    myDHT22.readData();
    dht22_temp.timed_log(myDHT22.getTemperatureCInt());
    dht22_humidity.timed_log(myDHT22.getHumidityInt());
    light_sensor.timed_log(analogRead(LIGHT_SENSOR_PIN));

    //vypis na seriak
    Serial.println(day_seconds() / 60.0);

    Serial.print("dht22_temp=");
    aJsonObject *msg = dht22_temp.json();
    //Serial.println(aJson.print(dht22_temp.l1.json()));
    aJson.print(msg, &serial_stream);
    aJson.deleteItem(msg);
    Serial.println();
    Serial.print("dht22_humidity=");
    msg = dht22_humidity.json();
    //Serial.println(aJson.print(dht22_humidity.l1.json()));
    aJson.print(msg, &serial_stream);
    aJson.deleteItem(msg);
    Serial.println();
    Serial.print("light_sensor=");
    msg = light_sensor.json();
    //Serial.println(aJson.print(light_sensor.l1.json()));
    aJson.print(msg, &serial_stream);
    aJson.deleteItem(msg);
    Serial.println();
}

const char * get_filename_ext(const char *filename){
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        return "";
    }
    return dot + 1;
};


const char * getContentType(char * filename) {
    //odhadne mimetype
    char ext[4];

    strncpy(ext, get_filename_ext(filename), 3);

    //preved priponu na mala
    for (char *p = ext ; *p; ++p) *p = tolower(*p);

    if (strcmp(ext, "htm") == 0)
        return "Content-Type: text/html";
    if (strcmp(ext, "jso") == 0)
        return "Content-Type: application/json";
    if (strcmp(ext, "js") == 0)
        return "Content-Type: text/javascript";
    if (strcmp(ext, "css") == 0)
        return "Content-Type: text/css";
    if (strcmp(ext, "png") == 0)
        return "Content-Type: image/png";
    if (strcmp(ext, "jpg") == 0)
        return "Content-Type: image/jpeg";
    return "Content-Type: text/plain";
}

void pageServe(EthernetClient client){
    char filename[13];

    //based on WebServer example
    boolean currentLineIsBlank = true;
    while (client.connected()) {
        if (client.available()) {
            char c = client.read();
            Serial.write(c);
            // if you've gotten to the end of the line (received a newline
            // character) and the line is blank, the http request has ended,
            // so you can send a reply
            if (c == '\n' && currentLineIsBlank) {
                // send a standard http response header
                client.println("HTTP/1.1 200 OK");

                client.println(getContentType(filename));
                } else {
                client.println("Content-Type: text/html");
                client.println("Connection: close");  // the connection will be closed after completion of the response
                client.println();
                client.println("<!DOCTYPE HTML>");
                client.println("<html>");
                client.println("</html>");
                break;
            }
            if (c == '\n') {
                // you're starting a new line
                currentLineIsBlank = true;
            }
            else if (c != '\r') {
                // you've gotten a character on the current line
                currentLineIsBlank = false;
                // here will be logic that determines what to send

            }
        }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
}

void loop(void){
    if (dht22_temp.available()) {
        worker();
    }
    EthernetClient client = server.available();
    if (client) {
        pageServe(client);
    }
    digitalWrite(13, 1 - digitalRead(13));
}
