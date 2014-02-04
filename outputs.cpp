#include "GrowduinoFirmware.h"

#include "outputs.h"


Output::Output() {
    common_init();
}

void Output::common_init(){
    time_t t_now = now();
    log_index = 0;
    log_file_index = 0;
    for (int i = 0; i < OUTPUTS; i++){
        broken[i] = 0;
        ctimes[i] = t_now;
        state[i] = 0;
        hw_state[i] = 0;

    }
    strcpy(name, "outputs");
}

int Output::get(int slot){
    // return true if %slot% should be on
    // ignoring broken state
    return (state[slot] != 0);
}

int Output::set(int slot, int val, int trigger){
    // update valute of %slot% 
    if (val == 0) {
        state[slot] = bitclr(state[slot], trigger);
    } else {
        state[slot] = bitset(state[slot], trigger);
    }
    return (state[slot] != 0);
}

int Output::uptime(int slot){
    // return time since last change
    return now() - ctimes[slot];
}

int Output::bitget(int value, int bit){
    // check if given bitfield %value% has bit %bit% set
    int out;
    out = !!(value & ( 1 << bit));
    return out;
}

int Output::bitset(int value, int bit){
    // set bit %bit% in %value% and return new value
#ifdef DEBUG_OUTPUTS
    Serial.print("Setting ");
    Serial.print(value, DEC);
    Serial.print(" bit ");
    Serial.print(bit);
#endif
    value |= 1 << bit;
#ifdef DEBUG_OUTPUTS
    Serial.print(" result ");
    Serial.println(out);
#endif
    return value;
}

int Output::bitclr(int value, int bit){
    // clear %bit% in %value% and return new value
#ifdef DEBUG_OUTPUTS
    Serial.print("Clearing ");
    Serial.print(value, DEC);
    Serial.print(" bit ");
    Serial.print(bit);
#endif
    value &= ~(1 << bit);
#ifdef DEBUG_OUTPUTS
    Serial.print(" result ");
    Serial.println(out);
#endif
    return value;
}

int Output::pack_states(){
    int packed;
    packed = 0;
    for (int i=0; i < OUTPUTS; i++){
#ifdef DEBUG_OUTPUTS
        Serial.print(state[i]);
        Serial.print(" ");
#endif
        packed += get(i) << i;
    }
#ifdef DEBUG_OUTPUTS
    Serial.print(": ");
    Serial.print(packed, BIN);
#endif
    return packed;
}

int Output::hw_update(int slot){
    // Update output %slot%, checking broken state
    // this is only place where we touch hardware
    int wanted;
    if (broken[slot] == 0) {
#ifdef DEBUG_OUTPUTS
        Serial.print("Output ");
        Serial.print(slot);
        Serial.println(" is broken");
#endif
        wanted = 0;
    } else {
        wanted = get(slot);
#ifdef DEBUG_OUTPUTS
        Serial.print("Desired state ");
        Serial.println(wanted, DEC);
#endif
    }
    if (wanted != hw_state[slot]) {
#ifdef DEBUG_OUTPUTS
        Serial.println("Changing output");
#endif
        time_t t_now = now();
        digitalWrite(RELAY_START + slot, wanted);
        log();
        ctimes[slot] = t_now;
    }
    return wanted;
}

bool Output::match(const char * request){
    // return true if we want this logger in http request
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

void Output::log(){
    log_states[log_index] = pack_states();
    log_times[log_index] = now();
    log_index++;
    if (log_index >= LOGSIZE) {
        log_index = 0;
        log_file_index++;
    }
    save();
}

char * Output::file_name(char * filename){
    sprintf(filename, "%d.jso", log_file_index);
    return filename;
}

char * Output::dir_name(char * dirname){
        sprintf(dirname, "/data/%s/%d/%02d/%02d", name, year(), month(), day());
        return dirname;
}

aJsonObject * Output::json(){
    char valname[] = "1390173000";
    aJsonObject * data = aJson.createObject();
    aJson.addStringToObject(data, "name", name);
    aJsonObject * values = aJson.createObject();
    for (int i = 0; i < log_index; i++) {
        sprintf(valname, "%ld", log_times[i]);
        aJson.addNumberToObject(values, valname, log_states[i]);
    }
    aJson.addItemToObject(data, "state", values);
}

int Output::save(){
    char dirname[64];
    char filename[13];
    aJsonObject *msg;

    msg = json();
    dir_name(dirname);
    file_name(filename);
    file_write(dirname, filename, msg);
    aJson.deleteItem(msg);
}

