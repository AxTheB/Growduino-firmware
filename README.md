Growduino-firmware api spec 0.1
==================
All URLs that correspond to files are in 8.3 form. Use only lowercase in urls.

*/sensors/* is 'live' area, URLs here do not correspond to file on card. 

When there is any data missing, its filled by -999 (MINVALUE from GrowduinoFirmware.h) TODO: Replace -999 with json NULL

Web client configuration
------------------------
/client.jso
 - Configuration data for web client, not used in firmware
 - supports GET, POST
 - FIXME: atm the json is parsed on file write

Firmware configuration
----------------------
/config.jso
 - Configuration data for firmware
 - supports GET, POST
 - If use_dhcp == 1 then get IP config from dhcp, if not then use values from json
 - TODO: use config values when dhcp fails
 - changes take effect on reboot

```json
    {
        "use_dhcp":1,
        "ip":"195.113.57.69",
        "netmask":"255.255.255.0",
        "gateway":"195.113.57.254",
        "mac":"de:ad:be:ef:55:44",
        "ntp":"195.113.56.8"
    }
```

Outputs (relay) state
---------------------
/sensors/outputs.jso
 - Stores last 60 minutes of sensor state
 - each value displays state after triggers run, one per minute
 - Values are bitfield, each bit corresponds to one output
 - supports GET only
 - soon to be obsoleted

```json
    {
        "name":"outputs",
        "min":[
            160,
            160,
            ...
            160
        ]
    }
```

/sensors/outputs.jso [Soon, subject to change]
 - Stores last 50 output positions
 - Values are bitfield, each bit corresponds to one output
 - supports GET only

```json
    {
        "name":"outputs",
        "state":{
            "1390173000":160,
            "1390173060":161,
            ...
            "1390173300":255
        }
    }
```

Sensor state
------------
/sensors/{tempX,humidity,light}.jso
 - shows last 60 minutes of sensor data in "min" array, with hourly averages for last day in "h" array and daily averages since boot in "day"
 - Values are to be divided by 10 before using

```json
     {
        "name":"Light",
        "min":[13,...,23],
        "h":[48,...,20],
        "day":[21,...,101]
    }
```

Trigger configuration
---------------------
/triggers/X.jso
 - X is 0 to TRIGGERS (GrowduinoFirmware.h, line 22)
 - supports GET, POST
 - changes take effect immediately
 - t_since and t_until are in minutes since midnight
 - on_value and off_value are raw sensor readings
 - sensors are (indexed from zero):
    - humidity
    - temperature
    - light
 - outputs are indexed from zero too

```json
    {
        "t_since":0,
        "t_until":0,
        "on_value":"<255",
        "off_value":">512",
        "sensor":-1,
        "output":7,
    }
```

