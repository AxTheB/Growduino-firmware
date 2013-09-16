#include "GrowduinoFirmware.h"

#include "config.h"

/*
 * Config object
 * Stores network data
 * TODO:
 * store triggers
 * store I/O names
 */


Config::Config(){
    // Simplest possible defaults
    use_dhcp = 1;
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
        if (!cnfobj || cnfobj->type != aJson_String || inet_aton(cnfobj->valuestring, ip) == 0) {
            ip = IPAddress(195, 113, 57, 67);
        }

        cnfobj = aJson.getObjectItem(json, "netmask");
        if (!cnfobj || cnfobj->type != aJson_String || inet_aton(cnfobj->valuestring, netmask) == 0) {
            netmask = IPAddress(255, 255, 255, 0);
        }

        cnfobj = aJson.getObjectItem(json, "gateway");
        if (!cnfobj || cnfobj->type != aJson_String || inet_aton(cnfobj->valuestring, gateway) == 0) {
            gateway = IPAddress(195, 113, 57, 254);
        }

        cnfobj = aJson.getObjectItem(json, "ntp");
        if (!cnfobj || cnfobj->type != aJson_String || inet_aton(cnfobj->valuestring, ntp) == 0) {
            ntp = IPAddress(195, 113, 56, 8);
        }
    }
}

int Config::save(){
    char addr[16];
    aJsonObject * root = aJson.createObject();
    aJson.addNumberToObject(root, "use_dhcp", use_dhcp);
    if (use_dhcp == 0) {
    inet_ntoa(ip, addr);
    aJson.addStringToObject(root, "ip", addr);
    inet_ntoa(netmask, addr);
    aJson.addStringToObject(root, "netmask", addr);
    inet_ntoa(gateway, addr);
    aJson.addStringToObject(root, "gateway", addr);
    inet_ntoa(ntp, addr);
    }
    aJson.addItemToObject(root, "ntp", aJson.createItem(addr));
    file_write("", "config.jso", root);
    aJson.deleteItem(root);
    return 1;
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

char * Config::inet_ntoa(IPAddress addr, char * dest) {
    //sprintf(dest, "%d.%d.%d.%d", IPAddress[0], IPAddress[1], IPAddress[2], IPAddress[3]);
    sprintf(dest, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
    return dest;
}
