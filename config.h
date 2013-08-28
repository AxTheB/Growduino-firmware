#pragma once

#include <Ethernet.h>
#include <aJSON.h>


class Config
{
    public:
        Config();
        Config(aJsonObject * json);
        int use_dhcp;
        IPAddress ip;
        IPAddress netmask;
        IPAddress gateway;
        IPAddress ntp;
        int inet_aton(const char* aIPAddrString, IPAddress& aResult);
        char * iptos(IPAddress addr, char * dest);
};

