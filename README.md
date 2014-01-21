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
/sensors/{tempX,humidity,light,...}.jso
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
 - t_since and t_until are in minutes since midnight, and define when the conditions are checked. For all-day trigger use t_since = -1. When t_since > t_until the trigger is checked over midnight.
 - when trigger turns on any output, it can be turned off by any of following:
    - the off_value condition of said trigger is met
    - t_until of said trigger is encountered
    - off_value with important (!) mark is true.
 - on_value and off_value are in format operator+parameter+importance, see examples.
 - off_value condition has precedence over on_value
 - Possible operators for on/off_value are:
    - "<": Lesser than. "on_value":"<10" on temp readings will resolve as true. Parameter is raw sensor reading
    - ">": Greater than. Opposite to "<".
    - "T": resolves true when the output was off for more than parameter minutes, switches off after off_value minutes has passed. See example 2.
 - Importance is noted by "!" at end, and means that this condition is critical, skips any other triggers relating to this output
 - sensors are (indexed from zero):
    - humidity
    - temperature
    - light
    - ultrasound
    - Dallas one wire devices
 - outputs are indexed from zero too

Ex. 1: Switch on output 7 when temperature falls to 25 degrees C, switch it back off when it climbs over 30C

```json
    {
        "t_since":-1,
        "t_until":0,
        "on_value":"<250",
        "off_value":">300",
        "sensor":1,
        "output":7,
    }
```

Ex. 2: Never turn on output 5, if the temperature is bellow 10C:

```json
    {
        "t_since":-1,
        "t_until":0,
        "on_value":">100",
        "off_value":"<100!",
        "sensor":1,
        "output":5,
    }
```

Ex. 3: During the night (since 8pm to 6am), when the output 4 was idle for 10 minutes, run it for 5 minutes:

```json
    {
        "t_since":1200,
        "t_until":360,
        "on_value":"T10",
        "off_value":"T5",
        "sensor":-1,
        "output":4,
    }
```

Historical data
-------------
/data/
 - stores historical data for all sensors and output
 - /data/SENSOR/YYYY/MM.jso stores daily averages for given month
 - /data/SENSOR/YYYY/MM/DD.jso stores hourly averages for given day
 - /data/SENSOR/YYYY/MM/DD/HH.jso stores data for each hour.

/data/output/YYYY/MM/DD/XX.jso stores log of sensor changes, split by 50 records. GET the sequence until 404 is encountered. TODO
