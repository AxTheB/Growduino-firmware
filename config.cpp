#include "GrowduinoFirmware.h"

struct configData config_init(){
    struct configData config;

    config.port = 80;
    config.ip[0] = 192;
    config.ip[0] = 168;
    config.ip[0] = 0;
    config.ip[0] = 44;

    config.mac[0] = 0xDE;
    config.mac[0] = 0xAD;
    config.mac[0] = 0xBE;
    config.mac[0] = 0xEF;
    config.mac[0] = 0x55;
    config.mac[0] = 0x44;

    aJsonObject * storedConfig;
    storedConfig = file_read("", "config.jso");
    if (storedConfig != NULL) {
        aJsonObject *ipaddr = aJson.getObjectItem(storedConfig, "ip");
    }

    return config;

}
