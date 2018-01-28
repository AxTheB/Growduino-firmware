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

#ifdef DISPLAY_2004

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3f, 20, 4); //address could be 0x27 or 0x3f
#else

#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
#endif

int ether = 1;

// DHT22 temp and humidity sensor. Treated as main temp and humidity source
dht DHT;

// OneWire
OneWire ds1(ONEWIRE_PIN);
OneWire ds2(ONEWIRE_PIN2);

//profiling
unsigned long t_loop_start;
unsigned long t_1;
unsigned long t_2;
unsigned long t_3;

Logger dht22_temp = Logger("Temp1");

Logger dht22_humidity = Logger("Humidity");

File sd_file;

Logger light_sensor = Logger("Light1");
Logger light_sensor2 = Logger("Light2");

Logger battery = Logger("Battery");
int ups_level;

Logger ultrasound = Logger("Usnd");

Logger onewire_temp1 = Logger("Temp2");
Logger onewire_temp2 = Logger("Temp3");

Logger ec = Logger("EC");
Logger ph = Logger("pH");
Logger co2 = Logger("CO2");

Config config;
Output outputs;

aJsonStream serial_stream(&SERIAL);

EthernetServer server(80);

Logger * loggers[LOGGERS + 1] = {
  &dht22_humidity,
  &dht22_temp,
  &light_sensor,
  &ultrasound,
  &onewire_temp1,
  &light_sensor2,
  &onewire_temp2,
  &ec,
  &ph,
  &co2,
  &battery
};

Trigger triggers[TRIGGERS];
Alert alerts[ALERTS];

int prepareAnalogPin(int pin) {

  //pull the pin high
  pinMode(pin, INPUT);
#ifdef ANALOG_DETECT
  digitalWrite(pin, HIGH);
#endif
}


int analogReadAvg(int pin) {


  // introduce delay between mux switch and actual reading
  analogRead(pin);
  delay(ANALOG_READ_AVG_DELAY);
  analogRead(pin);

#ifdef DEBUG_CALIB
  int minval, maxval;
  minval = MINVALUE;
  maxval = MINVALUE;
  SERIAL.print(F("Analog read "));
  SERIAL.println(pin);
#endif
  long dataSum = 0L;
  int data;
#ifdef WATCHDOG
  SERIAL.print(F("Analog read avg timer reset"));
  wdt_reset();
#endif

  for (int i = 0; i < ANALOG_READ_AVG_TIMES; i++) {
    data = analogRead(pin);
    dataSum += data;
#ifdef WATCHDOG
    SERIAL.print(F("."));
    wdt_reset();
#endif

#ifdef DEBUG_CALIB
    SERIAL.println(data);
    if (minval == MINVALUE || minval > data) {
      minval = data;
    }
    if (maxval == MINVALUE || maxval < data) {
      maxval = data;
    }
#endif

    delay(ANALOG_READ_AVG_DELAY);
  }

#ifdef WATCHDOG
  SERIAL.println(F(" done"));
  wdt_reset();
#endif

  int retval = (int) (dataSum / ANALOG_READ_AVG_TIMES);

#ifdef DEBUG_CALIB
  SERIAL.println("");
  SERIAL.print(F("min: "));
  SERIAL.print(minval);
  SERIAL.print(F(" max: "));
  SERIAL.print(maxval);
  SERIAL.print(F(" avg: "));
  SERIAL.println(retval);
#endif

  return retval;
}

int triple_read(int (* funct)()){ 
    return return_middle((*funct)(), (*funct)(), (*funct)());
}

int triple_read(int (* funct)(int), int param1){ 
    return return_middle((*funct)(param1), (*funct)(param1), (*funct)(param1));
}

int return_middle(int first_value, int second_value, int third_value){
#ifdef WATCHDOG
  wdt_reset();
#endif
    int values[] = {first_value, second_value, third_value};
    int * tmpval;
    if (values[0] > values[1]){
        tmpval = values[1];
        values[1] = values[0];
        values[0] = tmpval;
        }

    if (values[1] > values[2]){
        tmpval = values[2];
        values[2] = values[1];
        values[1] = tmpval;
        }

    if (values[0] > values[1]){
        tmpval = values[1];
        values[1] = values[0];
        values[0] = tmpval;
        }

    if (values[0] != MINVALUE) {
        return values[1];  // middle value if there are not any MINVALUEs
    }

    if (values[1] == MINVALUE){
        return values[2];
    } else {
        return (values[1] + values[2]) /2; 
    }   
}


