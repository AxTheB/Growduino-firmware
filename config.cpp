#include "GrowduinoFirmware.h"
#include <Arduino.h>

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
    mac_aton("de:ad:be:ef:55:44", mac);
    ntp = IPAddress(195, 113, 56, 8);
}

void Config::load(aJsonObject * json){
    //loads values from json. Values not in json are not touched
    byte tmpmac[6];
    char debug_out[32];
    IPAddress tmpip;

    aJsonObject* cnfobj = aJson.getObjectItem(json, "use_dhcp");
    if (cnfobj) {
        if (strcmp(cnfobj->valuestring, "0") == 0 || cnfobj->valueint == 0) {
            use_dhcp = 0;
        } else {
            use_dhcp = 1;
        }
    }

    cnfobj = aJson.getObjectItem(json, "mac");
    if (cnfobj && cnfobj->type == aJson_String && mac_aton(cnfobj->valuestring, tmpmac) == 1) {
        mac_aton(cnfobj->valuestring, mac);
        Serial.print(F("mac "));
        mac_ntoa(mac, debug_out);
        Serial.println(debug_out);

    }

    if (use_dhcp == 0) {  // Static IP config
        cnfobj = aJson.getObjectItem(json, "ip");
        if (cnfobj && cnfobj->type == aJson_String && inet_aton(cnfobj->valuestring, tmpip) == 1) {
            inet_aton(cnfobj->valuestring, ip);
            Serial.print(F("ip "));
            inet_ntoa(ip, debug_out);
            Serial.println(debug_out);
        }

        cnfobj = aJson.getObjectItem(json, "netmask");
        if (cnfobj && cnfobj->type == aJson_String && inet_aton(cnfobj->valuestring, tmpip) == 1) {
            inet_aton(cnfobj->valuestring, netmask);
            Serial.print(F("netmask "));
            inet_ntoa(ip, debug_out);
            Serial.println(debug_out);
        }

        cnfobj = aJson.getObjectItem(json, "gateway");
        if (cnfobj && cnfobj->type == aJson_String && inet_aton(cnfobj->valuestring, tmpip) == 1) {
            inet_aton(cnfobj->valuestring, gateway);
            Serial.print(F("gateway "));
            inet_ntoa(gateway, debug_out);
            Serial.println(debug_out);
        }
    }

    cnfobj = aJson.getObjectItem(json, "ntp");
    if (cnfobj && cnfobj->type == aJson_String && inet_aton(cnfobj->valuestring, tmpip) == 1) {
        inet_aton(cnfobj->valuestring, ntp);
        Serial.print(F("ntp "));
        inet_ntoa(ntp, debug_out);
        Serial.println(debug_out);
    }
}

int Config::save(){
    char addr[20];
    aJsonObject * root = aJson.createObject();
    aJson.addNumberToObject(root, "use_dhcp", use_dhcp);
    mac_ntoa(mac, addr);
    aJson.addStringToObject(root, "mac", addr);
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
    unsigned int ares[4];
    int len = 0;
    ares[0] = 0;
    len = sscanf(aIPAddrString, "%d.%d.%d.%d", &ares[0], &ares[1], &ares[2], &ares[3]);

    if (len == 4) {
        // we matched 4 ints
        for(int i = 0; i < 4; i++) {
            if (ares[i] < 256) {
                // store parsed value in corresponding byte
                aResult[i] = (byte) ares[i];
            } else {
                // What we got will not fit in byte, so fail
                return 0;
            }
        }
        // now we have full address
        return 1;
    } else {
        // scanf fail
        return 0;
    }
}

char * Config::inet_ntoa(IPAddress addr, char * dest) {
    //sprintf(dest, "%d.%d.%d.%d", IPAddress[0], IPAddress[1], IPAddress[2], IPAddress[3]);
    sprintf(dest, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
    return dest;
}


int Config::mac_aton(const char * MacAddr, byte (&macResult)[6]) {
    unsigned int ares[6];
    int len = 0;
    ares[0] = 0;
    len = sscanf(MacAddr, "%x:%x:%x:%x:%x:%x", &ares[0], &ares[1], &ares[2], &ares[3], &ares[4], &ares[5]);

    if (len == 6) {
        // we matched 6 ints
        for(int i = 0; i < 6; i++) {
            if (ares[i] < 256) {
                // store parsed value in corresponding byte
                macResult[i] = (byte) ares[i];
            } else {
                // What we got will not fit in byte, so fail
                return 0;
            }
        }
        // now we have full mac address
        return 1;
    } else {
        // scanf fail
        return 0;
    }
}


char * Config::mac_ntoa(byte addr[], char * dest) {
    sprintf(dest, "%x:%x:%x:%x:%x:%x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    return dest;
}
