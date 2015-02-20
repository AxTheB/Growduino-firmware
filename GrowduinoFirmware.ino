

#include "GrowduinoFirmware.h"

#include <dht.h>

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

#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>

// #define USE_GSM 1

#ifdef USE_GSM
#include <GSM_Shield.h>
GSM gsm;
#endif


int gsm_init_done = 0;

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

int ether = 1;

// DHT22 temp and humidity sensor. Treated as main temp and humidity source
dht DHT;

// OneWire
OneWire ds(ONEWIRE_PIN);
byte temp1_addr[8];
//byte temp2_addr[8];

//profiling
unsigned long t_loop_start;
unsigned long t_1;
unsigned long t_2;
unsigned long t_3;

Logger dht22_temp = Logger("Temp1");

Logger dht22_humidity = Logger("Humidity");

File sd_file;

// light sensor on analog A15
#define LIGHT_SENSOR_PIN 15
Logger light_sensor = Logger("Light1");
Logger light_sensor2 = Logger("Light2");

int ups_level;

Logger ultrasound = Logger("Usnd");

Logger onewire_temp1 = Logger("Temp2");
// Logger onewire_temp2 = Logger("Temp3");

Config config;
Output outputs;

aJsonStream serial_stream(&Serial);

EthernetServer server(80);

Logger * loggers[LOGGERS] = {&dht22_humidity, &dht22_temp, &light_sensor, &ultrasound, &onewire_temp1, &light_sensor2};

Trigger triggers[TRIGGERS];
Alert alerts[ALERTS];

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
    aJson.addItemToObject(msg, "sys_name", aJson.createItem(config.sys_name));
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
    aJson.addItemToObject(msg, "alerts", aJson.createItem(ALERTS));
    aJson.addItemToObject(msg, "triggers_log_size", aJson.createItem(LOGSIZE));
    sprintf(buffer, "%ld", millis() / 1000);
    aJson.addItemToObject(msg, "uptime", aJson.createItem(buffer));
    aJson.addItemToObject(msg, "tz", aJson.createItem(config.time_zone));
    aJson.addItemToObject(msg, "daymin", aJson.createItem(daymin()));
    aJson.addItemToObject(msg, "ups_level", aJson.createItem(ups_level));
    digitalClockDisplay(buffer);
    aJson.addItemToObject(msg, "time", aJson.createItem(buffer));


    return msg;
}

void setup(void) {
    int i;
    int we_have_net = 0;
    wdt_disable();
    pinMode(13, OUTPUT);
    // start serial port
    Serial.begin(115200);
    Serial.println(F("Grow!"));
    lcd_setup();
    pFreeRam();

    delay(1000);

    //Serial.println(F("SD Card init"));
    lcd_print_immediate(F("SD Card init"));
    // disable ethernet before card init
    pinMode(10,OUTPUT);
    digitalWrite(10,HIGH);
    pinMode(53,OUTPUT);
    digitalWrite(53,HIGH);
    if (sdcard_init()) {
        lcd_print_immediate(F("SD init OK"));
    } else {
        lcd_print_immediate(F("SD init failure"));
    }

    char index[] = "/index.htm";
    if (SD.exists(index)) {
        lcd_print_immediate(F("INDEX.HTM found OK"));
    } else {
        lcd_print_immediate(F("INDEX.HTM not found"));
        int q = 0;
        while (!SD.exists(index)) {
            digitalWrite(13, q);
            q = 1 - q;
            delay(600);
        }
    }
    digitalWrite(13, LOW);
    // Serial.println(F("Inititalising Ethernet"));
    lcd_print_immediate(F("Starting Eth..."));

    // load config from sdcard
    pFreeRam();
    aJsonObject * cfile = file_read("", "config.jso");
    if (cfile != NULL) {
        config.load(cfile);
        aJson.deleteItem(cfile);
    } else {
        lcd_print_immediate(F("E:Default cfg"));
    }
    pFreeRam();
    config.save();
    pFreeRam();
    if (config.use_dhcp == 1) {
        we_have_net = Ethernet.begin(config.mac);
    }

    if (we_have_net == 0) {
        Ethernet.begin(config.mac, config.ip, config.gateway, config.gateway, config.netmask);
    }
    server.begin();
    pFreeRam();
    Serial.print(F("server is at "));
    Serial.println(Ethernet.localIP());

    // load triggers if available
    Serial.println(freeRam());
    triggers_load(triggers, loggers);
    Serial.println(freeRam());

    // load alerts
    pFreeRam();
    Serial.println(F("Loading alerts"));
    alerts_load();

    #ifdef USE_GSM
    pFreeRam();
    lcd_print_immediate(F("Starting GSM..."));
    //Serial3.begin(9600);
    gsm.TurnOn(9600);
    #endif

    // start real time clock
    pFreeRam();
    //Serial.println(F("Initialising clock"));
    lcd_print_immediate(F("Starting clock"));
    daytime_init();

    // find ds temp sensor addresses
    ds.reset_search();
    ds.search(temp1_addr);
    //  ds.search(temp2_addr);

    //load data from sd card
    for(i=0; i <LOGGERS; i++){
        loggers[i]->load();
    }

    //initialise outputs
    outputs.common_init();
    pFreeRam();
    Serial.println(F("Loading output history"));
    outputs.load();
    pFreeRam();
    Serial.println(F("Relay setup"));
    for(i=0; i <8; i++) {
        pinMode(RELAY_START + i, OUTPUT);
        // outputs.set(i, 0);
    }
    //outputs.load();

    #ifdef WATCHDOG
    wdt_enable(WDTO_8S);
    #endif

    // init temp/humidity logger
    pFreeRam();
    lcd_flush();
    lcd_print_immediate(F("Setup done"));
}

