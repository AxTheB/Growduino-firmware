#pragma once
int analogReadAvg(int pin);

int triple_read(int (* funct)());
int triple_read(int (* funct)(int), int param1);
int return_middle(int first_value, int second_value, int third_value);

int perThousand(int pin);
