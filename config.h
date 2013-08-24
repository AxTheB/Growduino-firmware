#pragma once

struct configData {
    int port;
    int ip[4];
    byte mac[6];
};

struct configData config_init();