void worker(){
    // Here the sensors are read, files written and so on. Once per minute
#ifdef DEBUG
    Serial.print(F("Uptime: "));
    Serial.println(millis() / 1000);
#endif
    digitalWrite(13, HIGH);
    //read sensors and store data
    //myDHT22.readData();
    int chk = DHT.read22(DHT22_PIN);
    switch (chk) {
        case DHTLIB_OK:
            break;
        case DHTLIB_ERROR_CHECKSUM:
            Serial.println(F("DHT Checksum error"));
            break;
        case DHTLIB_ERROR_TIMEOUT:
            Serial.println(F("DHT Time out error"));
            break;
        default:
            Serial.println(F("Unknown error"));
            break;
    }
    int temp = (int) lround(10 * DHT.temperature);
    dht22_temp.timed_log(temp);

    int hum = (int) lround(10 * DHT.humidity);
    dht22_humidity.timed_log(hum);

    light_sensor.timed_log(map(analogRead(LIGHT_SENSOR_PIN), 0, 1024, 0, 1000));
    light_sensor2.timed_log(map(analogRead(LIGHT_SENSOR_PIN-1), 0, 1024, 0, 1000));
    ultrasound.timed_log(ultrasound_ping(USOUND_TRG, USOUND_ECHO));

    onewire_temp1.timed_log(ds_read(ds, temp1_addr));
    //onewire_temp2.timed_log(ds_read(ds, temp2_addr));

#ifdef DEBUG_LOGGERS
    //  int numLogers = sizeof(loggers) / sizeof(Logger *);
    // Serial.print(F("# of loggers: "));
    // Serial.println(numLogers, DEC);

    for(int i=0; i < LOGGERS; i++) {
#ifdef WATCHDOG
        wdt_reset();
#endif
        Serial.print(loggers[i]->name);
        Serial.print(F(": "));
        aJsonObject *msg = loggers[i]->json();
        aJson.print(msg, &serial_stream);
        aJson.deleteItem(msg);
        Serial.println();
    }
#endif
    ups_level = analogRead(UPS_READ_PIN);

#ifdef DEBUG_OUTPUT
    pFreeRam();
    outputs.json(&Serial);
    Serial.println();
    pFreeRam();
#endif


    // tick triggers
    for(int i=0; i < TRIGGERS; i++) {
#ifdef DEBUG_TRIGGERS
        Serial.print(F("Trigger "));
        Serial.println(i);
        trigger_json(i, &sd_file);
        Serial.println("");
#endif

        trigger_tick(i);
    }
    // tick alerts
    for(int i=0; i < ALERTS; i++) {
#ifdef DEBUG_ALERTS
        Serial.print(F("Alert "));
        Serial.println(i);
        Serial.print(F(" "));
        Serial.print(alerts[i].last_state);
        Serial.println("");
#endif

        alert_tick(i);
    }

    // move relays
    for(int i=0; i <8; i++) {
        outputs.hw_update(i);
    }

    outputs.log();

    // Send things to lcd
    lcd_flush();
    char lcd_msg[18];
    snprintf(lcd_msg, 17, "Air Temp %d.%dC", dht22_temp.peek() / 10, abs(dht22_temp.peek() % 10));
    lcd_publish(lcd_msg);

    snprintf(lcd_msg, 17, "Humidity %d.%d%%", dht22_humidity.peek() / 10, abs(dht22_humidity.peek() % 10));
    lcd_publish(lcd_msg);

    snprintf(lcd_msg, 17, "Water Lvl %dcm", ultrasound.peek());
    lcd_publish(lcd_msg);

    snprintf(lcd_msg, 17, "Water Temp %d.%dC", onewire_temp1.peek() / 10, abs(onewire_temp1.peek() % 10));
    lcd_publish(lcd_msg);

    int uptime = millis() / 60000;
    snprintf(lcd_msg, 17, "Uptime %d", uptime);
    lcd_publish(lcd_msg);

    lcd_tick();

#ifdef USE_GSM
    int gsm_reg;
    gsm_reg = gsm.CheckRegistration();
    switch (gsm_reg){
        case REG_NOT_REGISTERED:
            lcd_publish("GSM not registered");
            break;
        case REG_REGISTERED:
            lcd_publish("GSM registered");
            if (gsm_init_done == 0) {
                Serial3.println("AT+CLTS=1\r");
                Serial3.println("AT+CENG=3\r");
            }
            break;
        case REG_NO_RESPONSE:
            lcd_publish("GSM no response");
            break;
        case REG_COMM_LINE_BUSY:
            lcd_publish("GSM busy");
            break;
    }
#endif

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
            outputs.json(&client);
            return 1;
        }
        for (int i = 0; i < LOGGERS; i++) {
            if (loggers[i]->match(request)){  // sensors
                found = true;
                send_headers(client, request, 30);
                loggers[i]->printjson(&client);
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
    sd_file = SD.open(request, FILE_READ);
    // abuse clientline as sd buffer
    int remain = 0;
    while ((remain = sd_file.available())) {
        remain = min(remain, BUFSIZE -1);
        sd_file.read(clientline, remain);
        clientline[remain] = 0;
        client.write(clientline);
    };
    sd_file.close();
    return 1;
}

int fn_extract_trg(char * request){
    int trg = -1;
    sscanf(request, "triggers/%d.jso", &trg);
    if (trg >= TRIGGERS) trg = -1;
    return trg;
}

int fn_extract_alert(char * request){
    int alert = -1;
    sscanf(request, "alerts/%d.jso", &alert);
    if (alert >= ALERTS) alert = -1;
    return alert;
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
            #ifdef WATCHDOG
            wdt_reset();
            #endif
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
        int trg_no = fn_extract_trg(request);
        int alert_no = fn_extract_alert(request);

        if (strcasecmp(request, "client.jso") == 0) {
            //aJsonStream eth_stream(&client);
            //aJsonObject * data = aJson.parse(&eth_stream);
            //file_write("", "client.jso", data);
            //aJson.deleteItem(data);
            file_passthru("", "client.jso", &client);
        } else if (strcasecmp(request, "config.jso") == 0) {
            aJsonStream eth_stream(&client);
            aJsonObject * data = aJson.parse(&eth_stream);
            config.load(data);
            config.save();
            aJson.deleteItem(data);
        } else if (trg_no > -1) {
            aJsonStream eth_stream(&client);
            aJsonObject * data = aJson.parse(&eth_stream);
            trigger_load(trg_no, data, loggers);
            trigger_save(triggers, trg_no);
            aJson.deleteItem(data);
        } else if (alert_no > -1) {
            //aJsonStream eth_stream(&client);
            //aJsonObject * data = aJson.parse(&eth_stream);
            //alert_load_target(alert_no, data);
            alert_passthru(alert_no, &client);
            alerts_load();
            //aJson.deleteItem(data);
        }
#ifdef DEBUG_HTTP
    Serial.println(F("POST request dealt with"));
#endif
    }



    // give the web browser time to receive the data
    delay(5);
    // close the connection:
    client.stop();
#ifdef DEBUG_HTTP
    Serial.println(F("eth client stopped"));
#endif
}

void loop(void){
    #ifdef WATCHDOG
    wdt_reset();
    #endif

    t_loop_start = millis();
    if (dht22_temp.available()) {
        worker();
        pFreeRam();
    }
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
    lcd_tick();
}