int perThousand(int pin) {
  int retval;
  retval = analogReadAvg(pin);
#ifdef ANALOG_DETECT
  if (retval > ADC_CUTOFF) {
    return MINVALUE;
  }
#endif
  retval = map(retval, 0, 1024, 0, 1000);
  return retval;
}


int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void pFreeRam() {
  SERIAL.print(F("Free ram: "));
  SERIAL.println(freeRam());
}

aJsonObject * status() {
  aJsonObject * msg = aJson.createObject();
  char buffer[21];
  int freeram = freeRam();
  aJson.addItemToObject(msg, "sys_name", aJson.createItem(config.sys_name));
  aJson.addItemToObject(msg, "free_ram", aJson.createItem(freeram));
  aJson.addItemToObject(msg, "sensors", aJson.createItem(LOGGERS));
  aJson.addItemToObject(msg, "outputs", aJson.createItem(OUTPUTS));
  aJsonObject * logger_list = aJson.createObject();
  for (int i = 0; i < LOGGERS; i++) {
    sprintf(buffer, "%d", i);
    aJson.addItemToObject(logger_list, buffer, aJson.createItem(loggers[i]->name));
  }
  aJson.addItemToObject(msg, "sensor_list", logger_list);
  aJson.addItemToObject(msg, "triggers", aJson.createItem(TRIGGERS));
  aJson.addItemToObject(msg, "alerts", aJson.createItem(ALERTS));
  aJson.addItemToObject(msg, "triggers_log_size", aJson.createItem(LOGSIZE));
  sprintf(buffer, "%lu", millis() / 1000);
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
  SERIAL.begin(SER_SPEED);
  SERIAL.println(F("Grow!"));
  lcd_setup();
  pFreeRam();

  for (int i = 0; i < 16; i++) {
    prepareAnalogPin(A0 + i);
  }

  delay(1000);

  //SERIAL.println(F("SD Card init"));
  lcd_print_immediate(F("SD Card init"));
  // disable ethernet before card init
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  pinMode(53, OUTPUT);
  digitalWrite(53, HIGH);
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
  // SERIAL.println(F("Inititalising Ethernet"));
  lcd_print_immediate(F("Loading config..."));

  // load config from sdcard
  pFreeRam();
  aJsonObject * cfile = file_read("", "config.jso");
  if (cfile != NULL) {
    config.load(cfile);
    aJson.deleteItem(cfile);
  } else {
    lcd_print_immediate(F("E:Default cfg"));
  }
  cfile = file_read("", "calib.jso");
  if (cfile != NULL) {
    config.loadcal(cfile);
    aJson.deleteItem(cfile);
  } else {
    lcd_print_immediate(F("E:Default cfg"));
  }
  pFreeRam();
  config.save();
  config.savecal();
  pFreeRam();
  lcd_print_immediate(F("Starting eth..."));
  if (config.use_dhcp == 1) {
    we_have_net = Ethernet.begin(config.mac);
  }

  if (we_have_net == 0) {
    Ethernet.begin(config.mac, config.ip, config.gateway, config.gateway, config.netmask);
  }
  server.begin();
  pFreeRam();
  SERIAL.print(F("server is at "));
  SERIAL.println(Ethernet.localIP());

  // load triggers if available
  SERIAL.println(freeRam());
  triggers_load(triggers, loggers);
  SERIAL.println(freeRam());

  // load alerts
  pFreeRam();
  SERIAL.println(F("Loading alerts"));
  alerts_load();

  // start real time clock
  pFreeRam();
  //SERIAL.println(F("Initialising clock"));
  lcd_print_immediate(F("Starting clock"));
  daytime_init();

  //load data from sd card
  for (i = 0; i < LOGGERS; i++) {
    loggers[i]->load();
  }
#ifdef USE_EC_SENSOR
  ec_enable();
#endif

  //initialise outputs
  outputs.common_init();
  pFreeRam();
  SERIAL.println(F("Loading output history"));
  outputs.load();
  pFreeRam();
  SERIAL.println(F("Relay setup"));
  for (i = 0; i < OUTPUTS; i++) {
    pinMode(RELAY_START + i, OUTPUT);
    // outputs.set(i, 0);
  }
  outputs.log();
  //outputs.load();

#ifdef WATCHDOG
  SERIAL.println(F("Watchdog start"));
  wdt_enable(WDTO_8S);
  SERIAL.println(F("Watchdog reset"));
  wdt_reset();
  SERIAL.println(F("Watchdog reset second time"));
  wdt_reset();
#endif

  ups_init();

  pFreeRam();
  lcd_flush();
  lcd_print_immediate(F("Setup nearly done"));
  worker();
  lcd_print_immediate(F("Setup done"));
}

