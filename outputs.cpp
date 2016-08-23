

#include "outputs.h"
#include "daytime.h"
#include "sdcard.h"


Output::Output() {
    common_init();
}

void Output::common_init(){
    time_t t_now = utc_now();
    log_index = 0;
    log_file_index = 0;
    for (int slot = 0; slot < OUTPUTS; slot++){
        broken[slot] = 0;
        ctimes[slot] = t_now;
        state[slot] = 0;
        hw_state[slot] = 0;

    }
    strcpy(name, "outputs");
    //log();
}

int Output::get(int slot){
    // return true if %slot% should be on
    // ignoring broken state
    if (slot == -1 || slot >= OUTPUTS) return 0;
    return (state[slot] != 0);
}

int Output::hw_get(int slot){
    // return true if %slot% is on
    if (slot == -1 || slot >= OUTPUTS) return 0;
    return (hw_state[slot] != 0);
}

int Output::set(int slot, int val, int trigger){
    // update valute of %slot% 
    if (slot == -1 || slot >= OUTPUTS) {
#ifdef DEBUG_OUTPUT
        SERIAL.println(F("set: nop"));
#endif
        return 0;
    }
    if (val == 0) {
        state[slot] = bitclr(state[slot], trigger);
#ifdef DEBUG_OUTPUT
        SERIAL.print(F("set: clr "));
#endif
        SERIAL.print(slot);
    } else {
        state[slot] = bitset(state[slot], trigger);
#ifdef DEBUG_OUTPUT
        SERIAL.print(F("set: set "));
#endif
        SERIAL.print(slot);
    }
    return (state[slot] != 0);
}

int Output::breakme(int slot, int val, int trigger){
    // update valute of %slot% 

    if (slot < 0 || slot >= OUTPUTS || trigger < 0 || trigger > TRIGGERS ) {
        SERIAL.println(F("breakme: nop"));
        return NONE;
    }
    if (val == 0) {
        SERIAL.println(F("breakme: clr"));
        broken[slot] = bitclr(broken[slot], trigger);
    } else {
        SERIAL.println(F("breakme: set"));
        broken[slot] = bitset(broken[slot], trigger);
    }
    return (state[slot] != 0);
}

void Output::kill(int slot, int trigger){
    if (slot == -1 || slot >= OUTPUTS) return;
    SERIAL.print(F("Killing output #"));
    SERIAL.println(slot, DEC);
    breakme(slot, 1, trigger);
}

void Output::revive(int slot, int trigger){
    if (slot == -1 || slot >= OUTPUTS) return;
    SERIAL.print(F("Reviving output #"));
    SERIAL.println(slot, DEC);
    breakme(slot, 0, trigger);
}

time_t Output::uptime(int slot){
    // return time since last change
    if (slot == -1 || slot >= OUTPUTS) return 0;
    return utc_now() - ctimes[slot];
}

int Output::bitget(int value, int bit){
    // check if given bitfield %value% has bit %bit% set
    if (bit > 31) return 0;
    int out;
    out = !!(value & ( 1 << bit));
    return out;
}

 long Output::bitset(int value, int bit){
    // set bit %bit% in %value% and return new value
    if (bit > 31) return 0;
#ifdef DEBUG_OUTPUT
    SERIAL.print(F("Setting "));
    SERIAL.print(value, DEC);
    SERIAL.print(F(" bit "));
    SERIAL.print(bit);
#endif
    value |= 1 << bit;
#ifdef DEBUG_OUTPUT
    SERIAL.print(F(" result "));
    SERIAL.println(value);
#endif
    return value;
}

long Output::bitclr(int value, int bit){
    // clear %bit% in %value% and return new value
    if (bit > 31) return 0;
#ifdef DEBUG_OUTPUT
    SERIAL.print(F("Clearing "));
    SERIAL.print(value, DEC);
    SERIAL.print(F(" bit "));
    SERIAL.print(bit);
#endif
    value &= ~(1 << bit);
#ifdef DEBUG_OUTPUT
    SERIAL.print(F(" result "));
    SERIAL.println(value);
#endif
    return value;
}

int Output::pack_states(){
    int packed;
    packed = 0;
#ifdef DEBUG_OUTPUT
    SERIAL.print(F("Packing: "));
#endif
    for (int i=0; i < OUTPUTS; i++){
#ifdef DEBUG_OUTPUT
        SERIAL.print(state[i]);
        SERIAL.print(F(" "));
#endif
        packed += get(i) << i;
    }
#ifdef DEBUG_OUTPUT
    SERIAL.print(F(": "));
    SERIAL.println(packed, BIN);
#endif
    return packed;
}

