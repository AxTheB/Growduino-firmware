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

// #include <OneWire.h>

#include <avr/pgmspace.h>

int ether = 1;


// DHT22 temp and humidity sensor. Treated as main temp and humidity source
#define DHT22_PIN 23
DHT22 myDHT22(DHT22_PIN);

//profiling
unsigned long t_loop_start;
unsigned long t_1;
unsigned long t_2;
unsigned long t_3;


const char message_1[] PROGMEM="Grow!";
const char message_2[] PROGMEM="SDcard init";
const char message_3[] PROGMEM="SD card OK";
const char message_4[] PROGMEM="INDEX.HTM not found. Not going on";
const char message_5[] PROGMEM="Inititalising Ethernet";
const char message_6[] PROGMEM="Loading config";

PROGMEM const char * messages[]={
    message_1,
    message_2,
    message_3,
    message_4,
    message_5,
    message_6,
};

void Serprintln(int seq){
    char * buffer;
    buffer = (char *) malloc(40);
    if (buffer != NULL) {
        strcpy_P(buffer, (char * ) pgm_read_word(&(messages[seq - 1])));
        Serial.println(buffer);
        free(buffer);
    } else {
        Serial.println("malloc fail");
    }
}

Logger dht22_temp = Logger("Temp1");

Logger dht22_humidity = Logger("Humidity");
// light sensor on analog A15
#define LIGHT_SENSOR_PIN 15
Logger light_sensor = Logger("Light");

Logger ultrasound = Logger("Ultrasound");

Logger onewire_temp1 = Logger("Temp2");
//Logger onewire_temp2 = Logger("Temp3");

Config config;
Output outputs;

aJsonStream serial_stream(&Serial);

EthernetServer server(80);

Logger * loggers[] = {&dht22_humidity, &dht22_temp, &light_sensor, &ultrasound};
//, &onewire_temp1};
//, &onewire_temp2};

Trigger triggers[TRIGGERS];

int loggers_no = 3;

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void pFreeRam() {
    Serial.print("Free ram: ");
    Serial.println(freeRam());
}

void setup(void) {
    pinMode(13, OUTPUT);
    // start serial port
    Serial.begin(115200);
    Serial.println("!");
    Serprintln(1); //grow!
    pFreeRam();
    /*
        int q = 0;
        while (true) {
            digitalWrite(13, q);
            q = 1 - q;
            delay(600);
        }
    */
    delay(1000);
    // serial_buffer = (char*) malloc (1024 * sizeof(int));
    Serprintln(2); //sd card init
    sdcard_init();
    char index[] = "/INDEX.HTM";
    if (SD.exists(index)) {
        Serprintln(3); //SD card OK
    } else {
        Serprintln(4); // INDEX.HTM not found. Not going on
        int q = 0;
        while (true) {
            digitalWrite(13, q);
            q = 1 - q;
            delay(600);
        }
    }
    Serprintln(5); //initialising eth

    // load config from sdcard
    aJsonObject * cfile = file_read("", "config.jso");
    if (cfile != NULL) {
        Serprintln(6);
        config.load(cfile);
        aJson.deleteItem(cfile);
    } else {
        Serial.println("Using default config");
        config.save();
    }
    if (config.use_dhcp == 1) {
        Ethernet.begin(config.mac);
    } else {
        Ethernet.begin(config.mac, config.ip, config.gateway, config.netmask);
    }
    server.begin();
    Serial.print("server is at ");
    Serial.println(Ethernet.localIP());

    // load triggers if available
    Serial.println(freeRam());
    triggers_load(triggers, loggers);
    Serial.println(freeRam());
    triggers_save(triggers);
    Serial.println(freeRam());

    // start real time clock
    Serial.println("Initialising clock");
    daytime_init();

    //load data from sd card
    dht22_temp.load();
    dht22_humidity.load();
    light_sensor.load();

    //initialise outputs
    for(int i=0; i <8; i++) {
        pinMode(RELAY_START + i, OUTPUT);
        outputs.set(i, 0);
    }

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
    //read sensors and store data
    pinMode(13, OUTPUT);
    myDHT22.readData();
    dht22_temp.timed_log(myDHT22.getTemperatureCInt());
    dht22_humidity.timed_log(myDHT22.getHumidityInt());
    light_sensor.timed_log(map(analogRead(LIGHT_SENSOR_PIN), 0, 1024, 0, 1000));

    Serial.print("dht22_temp=");
    aJsonObject *msg = dht22_temp.json();
    aJson.print(msg, &serial_stream);
    aJson.deleteItem(msg);
    Serial.println();
    Serial.print("dht22_humidity=");
    msg = dht22_humidity.json();
    aJson.print(msg, &serial_stream);
    aJson.deleteItem(msg);
    Serial.println();
    Serial.print("light_sensor=");
    msg = light_sensor.json();
    aJson.print(msg, &serial_stream);
    aJson.deleteItem(msg);
    Serial.println();

    outputs.log();

    // tick triggers
    for(int i=0; i < TRIGGERS; i++) {
        triggers[i].tick();
    }

    // move relays
    for(int i=0; i <8; i++) {
        outputs.hw_update(i);
    }

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
    // returns pointer to filename in request
    // dont forget to copy it before overwriting line
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
    if (strstr(line, "POST /") != 0) {
        char *filename;
        filename = line + 6;
        // convert space before HTTP to null, trimming the string
        (strstr(line, " HTTP"))[0] = 0;
        return filename;
    }
    return NULL;
}