void worker() {
  // Here the sensors are read, files written and so on. Once per minute
#ifdef DEBUG
  SERIAL.print(F("Uptime: "));
  SERIAL.println(millis() / 1000);
#endif
#ifdef WATCHDOG
  SERIAL.println(F("Watchdog reset worker 1"));
  wdt_reset();
#endif

  digitalWrite(13, HIGH);
  //read sensors and store data
  //myDHT22.readData();
  int chk = DHT.read22(DHT22_PIN);
  switch (chk) {
    case DHTLIB_OK:
      break;
    case DHTLIB_ERROR_CHECKSUM:
      SERIAL.println(F("DHT Checksum error"));
      break;
    case DHTLIB_ERROR_TIMEOUT:
      SERIAL.println(F("DHT Time out error"));
      break;
    default:
      SERIAL.println(F("Unknown error"));
      break;
  }
  int temp = (int) lround(10 * DHT.temperature);
  if (temp == (10 * MINVALUE)) {
    temp = MINVALUE;
  }
  dht22_temp.timed_log(temp);

  int hum = (int) lround(10 * DHT.humidity);
  if (hum == (10 * MINVALUE)) {
    hum = MINVALUE;
  }
#ifdef WATCHDOG
  SERIAL.println(F("Watchdog reset worker 2"));
  wdt_reset();
#endif

  dht22_humidity.timed_log(hum);

  light_sensor.timed_log(triple_read(&perThousand, LIGHT_SENSOR_PIN_1));
  light_sensor2.timed_log(triple_read(&perThousand, LIGHT_SENSOR_PIN_2));
#ifdef WATCHDOG
  SERIAL.println(F("Watchdog reset worker 3"));
  wdt_reset();
#endif

  ultrasound.timed_log(triple_read(&ultrasound_ping));
#ifdef WATCHDOG
  SERIAL.println(F("Watchdog reset worker 4"));
  wdt_reset();
#endif

#ifdef USE_EC_SENSOR
  ec.timed_log(triple_read(&ec_read));
#ifdef WATCHDOG
  SERIAL.println(F("Watchdog reset worker 5"));
  wdt_reset();
#endif
#endif

#ifdef USE_PH_SENSOR
  ph.timed_log(triple_read(&PH_read));
#ifdef WATCHDOG
  SERIAL.println(F("Watchdog reset worker 6"));
  wdt_reset();
#endif
#endif

#ifdef USE_CO2_SENSOR
  co2.timed_log(triple_read(&CO2_read));
#endif

  battery.timed_log(ups_read());

#ifdef WATCHDOG
  SERIAL.println(F("Watchdog reset worker 7"));
  wdt_reset();
#endif


  onewire_temp1.timed_log(ds_read(ds1));
  onewire_temp2.timed_log(ds_read(ds2));

#ifdef DEBUG_LOGGERS
  //  int numLogers = sizeof(loggers) / sizeof(Logger *);
  // SERIAL.print(F("# of loggers: "));
  // SERIAL.println(numLogers, DEC);

  for (int i = 0; i < LOGGERS; i++) {
#ifdef WATCHDOG
    SERIAL.println(F("Watchdog reset logger loop"));
    wdt_reset();
#endif
    SERIAL.print(loggers[i]->name);
    SERIAL.print(F(": "));
    aJsonObject *msg = loggers[i]->json();
    aJson.print(msg, &serial_stream);
    aJson.deleteItem(msg);
    SERIAL.println();
  }
#endif

#ifdef DEBUG_OUTPUT
  pFreeRam();
  outputs.json(&SERIAL);
  SERIAL.println();
  pFreeRam();
#endif


  // tick triggers
  for (int i = 0; i < TRIGGERS; i++) {
#ifdef DEBUG_TRIGGERS
    SERIAL.print(F("Trigger "));
    SERIAL.println(i);
    trigger_json(i, &sd_file);
    SERIAL.println("");
#endif

    trigger_tick(i);
  }
  // tick alerts
  for (int i = 0; i < ALERTS; i++) {
#ifdef DEBUG_ALERTS
    SERIAL.print(F("Alert "));
    SERIAL.print(i);
    SERIAL.print(F(" last state "));
    SERIAL.print(alerts[i].last_state);
    SERIAL.println("");
#endif

    alert_tick(i);
  }

  // move relays
  for (int i = 0; i < OUTPUTS; i++) {
    outputs.hw_update(i);
  }

  outputs.log();

  // Send things to lcd
  lcd_flush();
  lcd_publish("Hi R-Man");
  lcd_publish("Growduino.cz");
  lcd_publish("Air Temp", "%s %d.%dC", dht22_temp.peek(), 10);
  lcd_publish("Humidity", "%s %d.%d%%", dht22_humidity.peek(), 10);
  lcd_publish("Water Temp", "%s %d.%dC", onewire_temp1.peek(), 10);
  lcd_publish("Water Lvl", "%s %dcm", ultrasound.peek());
  lcd_publish("Bulb Temp", "%s %d.%dC", onewire_temp2.peek(), 10);
#ifdef USE_PH_SENSOR
  lcd_publish("pH", "%s %d.%.2d", ph.peek(), 100);
#endif
#ifdef USE_CO2_SENSOR
  lcd_publish("CO2", "%s %d", co2.peek(), 0.1);
#endif
#ifdef USE_EC_SENSOR
  lcd_publish("EC", "%s %d.%.2d", ec.peek(), 100);
#endif
  unsigned long uptime = millis() / 60000;
  lcd_publish("Uptime", "%s %d", uptime);
#ifdef HAVE_UPS
  lcd_publish("Battery", "%s %d%%", battery.peek());
#endif

  lcd_tick();

  digitalWrite(13, LOW);
}

