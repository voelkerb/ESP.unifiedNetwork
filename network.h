/***************************************************
 Library for network stuff, connection, AP and so on.

 Feel free to use the code as it is.

 Benjamin Völker, voelkerb@me.com
 Embedded Systems
 University of Freiburg, Institute of Informatik
 ****************************************************/

#ifndef NETWORK_h
#define NETWORK_h

// Wifi stuff
#include <esp_wifi.h>
#include <WiFi.h>
#include <WiFiAP.h>
// Ethernet stuff
#include <ETH.h>
// To trigger things regularly
#include <Ticker.h> 
// Both to set bt to sleep
#include <esp_sleep.h>
#include <esp_bt.h>
// Logging and configuration stuff
#include "../logger/logger.h"
#include "../config/config.h"

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
// #define DEBUG_DEEP
#define CHECK_PERIODE 60

namespace Network {
    extern MultiLogger& logger;
    extern bool connected;
    extern bool apMode;
    extern bool _ethernet;

    void init(Configuration * config);
    void init(Configuration * config, void (*onConnect)(void), void (*onDisconnect)(void), bool ethernet=false);
    bool update();
    bool connect(char * network, char * pswd);
    void setupAP();
    IPAddress localIP();
    void wifiEvent(WiFiEvent_t event);
    void checkNetwork();
    void scanNetwork( void * pvParameters );

}

#endif
