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

#include <OneWire.h>

#include <avr/pgmspace.h>

#include <LiquidCrystal.h>

LiquidCrystal lcd(LCD_RESET, LCD_ENABLE, LCD_D1, LCD_D2, LCD_D3, LCD_D4);

int ether = 1;

// DHT22 temp and humidity sensor. Treated as main temp and humidity source
DHT22 myDHT22(DHT22_PIN);

// OneWire
OneWire ds(ONEWIRE_PIN);
byte temp1_addr[8];
byte temp2_addr[8];

//profiling
unsigned long t_loop_start;
unsigned long t_1;
unsigned long t_2;
unsigned long t_3;

Logger dht22_temp = Logger("Temp1");

Logger dht22_humidity = Logger("Humidity");
// light sensor on analog A15
#define LIGHT_SENSOR_PIN 15
Logger light_sensor = Logger("Light");

Logger ultrasound = Logger("Usnd");

Logger onewire_temp1 = Logger("Temp2");
Logger onewire_temp2 = Logger("Temp3");

Config config;
Output outputs;

aJsonStream serial_stream(&Serial);

//LiquidCrystal lcd(8,9,4,5,6,7);

EthernetServer server(80);

Logger * loggers[LOGGERS] = {&dht22_humidity, &dht22_temp, &light_sensor, &ultrasound, &onewire_temp1, &onewire_temp2};

Trigger triggers[TRIGGERS];

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void pFreeRam() {
    Serial.print(F("Free ram: "));
    Serial.println(freeRam());
}

aJsonObject * status(){
    aJsonObject * msg = aJson.createObject();
    char buffer[21];
    int freeram = freeRam();
    aJson.addItemToObject(msg, "free_ram", aJson.createItem(freeram));
    aJson.addItemToObject(msg, "sensors", aJson.createItem(LOGGERS));
    aJson.addItemToObject(msg, "outputs", aJson.createItem(OUTPUTS));
    aJsonObject * logger_list = aJson.createObject();
    for (int i=0; i<LOGGERS; i++){
        sprintf(buffer, "%d", i);
        aJson.addItemToObject(logger_list, buffer, aJson.createItem(loggers[i]->name));
    }
    aJson.addItemToObject(msg, "sensor_list", logger_list);
    aJson.addItemToObject(msg, "triggers", aJson.createItem(TRIGGERS));
    aJson.addItemToObject(msg, "triggers_log_size", aJson.createItem(LOGSIZE));
    sprintf(buffer, "%ld", millis() / 1000);
    aJson.addItemToObject(msg, "uptime", aJson.createItem(buffer));
    aJson.addItemToObject(msg, "tz", aJson.createItem(timeZone));
    aJson.addItemToObject(msg, "daymin", aJson.createItem(daymin()));
    digitalClockDisplay(buffer);
    aJson.addItemToObject(msg, "time", aJson.createItem(buffer));


    return msg;
}

void setup(void) {
    pinMode(13, OUTPUT);
    // start serial port
    Serial.begin(115200);
    Serial.println(F("Grow!"));
    lcd_setup();
    pFreeRam();

    delay(1000);

    Serial.println(F("SD Card init"));
    sdcard_init();
    char index[] = "/INDEX.HTM";
    if (SD.exists(index)) {
        Serial.println(F("SD Card OK"));
    } else {
        Serial.println(F("INDEX.HTM not found. Not going on"));
        int q = 0;
        while (!SD.exists(index)) {
            digitalWrite(13, q);
            q = 1 - q;
            delay(600);
        }
    }
    digitalWrite(13, LOW);
    Serial.println(F("Inititalising Ethernet"));

    // load config from sdcard
    aJsonObject * cfile = file_read("", "config.jso");
    if (cfile != NULL) {
        Serial.println(F("Loading config"));
        config.load(cfile);
        aJson.deleteItem(cfile);
    } else {
        Serial.println(F("Using default config"));
        config.save();
    }
    if (config.use_dhcp == 1) {
        Ethernet.begin(config.mac);
    } else {
        Ethernet.begin(config.mac, config.ip, config.gateway, config.netmask);
    }
    server.begin();
    Serial.print(F("server is at "));
    Serial.println(Ethernet.localIP());

    // load triggers if available
    Serial.println(freeRam());
    triggers_load(triggers, loggers);
    Serial.println(freeRam());
    triggers_save(triggers);
    Serial.println(freeRam());

    // start real time clock
    Serial.println(F("Initialising clock"));
    daytime_init();

    // find ds temp sensor addresses
    ds.reset_search();
    ds.search(temp1_addr);
    ds.search(temp2_addr);

    //load data from sd card
    dht22_temp.load();
    dht22_humidity.load();
    light_sensor.load();

    //initialise outputs
    for(int i=0; i <8; i++) {
        pinMode(RELAY_START + i, OUTPUT);
        // outputs.set(i, 0);
    }

    Serial.println(F("Wasting time (2s)"));
    delay(2000);
    // init temp/humidity logger
    myDHT22.readData();
    Serial.print(F("DHT22 Sensor - temp: "));
    Serial.print(myDHT22.getTemperatureCInt());
    Serial.print(F(" humidity: "));
    Serial.println(myDHT22.getHumidityInt());
    Serial.print(F("Light sensor: "));
    Serial.println(analogRead(LIGHT_SENSOR_PIN));
}

