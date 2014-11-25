#include "GrowduinoFirmware.h"
#include "RingBuffer.h"

void buffer_cleanup(int * buffer, int buf_len, int start, int end) {
    //fills parts of buffer with minvalue so we can put gaps in graph.
    
    //int buf_len = sizeof(&buffer)/sizeof(int);

    if (start !=0 || end != -1) {  //Do not write to Serial on initial cleanup, as its not initialised yet
#ifdef DEBUG_RB_DATA
        Serial.print(F("Cleanup:"));
        Serial.print(start);
        Serial.print(F(" - "));
        Serial.print(end);
        Serial.print(F(", size:"));
        Serial.println(buf_len);
#endif
    }

    if (end > buf_len || start == end) {
        return;
    }

    if (end == -1) {
        end = buf_len;
    }
    if(start > end) {  // when cleanup is over the end
        buffer_cleanup(buffer, buf_len, start, buf_len);
        start = 0;
    }
    for (int buf_idx = start; buf_idx < end; buf_idx ++) {
        buffer[buf_idx] = MINVALUE;
    }
}

int buffer_load(int * buffer, char * buf_name, aJsonObject * data){
    aJsonObject * buff;
    aJsonObject * data_item;
    int i,i_end;
    int item;
    char * dValue;

#ifdef DEBUG_RB_DATA
    Serial.print(F("Loading buffer "));
    Serial.println(buf_name);
#endif
    buff = aJson.getObjectItem(data, buf_name);
    if (!buff) {
#ifdef DEBUG_RB_DATA
        Serial.println(F("json contains no related data"));
#endif
        i_end = -1;
    } else {
        i_end = aJson.getArraySize(buff);
#ifdef DEBUG_RB_DATA
        Serial.print(F("Array size: "));
        Serial.println(i_end);
#endif
        for(i=0; i<i_end; i++){
            data_item = aJson.getArrayItem(buff, i);
            dValue = aJson.print(data_item);
            item = atoi(dValue);

            buffer[i] = item;
            free(dValue);
        }
        i_end--;
    }
#ifdef DEBUG_RB_DATA
    Serial.print(F("Loaded buffer "));
    Serial.print(buf_name);
    Serial.print(F(". Stored "));
    Serial.print(i_end);
    Serial.println(F(" values."));
#endif
    return i_end;
}

int buffer_avg(int * buffer, int buf_len){
    int count = 0;
    long sum = 0;
    //int buf_len = sizeof(&buffer)/sizeof(int);
    for (int i=0; i < buf_len; i++) {
        if (buffer[i] != MINVALUE){
            count++;
            sum += buffer[i];
        }
    }
    int average = sum / count;
    return average;
}

void buffer_printjson(int * buffer, int buf_len, char * buf_name, int index, bool full, Stream * output){
    // dump json of buffer into stream
    int buf_idx;
    //int buf_len = sizeof(&buffer)/sizeof(int);

#ifdef DEBUG_RB_DATA
    Serial.println(F("Debug: printjson"));
#endif
    output->print("\"");
    output->print(buf_name);
    output->print("\":[");
    if (full) {
        for (int j = 0; j < buf_len; j++) {
            if (j > 0) output->print(",");
            if ((j + index + 1) < buf_len) {  // put oldest sample at dynamic_buffer[0]
                buf_idx = j + index + 1;
            } else {
                buf_idx = j + index + 1 - buf_len;
            }
            if ((buf_idx >= 0) && (buf_idx < buf_len)) {
                output->print(buffer[buf_idx], DEC);
            } else {
#ifdef DEBUG_RB_DATA
                Serial.print(F("Err: read outside ringbuffer j:"));
                Serial.print(j, DEC);
                Serial.print(F(" index:"));
                Serial.print(index, DEC);
                Serial.print(F(" buf_len:"));
                Serial.print(buf_len, DEC);
                Serial.println();
#endif
            }
        }
    } else {
        for (int j = 0; j <= index; j++) { //print only values since start of buffer
            if (j > 0) output->print(",");
            output->print(buffer[j], DEC);
        }
    }
    output->print("]");
#ifdef DEBUG_RB_DATA
    Serial.println(F("Debug: printjson done"));
#endif
}

bool buffer_store(int * buffer, int buf_len, int value, int new_position, int old_position){
    // store 'value' into 'buffer' at 'old_position' position. Clean values between 'old_position' and last stored postition ('new_position')
#ifdef DEBUG_RB_DATA
    Serial.print(F("Storing "));
    Serial.print(value);
    Serial.print(F(", new_position "));
    Serial.print(new_position);
    Serial.print(F(" with old_position "));
    Serial.println(old_position);
#endif
    //int buf_len = sizeof(&buffer)/sizeof(int);
    //stay inside buffer
    //this allows us to log sequence longer than buffer, for example using
    //ordinary timestamp or day-seconds
    while (new_position >= buf_len){
        new_position = new_position % buf_len;
    }

    //purge skipped values. Do not purge if we advanced by one or if we write
    //to the same place again
    //(needed for recalculating average for last day and month value)
    if (old_position != new_position - 1 && old_position != new_position){
        buffer_cleanup(buffer, buf_len, old_position + 1, new_position);
    }

    buffer[new_position] = value;
    old_position = new_position;
    return true;
}
