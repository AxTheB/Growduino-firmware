#pragma once

#include <Time.h>

void daytime_init();

void digitalClockDisplay();

time_t getNtpTime();
void sendNTPpacket(IPAddress &address);
