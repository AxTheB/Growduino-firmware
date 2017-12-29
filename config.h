#pragma once
#include "GrowduinoFirmware.h"

#include <Ethernet.h>
#include <aJSON.h>


class Config
{
  public:
    Config();
    void load(aJsonObject * json);
    int save();
    void loadcal(aJsonObject * json);
    int savecal();
    int inet_aton(const char* aIPAddrString, IPAddress& aResult);
    char * inet_ntoa(IPAddress addr, char * dest);
    int mac_aton(const char * MacAddr, byte (&macResult)[6]);
    char * mac_ntoa(byte addr[], char * dest);
    int time_zone;
    int ups_trigger_level;
    int co2_400;
    int co2_40k;
    int ph_4;
    int ph_7;
    int ec_low_ion;
    int ec_high_ion;
    int ec_offset;
    int use_dhcp;
    byte mac[6];
    IPAddress ip;
    IPAddress netmask;
    IPAddress gateway;
    IPAddress ntp;
    IPAddress smtp;
    int smtp_port;
    char * mail_from;
    char * sys_name;
};
