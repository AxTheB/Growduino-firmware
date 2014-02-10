#include "GrowduinoFirmware.h"
#include "RingBuffer.h"

RingBuffer::RingBuffer(int size, const char * name) {
    init(size, name);
}

RingBuffer::RingBuffer(){
    RingBuffer(0, "nul");
}

void RingBuffer::init(int size, const char * name){
    //Prepare RR buffer and clean it so we can start graphing immediatelly
    buf_len = size;
    strlcpy(buf_name, name, 4);
    if (size > 0) {
        buffer = (int*) malloc(sizeof(int) * (size));
    } else {
        buffer = NULL;
    }
    index = -1;  // points to last logged value
    last_average = MINVALUE;

    cleanup();  // fill log with MINVALuEs
}

void RingBuffer::cleanup(int start=0, int end=-1) {
    //fills parts of buffer with minvalue so we can put gaps in graph.

    if (start !=0 || end != -1) {  //Do not write to Serial on initial cleanup, as its not initialised yet
        Serial.print(F("Cleanup "));
        Serial.print(buf_name);
        Serial.print(F(" "));
        Serial.print(start);
        Serial.print(F(" "));
        Serial.print(end);
        Serial.println(F(";"));
    }

    if (end > buf_len || start == end) {
        return;
    }

    if (end == -1) {
        end = buf_len;
    }
    if(start > end) {  // when cleanup is over the end
        cleanup(start, buf_len);
        start = 0;
    }
    for (int buf_idx = start; buf_idx < end; buf_idx ++) {
        buffer[buf_idx] = MINVALUE;
    }
}

void RingBuffer::cleanup(){
    RingBuffer::cleanup(0, -1);
}

void RingBuffer::load(aJsonObject * data){
    aJsonObject * buff;
    aJsonObject * data_item;
    int i,i_end;
    int item;
    char * dValue;

    Serial.print(F("Loading buffer "));
    Serial.println(buf_name);
    buff = aJson.getObjectItem(data, buf_name);
    if (!buff) {
        Serial.println(F("json contains no related data"));
        i_end = -1;
    } else {
        i_end = aJson.getArraySize(buff);
        Serial.print(F("Array size: "));
        Serial.println(i_end);
        for(i=0; i<i_end; i++){
            data_item = aJson.getArrayItem(buff, i);
            dValue = aJson.print(data_item);
            item = atoi(dValue);

            buffer[i] = item;
            index = i;
            free(dValue);
        }
    }
    Serial.print(F("Loaded buffer "));
    Serial.print(buf_name);
    Serial.print(F(". Stored "));
    Serial.print(i_end);
    Serial.print(F(" values, index is now "));
    Serial.print(index);
    Serial.println(F("."));
}

int RingBuffer::avg(){
    int count = 0;
    long sum = 0;
    for (i=0; i < buf_len; i++) {
        if (buffer[i] != MINVALUE){
            count++;
            sum += buffer[i];
        }
    }
    last_average = sum / count;
    return last_average;
}

int RingBuffer::get_last_avg(){
    int toret = last_average;
    last_average = MINVALUE;
    return toret;
}

aJsonObject* RingBuffer::json(){
    aJsonObject *msg = aJson.createObject();
    msg = json(msg);
    return msg;
}

aJsonObject* RingBuffer::json(aJsonObject *msg) {
    aJsonObject *json = aJson.createIntArray(buffer, index + 1);
    aJson.addItemToObject(msg, buf_name, json);

    return msg;
}

aJsonObject* RingBuffer::json_dynamic(aJsonObject *msg) {
    // show buffer in non-ring fashion, last logged value in last positon
    // ie last 60 minutes of logging for "min" buffer
    int * dynamic_buffer = (int*) malloc(sizeof(int) * (buf_len));
    if (dynamic_buffer != NULL) {
        // fill-in values
        for (int j = 0; j < buf_len; j++) {
            if ((j + index + 1) < buf_len) {  // put oldest sample at dynamic_buffer[0]
                dynamic_buffer[j] = buffer[j + index + 1];
            } else {
                dynamic_buffer[j] = buffer[j + index + 1 - buf_len];
            }
        }
        aJsonObject *json = aJson.createIntArray(dynamic_buffer, buf_len);
        aJson.addItemToObject(msg, buf_name, json);

        free(dynamic_buffer);
            return msg;

    } else {  // if we are low on memry, try to return short json at last
        return json(msg);
    }
}

bool RingBuffer::store(int value, int slot){
    Serial.print(F("Storing "));
    Serial.print(value);
    Serial.print(F(" into "));
    Serial.print(buf_name);
    Serial.print(F(", slot "));
    Serial.print(slot);
    Serial.print(F(" with index "));
    Serial.println(index);
    //stay inside buffer
    //this allows us to log sequence longer than buffer, for example using
    //ordinary timestamp or day-seconds
    while (slot >= buf_len){
        slot = slot % buf_len;
    }

    //purge skipped values. Do not purge if we advanced by one or if we write
    //to the same place again
    //(needed for recalculating average for last day and month value)
    if (index != slot - 1 && index != slot){
        cleanup(index + 1, slot);
    }

    //recalculate index on turn around
    if (index > slot) {
        avg();
    }

    buffer[slot] = value;
    index = slot;
    return true;
}
