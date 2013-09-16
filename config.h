#pragma once

#include <Ethernet.h>
#include <aJSON.h>


class Config
{
    public:
        Config();
        Config(aJsonObject * json);
        int save();
        int inet_aton(const char* aIPAddrString, IPAddress& aResult);
        char * inet_ntoa(IPAddress addr, char * dest);

        int use_dhcp;
        IPAddress ip;
        IPAddress netmask;
        IPAddress gateway;
        IPAddress ntp;
};

