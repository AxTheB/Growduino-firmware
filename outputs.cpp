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
    //log();
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

int Output::breakme(int slot, int val, int trigger){
    // update valute of %slot% 
    if (val == 0) {
        state[slot] = bitclr(broken[slot], trigger);
    } else {
        state[slot] = bitset(broken[slot], trigger);
    }
    return (state[slot] != 0);
}

void Output::kill(int slot, int trigger){
    breakme(slot, 1, trigger);
}

void Output::revive(int slot, int trigger){
    breakme(slot, 0, trigger);
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
#ifdef DEBUG_OUTPUT
    Serial.print(F("Setting "));
    Serial.print(value, DEC);
    Serial.print(F(" bit "));
    Serial.print(bit);
#endif
    value |= 1 << bit;
#ifdef DEBUG_OUTPUT
    Serial.print(F(" result "));
    Serial.println(value);
#endif
    return value;
}

int Output::bitclr(int value, int bit){
    // clear %bit% in %value% and return new value
#ifdef DEBUG_OUTPUT
    Serial.print(F("Clearing "));
    Serial.print(value, DEC);
    Serial.print(F(" bit "));
    Serial.print(bit);
#endif
    value &= ~(1 << bit);
#ifdef DEBUG_OUTPUT
    Serial.print(F(" result "));
    Serial.println(value);
#endif
    return value;
}

int Output::pack_states(){
    int packed;
    packed = 0;
#ifdef DEBUG_OUTPUT
    Serial.print(F("Packing: "));
#endif
    for (int i=0; i < OUTPUTS; i++){
#ifdef DEBUG_OUTPUT
        Serial.print(state[i]);
        Serial.print(F(" "));
#endif
        packed += get(i) << i;
    }
#ifdef DEBUG_OUTPUT
    Serial.print(F(": "));
    Serial.println(packed, BIN);
#endif
    return packed;
}

int Output::hw_update(int slot){
    // Update output %slot%, checking broken state
    // this is only place where we touch hardware
#ifdef DEBUG_OUTPUT
        Serial.print(F("Output "));
        Serial.print(slot, DEC);
#endif
    int wanted;
    if (broken[slot] != 0) {
#ifdef DEBUG_OUTPUT
        Serial.println(F(" is broken"));
#endif
        wanted = 0;
    } else {
        wanted = get(slot);
#ifdef DEBUG_OUTPUT
        Serial.print(F(" desired state "));
        Serial.println(wanted, DEC);
#endif
    }
    if (wanted != hw_state[slot]) {
#ifdef DEBUG_OUTPUT
        Serial.println(F("Changing output"));
#endif
        time_t t_now = now();
        digitalWrite(RELAY_START + slot, wanted);
        hw_state[slot] = wanted;
        log();
        ctimes[slot] = t_now;
    } else {
#ifdef DEBUG_OUTPUT
        Serial.println(F("Not changing output"));
#endif
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
    int packed_states;
    int last_rec;
    last_rec = log_index - 1;
    packed_states = pack_states();
    if (log_index > 0) {
        if (log_states[last_rec] == packed_states){
#ifdef DEBUG_OUTPUT
            Serial.println(F("Output log noop"));
#endif
            return;
        }
    }
#ifdef DEBUG_OUTPUT
        Serial.print(F("Output logging. Index:"));
        Serial.println(log_index);
        Serial.print(F("packed states: "));
        Serial.println(packed_states);
#endif
        log_states[log_index] = packed_states;
        log_times[log_index] = now();
        log_index++;
        if (log_index >= LOGSIZE) {
            log_index = 0;
            log_file_index++;
        }
        save();
        aJsonObject *msg;
        msg = json();

        aJson.deleteItem(msg);

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
    aJsonObject *msg = aJson.createObject();
    msg = json(msg);
    return msg;
}

aJsonObject * Output::json(aJsonObject *msg){
    char valname[] = "1390173000";
    aJson.addItemToObject(msg, "name", aJson.createItem(name));
    aJsonObject * values = aJson.createObject();
    for (int i = 0; i < log_index; i++) {
        sprintf(valname, "%ld", log_times[i]);
        aJson.addNumberToObject(values, valname, log_states[i]);
    }
    aJson.addItemToObject(msg, "state", values);
    return msg;
}

int Output::save(){
    char dirname[64];
    char filename[13];
    aJsonObject *msg = aJson.createObject();
    msg = json(msg);
    dir_name(dirname);
    file_name(filename);
    file_write(dirname, filename, msg);
    aJson.deleteItem(msg);
}

