#include "GrowduinoFirmware.h"

#include <Wire.h>

#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>

#include <stdio.h>

#include <DS1307RTC.h>

#include "Lcd.h"

extern Config config;

time_t time_now;
EthernetUDP Udp;


void daytime_init() {
  tmElements_t tm;
  RTC.read(tm);
  if (RTC.chipPresent() && RTC.get() > 1400000000) {
    SERIAL.println(F("RTC: Ok."));
    setSyncProvider(RTC.get);   // the function to get the time from the RTC
    if (timeStatus() != timeSet)
      SERIAL.println(F("Unable to sync with the RTC"));
    else
      SERIAL.println(F("RTC has set the system time"));
  } else {
    lcd_print_immediate(F("RTC read failure"));
    if (RTC.chipPresent()) {
      SERIAL.println(F("The DS1307 is stopped, set time from NTP"));
      SERIAL.println();
    } else {
      SERIAL.println(F("DS1307 read error!  Please check the circuitry."));
      SERIAL.println();
    }
  }
  SERIAL.println(F("trying NTP sync"));
  unsigned int localPort = 8888;  // local port to listen for UDP packets
  Udp.begin(localPort);
  time_t ntptime;
  ntptime = getNtpTime();
  for (int i = 0; ((i < 5) && (ntptime < 1400000000)); i++) {
      delay(1000);
      ntptime = getNtpTime();
  }

  if (ntptime > 1400000000) {
    // get time from internets
    SERIAL.println(F("NTP has set the system time"));
    //setSyncProvider(getNtpTime);
    if (RTC.chipPresent()) {
      RTC.set(ntptime);
    }
  } else {
    SERIAL.println(F("Unable to reach NTP server"));
  }
  digitalClockDisplay();
}

void digitalClockDisplay(char * time) {
  // pushes current time into char * time
  sprintf(time, "%4d-%02d-%02d %02d:%02d:%02d", year(), month(), day(), hour(), minute(), second());
}

void digitalClockDisplay() {
  // prints time in readable format to serial console
  char time[21];
  digitalClockDisplay(time);
  SERIAL.println(time);
}

//-------- NTP code ----------/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  SERIAL.println(F("Transmit NTP Request"));
  sendNTPpacket(config.ntp);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 2000) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      SERIAL.println(F("Receive NTP Response"));
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + config.time_zone * SECS_PER_HOUR;
    }
  }
  SERIAL.println(F("No NTP Response :-("));
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

int daymin() {
  int daymin;
  daymin = hour() * 60 + minute();
  return daymin;
}

time_t utc_now() {
  return now() - config.time_zone * SECS_PER_HOUR;
}
