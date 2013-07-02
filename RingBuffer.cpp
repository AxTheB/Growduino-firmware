#include "RingBuffer.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

RingBuffer::RingBuffer(int size, const char * name) {
    //Prepare RR buffer and clean it so we can start graphing immediatelly
    buf_len = size;
    strcpy(name_, name);
    buffer = (int*) malloc(sizeof(int) * (size));
    index = -1;
    last_average = MINVALUE;

    cleanup();
}

RingBuffer::RingBuffer(){
    RingBuffer(60, "min");
}

void RingBuffer::cleanup(int start=0, int end=-1) {
    //fills parts of buffer with minvalue so we can put gaps in graph.

    if (end > buf_len) {
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

void RingBuffer::json(char *jsonbuf){
    // Appends json to jsonbuf
    strcat(jsonbuf, "\"");
    strcat(jsonbuf, name_);
    strcat(jsonbuf, "\": [");

    for (int i=0; i <= index; i++) {
        if (buffer[i] == MINVALUE) { //This will prolly have to go out for real usage? 
            sprintf(jsontmp, ",");
        } else {
        if (i==0) {
            sprintf(jsontmp, "%d", buffer[i]);
        } else {
            sprintf(jsontmp, ",%d", buffer[i]);
        }
        }
        strcat(jsonbuf, jsontmp);
    }
    strcat(jsonbuf, "]");
}

bool RingBuffer::store(int value, int slot){
    if (slot == index) {
        return false;
    }
    //stay inside buffer
    //this allows us to log sequence longer than buffer, for example using
    //ordinary timestamp or day-seconds
    while (slot >= buf_len){
        slot = slot - buf_len;
    }

    //purge skipped values
    if (index != slot - 1){
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
