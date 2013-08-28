#include "GrowduinoFirmware.h"

#include "config.h"

Config::Config(){
    use_dhcp = 1;
    ntp = IPAddress(195, 113, 56, 8);
}

Config::Config(aJsonObject * json){
    
    aJsonObject* cnfobj = aJson.getObjectItem(json, "use_dhcp");
    if (!cnfobj) {
        use_dhcp = 1;
    } else {
        use_dhcp = cnfobj->valueint;
    }

    if (use_dhcp == 0) {  // Static IP config
        aJsonObject* cnfobj = aJson.getObjectItem(json, "ip");
        if (!cnfobj || cnfobj->type != aJson_String) {
            ip = IPAddress(10, 0, 0, 55);
        } else {
            inet_aton(cnfobj->valuestring, ip);
        }

        cnfobj = aJson.getObjectItem(json, "netmask");
        if (!cnfobj || cnfobj->type != aJson_String) {
            netmask = IPAddress(255, 255, 0, 0);
        } else {
            inet_aton(cnfobj->valuestring, netmask);
        }

        cnfobj = aJson.getObjectItem(json, "gateway");
        if (!cnfobj || cnfobj->type != aJson_String) {
            gateway = IPAddress(10, 0, 0, 1);
        } else {
            inet_aton(cnfobj->valuestring, gateway);
        }

        cnfobj = aJson.getObjectItem(json, "ntp");
        if (!cnfobj || cnfobj->type != aJson_String) {
            ntp = IPAddress(195, 113, 56, 8);
        } else {
            inet_aton(cnfobj->valuestring, ntp);
        }
    }
}

int Config::inet_aton(const char* aIPAddrString, IPAddress& aResult) {
    // Copy of DNSClient::inet_aton which I do not want to include
    // See if we've been given a valid IP address
    const char* p =aIPAddrString;
    while (*p &&
           ( (*p == '.') || (*p >= '0') || (*p <= '9') ))
    {
        p++;
    }

    if (*p == '\0')
    {
        // It's looking promising, we haven't found any invalid characters
        p = aIPAddrString;
        int segment =0;
        int segmentValue =0;
        while (*p && (segment < 4))
        {
            if (*p == '.')
            {
                // We've reached the end of a segment
                if (segmentValue > 255)
                {
                    // You can't have IP address segments that don't fit in a byte
                    return 0;
                }
                else
                {
                    aResult[segment] = (byte)segmentValue;
                    segment++;
                    segmentValue = 0;
                }
            }
            else
            {
                // Next digit
                segmentValue = (segmentValue*10)+(*p - '0');
            }
            p++;
        }
        // We've reached the end of address, but there'll still be the last
        // segment to deal with
        if ((segmentValue > 255) || (segment > 3))
        {
            // You can't have IP address segments that don't fit in a byte,
            // or more than four segments
            return 0;
        }
        else
        {
            aResult[segment] = (byte)segmentValue;
            return 1;
        }
    }
    else
    {
        return 0;
    }
}

char * Config::iptos(IPAddress addr, char * dest) {
    //sprintf(dest, "%d.%d.%d.%d", IPAddress[0], IPAddress[1], IPAddress[2], IPAddress[3]);
    sprintf(dest, "%d.%d.%d.%d", 1, 1, 2, 3);
    return dest;
}

