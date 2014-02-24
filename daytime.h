#pragma once

#include <Time.h>

void daytime_init();


void digitalClockDisplay();
void digitalClockDisplay(char * time);

time_t getNtpTime();
void sendNTPpacket(IPAddress &address);

int daymin();
extern int timeZone;