void send_headers(EthernetClient client, char * request, int age) {
    responseNum(client, 200);
    client.println(getContentType(request));
    client.println("Access-Control-Allow-Origin: *");
    client.print("cache-control: max-age=");
    client.println(age, DEC);
    client.println("Connection: close");
    client.println();
}


int senddata(EthernetClient client, char * request, char * clientline){
    // Send response
    bool found = false;
    Serial.print("Request: ");
    Serial.println(request);
    if (strncasecmp(request, "sensors", 7) == 0) {
        Serial.println("Sensor area");
        if (outputs.match(request)) {  // outputs
            found = true;
            send_headers(client, request, 30);
            aJsonStream eth_stream(&client);
            aJsonObject *msg = outputs.json_dynamic();
            aJson.print(msg, &eth_stream);
            aJson.deleteItem(msg);
            return 1;
        }
        for (int i = 0; i < loggers_no; i++) {
            if (loggers[i]->match(request)){  // sensors
                found = true;
                send_headers(client, request, 30);
                aJsonStream eth_stream(&client);
                aJsonObject *msg = loggers[i]->json_dynamic();
                aJson.print(msg, &eth_stream);
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
    send_headers(client, request, 600);

    // client.println(getContentType(request));
    File dataFile = SD.open(request, FILE_READ);
    // abuse clientline as sd buffer
    int remain = 0;
    while ((remain = dataFile.available())) {
        remain = min(remain, BUFSIZ -1);
        dataFile.read(clientline, remain);
        clientline[remain] = 0;
        client.write(clientline);
    };
    dataFile.close();
    return 1;
}

int fn_extract_trg(char * request){
    char * rq;
    int trg = -1;
    sscanf(request, "triggers/%d.jso", &trg);
    if (trg >= TRIGGERS) trg = -1;
    return trg;
}

void pageServe(EthernetClient client){
    char request[33];
    char clientline[BUFSIZ];
    int index;
    int linesize;
    bool is_url;

    index = 0;
    request[0] = 0;
    int post;
    post = 0;

    t_1 = millis();
    while (client.connected()) {
        if (client.available()) {
            linesize = client.readBytesUntil('\n', clientline, BUFSIZ-1);
            clientline[linesize] = '\0';
            Serial.println(clientline);

            // the line is blank, the http request has ended,
            // so you can send a reply
            if (linesize <= 1) {
                t_2 = millis();
                if (post) {
                    responseNum(client, 200);
                } else {
                    senddata(client, request, clientline);
                }
                t_3 = millis();

                break;
            }
            is_url = false;
            if (strstr(clientline, "GET /") != 0) {
                is_url = true;
            } else if (strstr(clientline, "POST /") != 0) {
                post = 1;
                Serial.println("Processing post request");
                is_url = true;
            };
            if (is_url) {
                if (strlcpy(request, extract_filename(clientline), 32) >= 32){
                    Serial.print("Filename too long: ");
                    Serial.println(clientline);
                    request[32]='\0';
                }
            }
        }
    }
    // Headers ale all here, if its post we now should read the body.
    if (post) {
        int trgno = fn_extract_trg(request);

        if (strcasecmp(request, "client.jso") == 0) {
            aJsonStream eth_stream(&client);
            aJsonObject * data = aJson.parse(&eth_stream);
            file_write("", "client.jso", data);
            aJson.deleteItem(data);
        } else if (strcasecmp(request, "config.jso") == 0) {
            aJsonStream eth_stream(&client);
            aJsonObject * data = aJson.parse(&eth_stream);
            config.load(data);
            config.save();
            aJson.deleteItem(data);
        } else if (trgno > -1) {
            aJsonStream eth_stream(&client);
            aJsonObject * data = aJson.parse(&eth_stream);
            trigger_load(triggers, loggers, data, trgno);
            triggers_save(triggers);
            aJson.deleteItem(data);
        }
    }

    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
}

void loop(void){
    t_loop_start = millis();
    if (dht22_temp.available()) {
        worker();
        pFreeRam();
    }
    EthernetClient client = server.available();
    if (client) {
        pageServe(client);
        Serial.println("times:");
        Serial.println(t_1 - t_loop_start);
        Serial.println(t_2 - t_loop_start);
        Serial.println(t_3 - t_loop_start);
        Serial.println(millis() - t_loop_start);
        pFreeRam();
    }
    delay(5);

    if (analogRead(LIGHT_SENSOR_PIN) < 250) {
        outputs.set(5, 1);
    } else if (analogRead(LIGHT_SENSOR_PIN) > 550) {
        outputs.set(5, 0);
    }
}
