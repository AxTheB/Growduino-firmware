#pragma once
#include <aJSON.h>

void buffer_cleanup(int * buffer, int buf_len, int start, int end);
int buffer_load(int * buffer, char * buf_name, aJsonObject * data);
int buffer_avg(int * buffer, int buf_len);
void buffer_printjson(int * buffer, int buf_len, char * buf_name, int index, bool full, Stream * output);
bool buffer_store(int * buffer, int buf_len, int value, int new_position, int old_position);
