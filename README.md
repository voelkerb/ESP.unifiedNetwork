# ESP.unifiedNetwork
Namespace to unify network (Wifi or Ethernet) code for ESP32 and ESP8266.

Requirements:
- [multiLogger](http://github.com/voelkerb/ESP.multiLogger) (-> see ```advanced.ino```)

```C++

#include "network.h"

NetworkConf config;

// Allow to e.g. load the config from EEProm and such
// TODO: Maybe add add/removeWifiAp function in the future
void initWifiConfig() {
  // A maximum of MAX_WIFI_APS is allowed
  snprintf(&config.SSIDs[0][0], MAX_NETWORK_LEN, "YourNetworkName");
  snprintf(&config.PWDs[0][0], MAX_PWD_LEN, "YourPassword");
  config.numAPs = 1;
}

void setup() {
  Serial.begin(115200);
  initWifiConfig();
  // Start network connection, will open AP if no known network found
  Network::init(&config, onWifiConnect, onWifiDisconnect);
  Serial.println("Setup done");
}

void loop() {
  ...
}

// CB If ESP connected to wifi successfully
void onWifiConnect() {
  if (not Network::apMode) {
    Serial.printf("Wifi Connected: %s ", Network::getBSSID());
    Serial.printf("IP: %s\n", Network::localIP().toString().c_str());
  } else {
    Serial.println("Network AP Opened");
  }
}


// CB If ESP disconnected from wifi
void onWifiDisconnect() {
  Serial.println("Wifi Disconnected");
}

```



This class can also be used with the an RTC.
The current time will then be gathered from the rtc object. After each successfull NTP request, the RTC time is updated. 

```C++
#include "timeHandling.h"
#include "rtc.h"
...
Rtc rtc(RTC_INT, SDA_PIN, SCL_PIN);
TimeHandler myTime(config.myConf.timeServer, LOCATION_TIME_OFFSET, &rtc, &ntpSynced);
...

```

You can also use it to get log time strings for a [multiLogger](https://github.com/voelkerb/ESP.multiLogger/) instance. See advanced example:
 ```advanced.ino```.

```bash
 - [I]02/06 08:28:16: Connecting to WiFi..
 - [I]02/06 08:28:16: Connected to the WiFi network: ******** with IP: *********
 - [D]02/06 08:28:16: Sending NTP packet...
 - [I]02/25 12:34:04: NTP Time: 02/25/2021 12:34:04.597, td 17
 - [D]02/25 12:34:04: Success NTP Time
 - [D]02/25 12:34:04: NTP synced with conf: 17
 - [I]02/25 12:34:04: Current time: 02/25/2021 12:34:04.728
 - [I]02/25 12:34:14: Another 10s passed
 - [I]02/25 12:34:14: Current time: 02/25/2021 12:34:14.000
 - [I]02/25 12:34:24: Another 10s passed
 - [I]02/25 12:34:24: Current time: 02/25/2021 12:34:24.000
```
