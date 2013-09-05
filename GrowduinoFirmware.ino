#include "GrowduinoFirmware.h"

#include <DHT22.h>

#include <Wire.h>
#include <Time.h>
#include <stdio.h>

#include <aJSON.h>

#include <SD.h>

#include <SPI.h>
#include <Ethernet.h>
#include <string.h>
#include <stdio.h>
#include <DS1307RTC.h>

int ether = 1;


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

Logger * loggers[] = {&dht22_humidity, &dht22_temp, &light_sensor};

int loggers_no = 3;

void setup(void)
{
    // start serial port
    Serial.begin(115200);
    Serial.println("Grow!");
    delay(1000);
    // serial_buffer = (char*) malloc (1024 * sizeof(int));
    Serial.println("SDcard init");
    sdcard_init();
    char index[] = "/INDEX.HTM";
    if (SD.exists(index)) {
        Serial.println("Card ok");
    } else {
        Serial.println("INDEX.HTM not found. Not going on");
        while (true) {
            digitalWrite(13, 1 - digitalRead(13));
            delay(100);
            }
    }
    Serial.println("Initialising eth");
    byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x55, 0x44};
    IPAddress myaddr(195, 113, 57, 67);
    IPAddress gateway(195, 113, 57, 254);
    IPAddress subnet(255, 255, 255, 0);
    Ethernet.begin(mac, myaddr, gateway, subnet);
    server.begin();
    Serial.print("server is at ");
    Serial.println(Ethernet.localIP());

    // start real time clock
    Serial.println("Initialising clock");
    daytime_init();

    //load data from sd card
    dht22_temp.load();
    dht22_humidity.load();
    light_sensor.load();

    Serial.println("Wasting time (2s)");
    delay(2000);
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
    //Serial.println(day_seconds() / 60.0);

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

    strncpy(ext, get_filename_ext(filename), 4);
    ext[3] = '\0';

    Serial.print("Extension: ");
    Serial.println(ext);

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

void responseNum(EthernetClient client, int code){
    Serial.print("Returning ");
    switch (code){
        case 200:
            client.println("HTTP/1.1 200 OK");
            Serial.println(200);
            break;
        case 404:
            Serial.println(404);
            client.println("HTTP/1.1 404 Not Found");
            client.println("Content-Type: text/html");
            client.println();
            client.println("<h2>File Not Found!</h2>");
            break;
    }

}

const char * extract_filename(char * line){
    // http://www.ladyada.net/learn/arduino/ethfiles.html ripoff
    if (strstr(line, "GET / ") != 0) {
        return "index.htm";
    }
    if (strstr(line, "GET /") != 0) {
        char *filename;
        filename = line + 5; // 'GET /' is 5 chars
        // convert space before HTTP to null, trimming the string
        (strstr(line, " HTTP"))[0] = 0;
        return filename;
    }
    return NULL;
}

int senddata(EthernetClient client, char * request, char * clientline){
    // Send response
    bool found = false;
    if (strncasecmp(request, "sensors", 7) == 0) {
        Serial.println("Sensor area");
        for (int i = 0; i < loggers_no; i++) {
            if (loggers[i]->match(request)){
                Serial.println("Match!");
                found = true;
                responseNum(client, 200);
                client.println(getContentType(request));
                client.println("Access-Control-Allow-Origin: *");
                client.println("cache-control: max-age=30");
                client.println("Connection: close");
                client.println();
                aJsonStream eth_stream(&client);
                Serial.println(loggers[i]->name);
                aJsonObject *msg = loggers[i]->json_dynamic();
                aJson.print(msg, &eth_stream);
                aJson.print(msg, &serial_stream);
                aJson.deleteItem(msg);
                return 1;
            }
        }
        if (!found)
            responseNum(client, 404);
        return 0;
    }
    Serial.println("File area");
    // Find the file
    if (!SD.exists(request)) {
        responseNum(client, 404);
        return 0;
    }

    // Now we know file exists, so serve it

    responseNum(client, 200);

    client.println(getContentType(request));
    client.println("Access-Control-Allow-Origin: *");
    // client.println("cache-control: max-age=86400");
    client.println("cache-control: max-age=600");
    client.println("Connection: close");  // the connection will be closed after completion of the response
    client.println();
    File dataFile = SD.open(request, FILE_READ);
    // abuse clientline as sd buffer
    int remain = 0;
    while ((remain = dataFile.available())) {
        remain = min(remain, BUFSIZ -1);
        dataFile.read(clientline, remain);
        clientline[remain] = 0;
        // Serial.println(clientline);
        client.write(clientline);
        // client.write(dataFile.read());
    };
    dataFile.close();
    return 1;
}

void pageServe(EthernetClient client){
    char request[32];
    char clientline[BUFSIZ];
    int index;
    int action;

    index = 0;
    action = 0;
    request[0] = 0;

    boolean currentLineIsBlank = true;
    while (client.connected()) {
        if (client.available()) {
            char c = client.read();

            // if you've gotten to the end of the line (received a newline
            // character) and the line is blank, the http request has ended,
            // so you can send a reply
            if (c == '\n' && currentLineIsBlank) {
                senddata(client, request, clientline);
                break;
            }
            if (c == '\n') {
                clientline[index] = '\0';
                index = 0;
                Serial.println(clientline);
                if (strstr(clientline, "GET /") != 0) {
                    strcpy(request, extract_filename(clientline));
                    Serial.print("extracted path: ");
                    Serial.println(request);
                }

                // you're starting a new line
                currentLineIsBlank = true;
            }
            else if (c != '\r') {
                // you've gotten a character on the current line
                currentLineIsBlank = false;
                clientline[index] = c;
                index++;
                if (index >= BUFSIZ)
                    index = BUFSIZ -1;
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