int Output::hw_update(int slot){
    // Update output %slot%, checking broken state
    // this is only place where we touch hardware
#ifdef DEBUG_OUTPUT
        SERIAL.print(F("Output "));
        SERIAL.print(slot, DEC);

#endif
    int wanted;
    if (broken[slot] != 0) {
#ifdef DEBUG_OUTPUT
        SERIAL.println(F(" is broken"));
#endif
        wanted = 0;
    } else {
        wanted = get(slot);
#ifdef DEBUG_OUTPUT
        SERIAL.print(F(" desired state "));
        SERIAL.println(wanted, DEC);
#endif
    }
    if (wanted != hw_state[slot]) {
#ifdef DEBUG_OUTPUT
        SERIAL.println(F("Changing output"));
#endif
        time_t t_now = utc_now();
        digitalWrite(RELAY_START + slot, wanted);
        delay(100);
        hw_state[slot] = wanted;
        log();
        ctimes[slot] = t_now;
    } else {
#ifdef DEBUG_OUTPUT
        SERIAL.println(F("Not changing output"));
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
            if (daymin() > last_save_daymin) {
                last_save_daymin = daymin();
            }
#ifdef DEBUG_OUTPUT
            SERIAL.println(F("Output log noop"));
#endif
            return;
        }
    }
#ifdef DEBUG_OUTPUT
    SERIAL.print(F("Output logging. Index:"));
    SERIAL.println(log_index);
    SERIAL.print(F("packed states: "));
    SERIAL.println(packed_states);
#endif
    log_states[log_index] = packed_states;
    log_times[log_index] = utc_now();
    log_index++;
    if (log_index >= LOGSIZE) { // switch logfile when full
        initial = new_initial;
        log_index = 0;
        log_file_index++;
    }
    if (daymin() < last_save_daymin) {  // switch logfile on day change
        initial = new_initial;
        log_index = 0;
        log_file_index = 0;
    }
    last_save_daymin = daymin();
    save();
    new_initial = packed_states;
}

char * Output::file_name(char * filename){
    sprintf(filename, "%d.jso", log_file_index);
    return filename;
}

char * Output::dir_name(char * dirname){
        sprintf(dirname, "/data/%s/%d/%02d/%02d", name, year(), month(), day());
        return dirname;
}

void Output::load(){
    //recover last state of output history
    SERIAL.println(F("Loading relay history"));
    char filename[12];
    char dirname[50];
    aJsonObject * buff;
    aJsonObject * data_item;
    last_save_daymin = daymin();

    file_name(filename);
    dir_name(dirname);
    log_file_index = 0;
    while (file_exists(dirname, filename)){ //find first unused filename
        log_file_index++;
        file_name(filename);
    }
    SERIAL.print(F("last used file: "));
    SERIAL.print(log_file_index);
    if (log_file_index > 0) log_file_index--; //back off to last used filename
    file_name(filename);

    if (file_exists(dirname, filename)){ // Restore data
        aJsonObject * logfile = file_read(dirname, filename);

        aJsonObject* cnfobj = aJson.getObjectItem(logfile, "initial");
        if (cnfobj) {
            sscanf(cnfobj->valuestring, "%d", &initial);
        } else {
            initial = 0;
        }

        buff = aJson.getObjectItem(logfile, "state");

        if (!buff) {
#ifdef DEBUG_OUTPUT
            SERIAL.println(F("json contains no related data"));
#endif
        } else {

            data_item = buff->child;
            int idx = 0;
            time_t tstamp = 0;
            int value = 0;

            while (data_item != NULL) {
#ifdef DEBUG_OUTPUT
                SERIAL.println(F("Loading data item"));
                SERIAL.print(F("Name: "));
                SERIAL.println(data_item->name);
#endif
                sscanf(data_item->name, "%lu", &tstamp);
#ifdef DEBUG_OUTPUT
                SERIAL.print(F("iName: "));
                SERIAL.println(tstamp);
#endif
                value = data_item->valueint;
#ifdef DEBUG_OUTPUT
                SERIAL.print(F("Value: "));
                SERIAL.println(value);
                SERIAL.println(F("----"));
#endif
                log_times[idx] = tstamp;
                log_states[idx] = value;
                idx++;
                data_item = data_item->next;
            }
            log_index = idx;
        }
        aJson.deleteItem(logfile);
    }
}

void Output::json(Stream * msg){
    msg->print(F("{"));
    msg->print("\"name\":\"");
    msg->print(name);
    msg->print("\",");
    msg->print("\"initial\":\"");
    msg->print(initial);
    msg->print("\",");

    msg->print("\"state\":{");
    for (int i = 0; i < log_index; i++) {
        if (i > 0) msg->print(F(","));
        msg->print(F("\""));
        msg->print(log_times[i]);
        msg->print(F("\":"));
        msg->print(log_states[i]);
    }
    msg->print(F("}}"));
}

int Output::save(){
    char dirname[64];
    char filename[13];
    dir_name(dirname);
    file_name(filename);
    file_for_write(dirname, filename, &sd_file);
    json(&sd_file);
    sd_file.close();
    return 1;
}
