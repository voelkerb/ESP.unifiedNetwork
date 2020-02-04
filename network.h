/***************************************************
 Library for network stuff, connection, AP and so on.

 Feel free to use the code as it is.

 Benjamin Völker, voelkerb@me.com
 Embedded Systems
 University of Freiburg, Institute of Informatik
 ****************************************************/

#ifndef NETWORK_h
#define NETWORK_h

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
// To trigger things regularly
#include <Ticker.h> 
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
    extern bool ethernet;

    void init(Configuration * config);
    void init(Configuration * config, void (*onConnect)(void), void (*onDisconnect)(void));
    bool update();
    bool connect(char * network, char * pswd);
    void setupAP();
    IPAddress localIP();
    
    // void onSoftAPModeStationConnected(const WiFiEventSoftAPModeStationConnected& evt);
    // void onSoftAPModeStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& evt);
    // void onStationModeGotIP(const WiFiEventStationModeGotIP& evt);
    // void onStationConnected(const WiFiEventSoftAPModeStationConnected& evt);
    // void onStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& evt);

    void checkNetwork();
    void scanNetwork( void * pvParameters );

}

#endif