const char * get_filename_ext(const char *filename) {
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

void responseNum(EthernetClient client, int code) {
  SERIAL.print(F("Returning "));
  switch (code) {
    case 200:
      client.println("HTTP/1.1 200 OK");
      SERIAL.println(200);
      break;
    case 404:
      SERIAL.println(404);
      client.println("HTTP/1.1 404 Not Found");
      client.println("Content-Type: text/html");
      client.println();
      client.println("<h2>File Not Found!</h2>");
      break;
  }
}

const char * extract_filename(char * line) {
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

int get_raw_data(int idx, Stream * output) {
  output->print("{\"raw_value\":\"");
  /*
      "0":"Humidity"
      "1":"Temp1"
      "2":"Light1"
      "3":"Usnd"
      "4":"Temp2"
      "5":"Light2"
      "6":"Temp3"
      "7":"EC"
      "8":"pH"
      "9":"CO2"
  */
  switch (idx) {
    case 3:  // usnd
      output->print(ultrasound_ping(USOUND_TRG, USOUND_ECHO));
      break;
    case 7:  // ec
#ifdef USE_EC_SENSOR
      output->print(ec_calib_raw());
#else
      output->print("disabled");
#endif
      break;
    case 8: //pH
#ifdef USE_PH_SENSOR
      output->print(PH_read_raw());
#else
      output->print("disabled");
#endif
      break;
    case 9:  //CO2
#ifdef USE_CO2_SENSOR
      output->print(CO2_read_raw());
#else
      output->print("disabled");
#endif
      break;
    default:
      output->print("-1");
  }
  output->print("\"}");
}

int fn_extract_raw(char * request) {
  int sen = -1;
  sscanf(request, "sensors/rawdata/%d.jso", &sen);
  if (sen > LOGGERS) sen = -1;
  return sen;
}


int senddata(EthernetClient client, char * request, char * clientline) {
#ifdef WATCHDOG
  SERIAL.println(F("Watchdog reset: send data"));
  wdt_reset();
#endif
  // Send response
  bool found = false;
  SERIAL.print(F("Request: "));
  SERIAL.println(request);
  int raw_no = fn_extract_raw(request);
  if (strncasecmp(request, "sensors", 7) == 0) {
    SERIAL.println(F("Sensor area"));
    if (raw_no > -1) {
      SERIAL.println("raw?");
      get_raw_data(raw_no, &client);
      return 1;
    } else {
      SERIAL.print(F("raw missed "));
      SERIAL.println(fn_extract_raw(request));
    }

    if (outputs.match(request)) {  // outputs
      found = true;
      send_headers(client, request, 30);
      aJsonStream eth_stream(&client);
      outputs.json(&client);
      return 1;
    }
    for (int i = 0; i < LOGGERS; i++) {
      if (loggers[i]->match(request)) { // sensors
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
  SERIAL.println(F("File area"));
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
    remain = min(remain, BUFSIZE - 1);
    sd_file.read(clientline, remain);
    clientline[remain] = 0;
    client.write(clientline);
  };
  sd_file.close();
  return 1;
}

int fn_extract_trg(char * request) {
  int trg = -1;
  sscanf(request, "triggers/%d.jso", &trg);
  if (trg >= TRIGGERS) trg = -1;
  return trg;
}

int fn_extract_alert(char * request) {
  int alert = -1;
  sscanf(request, "alerts/%d.jso", &alert);
  if (alert >= ALERTS) alert = -1;
  return alert;
}

void pageServe(EthernetClient client) {
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
      SERIAL.println(F("Watchdog reset pageServe"));
      wdt_reset();
#endif
      linesize = client.readBytesUntil('\n', clientline, BUFSIZE - 1);
      clientline[linesize] = '\0';
#ifdef DEBUG_HTTP
      SERIAL.println(clientline);
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
        SERIAL.println(F("Processing post request"));
        is_url = true;
      };
      if (is_url) {
        if (strlcpy(request, extract_filename(clientline), 32) >= 32) {
          SERIAL.print(F("Filename too long: "));
          SERIAL.println(clientline);
          request[32] = '\0';
        }
      }
    }
  }
  // Headers ale all here, if its post we now should read the body.
  if (post) {
#ifdef WATCHDOG
    SERIAL.println(F("Watchdog reset pageServe post"));
    wdt_reset();
#endif
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
    } else if (strcasecmp(request, "calib.jso") == 0) {
      aJsonStream eth_stream(&client);
      aJsonObject * data = aJson.parse(&eth_stream);
      config.loadcal(data);
      config.savecal();
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
    SERIAL.println(F("POST request dealt with"));
#endif
  }
  // give the web browser time to receive the data
  delay(5);
  // close the connection:
  client.stop();
#ifdef DEBUG_HTTP
  SERIAL.println(F("eth client stopped"));
#endif
}

void loop(void) {
#ifdef WATCHDOG
  // SERIAL.println(F("Watchdog reset")); too much serial output
  wdt_reset();
#endif

  t_loop_start = millis();
  if (dht22_temp.available()) {
#ifdef WATCHDOG
    SERIAL.println(F("Watchdog reset before sensors"));
    wdt_reset();
#endif
    worker();
    pFreeRam();
  }
  EthernetClient client = server.available();
  if (client) {
#ifdef WATCHDOG
    SERIAL.println(F("Watchdog reset before pageServe"));
    wdt_reset();
#endif
    pageServe(client);
    SERIAL.println(F("times:"));
    SERIAL.println(t_1 - t_loop_start);
    SERIAL.println(t_2 - t_loop_start);
    SERIAL.println(t_3 - t_loop_start);
    SERIAL.println(millis() - t_loop_start);
    pFreeRam();
  }
  delay(50);
  lcd_tick();
}