void worker(){
    digitalWrite(13, HIGH);
    //read sensors and store data
    myDHT22.readData();
    dht22_temp.timed_log(myDHT22.getTemperatureCInt());
    dht22_humidity.timed_log(myDHT22.getHumidityInt());
    light_sensor.timed_log(map(analogRead(LIGHT_SENSOR_PIN), 0, 1024, 0, 1000));
    ultrasound.timed_log(ultrasound_ping(USOUND_TRG, USOUND_ECHO));

    onewire_temp1.timed_log(ds_read(ds, temp1_addr));
    onewire_temp2.timed_log(ds_read(ds, temp2_addr));

    #ifdef DEBUG_LOGGERS
    int numLogers = sizeof(loggers) / sizeof(Logger *);
    Serial.print(F("# of loggers: "));
    Serial.println(numLogers, DEC);

    for(int i=0; i < numLogers; i++) {
        Serial.print(loggers[i]->name);
        Serial.print(F(": "));
        aJsonObject *msg = loggers[i]->json();
        aJson.print(msg, &serial_stream);
        aJson.deleteItem(msg);
        Serial.println();
    }
    #endif

#ifdef DEBUG_OUTPUT
    pFreeRam();
    aJsonObject *msg = outputs.json();
    aJson.print(msg, &serial_stream);
    aJson.deleteItem(msg);
    Serial.println(); 
    pFreeRam();
#endif


    // tick triggers
    for(int i=0; i < TRIGGERS; i++) {
#ifdef DEBUG_TRIGGERS
        Serial.print(F("Trigger "));
        Serial.println(i);
        aJsonObject *msg = aJson.createObject();
        msg = triggers[i].json(msg);
        aJson.print(msg, &serial_stream);
        aJson.deleteItem(msg);
        Serial.println("");
#endif

        triggers[i].tick();
    }

    // move relays
    for(int i=0; i <8; i++) {
        outputs.hw_update(i);
    }

    outputs.log();
    lcd_flush();
    char lcd_msg[17];
    sprintf(lcd_msg, "T: %d.%dC", dht22_temp.peek() / 10, dht22_temp.peek() % 10);
    lcd_publish(lcd_msg);

    sprintf(lcd_msg, "H: %d.%d%%", dht22_humidity.peek() / 10, dht22_humidity.peek() % 10);
    lcd_publish(lcd_msg);
    digitalWrite(13, LOW);
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
    Serial.print(F("Returning "));
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
    Serial.print(F("Request: "));
    Serial.println(request);
    if (strncasecmp(request, "sensors", 7) == 0) {
        Serial.println(F("Sensor area"));
        if (outputs.match(request)) {  // outputs
            found = true;
            send_headers(client, request, 30);
            aJsonStream eth_stream(&client);
            aJsonObject *msg = outputs.json();
            aJson.print(msg, &eth_stream);
            aJson.deleteItem(msg);
            return 1;
        }
        for (int i = 0; i < LOGGERS; i++) {
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
        if (strcasecmp(request, "sensors/status.jso") == 0) {
            found = true;
            send_headers(client, request, 30);
            aJsonStream eth_stream(&client);
            aJsonObject *msg = status();
            aJson.print(msg, &eth_stream);
            aJson.deleteItem(msg);
            return 1;
        }
        if (!found)
            responseNum(client, 404);
        return 0;
    }
    Serial.println(F("File area"));
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
        remain = min(remain, BUFSIZE -1);
        dataFile.read(clientline, remain);
        clientline[remain] = 0;
        client.write(clientline);
    };
    dataFile.close();
    return 1;
}

int fn_extract_trg(char * request){
    int trg = -1;
    sscanf(request, "triggers/%d.jso", &trg);
    if (trg >= TRIGGERS) trg = -1;
    return trg;
}

void pageServe(EthernetClient client){
    char request[33];
    char clientline[BUFSIZE];
    int linesize;
    bool is_url;

    request[0] = 0;
    int post;
    post = 0;

    t_1 = millis();
    while (client.connected()) {
        if (client.available()) {
            linesize = client.readBytesUntil('\n', clientline, BUFSIZE-1);
            clientline[linesize] = '\0';
            #ifdef DEBUG_HTTP
            Serial.println(clientline);
            #endif

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
                Serial.println(F("Processing post request"));
                is_url = true;
            };
            if (is_url) {
                if (strlcpy(request, extract_filename(clientline), 32) >= 32){
                    Serial.print(F("Filename too long: "));
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
            trigger_save(triggers, trgno);
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
    lcd_tick();
    EthernetClient client = server.available();
    if (client) {
        pageServe(client);
        Serial.println(F("times:"));
        Serial.println(t_1 - t_loop_start);
        Serial.println(t_2 - t_loop_start);
        Serial.println(t_3 - t_loop_start);
        Serial.println(millis() - t_loop_start);
        pFreeRam();
    }
    delay(50);
}
