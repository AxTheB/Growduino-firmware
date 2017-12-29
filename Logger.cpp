#include "GrowduinoFirmware.h"

#include <string.h>

int log_every_time = true;

int buf_h[24];
int buf_day[31];

Logger::Logger(const char * logger_name) {
  setup();
  strncpy(name, logger_name, 8);
  name[8] = '\0';
}

void Logger::setup() {
  //prepare buffer
  buffer_cleanup(buf_min, 60, 0, -1);
}

void Logger::load() {
  SERIAL.println(F("Loading buffer data"));
  char dirname[64];
  char filename[13];
  aJsonObject * data;

  dirname_l1(dirname);
  sprintf(filename, "%02d.jso", hour());
  data = file_read(dirname, filename);
  if (data) {
    //buffer_load(int * buffer, char * buf_name, aJsonObject * data);
    idx_min = buffer_load(buf_min, "min", data);
    aJson.deleteItem(data);
  } else {
    SERIAL.println(F("Minute data load failure"));
  }
}

void Logger::load_all() {
  char dirname[64];
  char filename[13];
  aJsonObject * data;

  buffer_cleanup(buf_h, 24, 0, -1);
  buffer_cleanup(buf_day, 31, 0, -1);
  SERIAL.println(F("Loading day/month data"));

  dirname_l2(dirname);
  sprintf(filename, "%02d.jso", day());
  data = file_read(dirname, filename);
  if (data) {
    buffer_load(buf_h, "h", data);
    aJson.deleteItem(data);
  } else {
    SERIAL.println(F("Hourly data load failure"));
  }

  dirname_l3(dirname);
  sprintf(filename, "%02d.jso", month());
  data = file_read(dirname, filename);
  if (data) {
    buffer_load(buf_day, "day", data);
    aJson.deleteItem(data);
  } else {
    SERIAL.println(F("Daily data load failure"));
  }
}

bool Logger::available() {
  // for timed logging, return false if we already logged this minute
  if ( idx_min == minute()) {
    return false;
  }
  return true;
}

bool Logger::match(const char * request) {
  // compares logger name to the request string
  // return true if we want this logger
  char * filename;
  char filebuf[13];

  filename = strrchr(request, '/');

  if (filename == NULL) {
    // something weird happened, we should have / in request
    // as we are sensors/sensor.jso
    return false;
  }

  // we can get weird things in filename
  // otoh name is safe, so make filename from name and compare that
  filename = filename + 1;
  sprintf(filebuf, "%s.jso", name);

  return (strcasecmp(filename, filebuf) == 0);
}

char * Logger::dirname_l1(char * dirname) {
  sprintf(dirname, "/data/%s/%d/%02d/%02d", name, year(), month(), day());
  return dirname;
}

char * Logger::dirname_l2(char * dirname) {
  sprintf(dirname, "/data/%s/%d/%02d", name, year(), month());
  return dirname;
}

char * Logger::dirname_l3(char * dirname) {
  sprintf(dirname, "/data/%s/%d", name, year());
  return dirname;
}

void Logger::timed_log(int value) {
  //Write value to l1, recalculate l2 and l3 buffer
  peekval = value;

  bool save_all;

  time = now();

  idx_min_new = minute();
  idx_h_new = hour();
  idx_day_new = day() - 1;

  //here we store the data into buffers
  buffer_store(buf_min, 60, value, idx_min_new, idx_min);
  if (idx_min_new  == 59) {
    save_all = true;
    load_all();
    buffer_store(buf_h, 24, buffer_avg(buf_min, 60), idx_h_new, idx_h_new);  // buffer is cleaned and reloaded, so its ok to use idx_h_new twice here
    buffer_store(buf_day, 31, buffer_avg(buf_h, 24), idx_day_new, idx_day_new);
  } else {
    save_all = false;
  }
  idx_min = idx_min_new;

  //write to card
  char dirname[40];
  char filename[13];
  // l1 - write every minute
  dirname_l1(dirname);
  sprintf(filename, "%02d.jso", hour());
  file_for_write(dirname, filename, &sd_file);
  sd_file.print(F("{"));
  // void buffer_printjson(int * buffer, char * buf_name, int index, bool full, Stream * output);
  buffer_printjson(buf_min, 60, "min", idx_min, false, &sd_file);
  sd_file.print(F("}"));
  sd_file.close();

  if (save_all) {
    // l2
    dirname_l2(dirname);
    sprintf(filename, "%02d.jso", day());
    file_for_write(dirname, filename, &sd_file);
    sd_file.print(F("{"));
    buffer_printjson(buf_h, 24, "h", idx_h_new, false, &sd_file);
    sd_file.print(F("}"));
    sd_file.close();
    // l3
    dirname_l3(dirname);
    sprintf(filename, "%02d.jso", month());
    file_for_write(dirname, filename, &sd_file);
    sd_file.print(F("{"));
    buffer_printjson(buf_day, 31, "day", idx_day_new, false, &sd_file);
    sd_file.print(F("}"));
    sd_file.close();
  }
}

void Logger::printjson(Stream * output) {
  output->print("{\"name\":\"");
  output->print(name);
  output->print("\",");
  buffer_printjson(buf_min, 60, "min", idx_min, true, output);
  output->print("}");
}

/*
  void Logger::log(int value) {
    //zapise do l1 bufferu, pripadne buffery otoci.
    peekval = value;
    time = now();

    l1.store(value, ++l1_idx);
    int tmpavg = l1.get_last_avg();
    if (tmpavg != MINVALUE){
        l2.store(tmpavg, ++l2_idx);
        int tmpavg = l2.get_last_avg();
        if (tmpavg != MINVALUE){
            l3.store(tmpavg, ++l3_idx);
            //int tmpavg = l2.get_last_avg();
        }
    }
  }
*/

int Logger::peek() {
  return peekval;
}
