/***************************************************
 Library for network stuff, connection, AP and so on.

 Feel free to use the code as it is.

 Benjamin Völker, voelkerb@me.com
 Embedded Systems
 University of Freiburg, Institute of Informatik
 ****************************************************/

#include "network.h"

namespace Network
{
  bool connected = false;
  bool apMode = false;
  MultiLogger& logger = MultiLogger::getInstance();

  static Configuration * _config;
  static void (*_onConnect)(void);
  static void (*_onDisconnect)(void);
  static Ticker checker;

  WiFiEventHandler stationModeConnectedHandler;
  WiFiEventHandler stationModeDisconnectedHandler;
  WiFiEventHandler stationModeGotIPHandler;
  WiFiEventHandler softAPModeStationConnectedHandler;
  WiFiEventHandler softAPModeStationDisconnectedHandler;


  void onSoftAPModeStationConnected(const WiFiEventSoftAPModeStationConnected& evt) {
    connected = true;
    apMode = true;
    logger.log("AP_START");
    if (_onConnect) _onConnect();
  }

  void onSoftAPModeStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& evt) {
    connected = false;
    apMode = false;
    logger.log("AP_STOP");
  }

  void onStationModeGotIP(const WiFiEventStationModeGotIP& evt) {
    connected = true;
    apMode = false;
    checker.detach();
    logger.log("STA_GOT_IP");
    if (_onConnect) _onConnect();
  }

  void onStationModeConnected(const WiFiEventStationModeConnected& evt) {
    apMode = false;
    connected = true;
    logger.log("STA_CONNECTED");
  }
  
  void onStationModeDisconnected(const WiFiEventStationModeDisconnected& evt) {
    connected = false;
    apMode = false;
    // lets check for networks regularly
    checker.attach(CHECK_PERIODE, checkNetwork);
    logger.log("STA_DISCONNECTED");
    if (_onDisconnect) _onDisconnect();
    setupAP();
  }
   


  void init(Configuration * config) {
    init(config, NULL, NULL);
  }


  void init(Configuration * config, void (*onConnect)(void), void (*onDisconnect)(void)) {
    _config = config;
    _onConnect = onConnect;
    _onDisconnect = onDisconnect;
    connected = false;
    WiFi.disconnect(true);
    // Line maybe causes error
    // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    
    stationModeConnectedHandler          = WiFi.onStationModeConnected(&onStationModeConnected);
    stationModeDisconnectedHandler       = WiFi.onStationModeDisconnected(&onStationModeDisconnected);
    stationModeGotIPHandler              = WiFi.onStationModeGotIP(&onStationModeGotIP);
    softAPModeStationConnectedHandler    = WiFi.onSoftAPModeStationConnected(&onSoftAPModeStationConnected);
    softAPModeStationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(&onSoftAPModeStationDisconnected);


    WiFi.hostname(_config->name);
    WiFi.mode(WIFI_AP_STA);
    // Disable wifi power saving
    wifi_set_sleep_type(NONE_SLEEP_T);
    checkNetwork();
    checker.attach(CHECK_PERIODE, checkNetwork);
  }

  IPAddress localIP() {
    return WiFi.localIP(); 
  }

  bool connect(char * network, char * pswd) {
    logger.log("Connecting to: %s", network);
    WiFi.mode(WIFI_STA);
    WiFi.begin(network, pswd);
    // Set the hostname
    WiFi.hostname(_config->name);
    long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
      yield();
      // After trying to connect for 8s continue without wifi
      if (millis() - start > 8000) {
        logger.log(ERROR, "Connection to %s failed!", network);
        return false;
      }
    }
    return true;
  }

  void setupAP() {
    WiFi.mode(WIFI_AP_STA);
    apMode = true;
    logger.log("Setting up AP: %s", _config->name);
    WiFi.softAP(_config->name);
    // Check for known networks regularly
    // checker.detach();
  }

  void scanNetwork() {
    // blocking call
    // NOTE: non blocking call did not work properly
    WiFi.mode(WIFI_AP_STA);
    size_t n = WiFi.scanNetworks();
    int found = -1;
    logger.log("Scan done %u networks found", n);
    if (n != 0) {
      for (size_t i = 0; i < n; ++i) {
        logger.log("%s (%i)", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
        // Print SSID and RSSI for each network found
        // logger.append("%s (%i)", WiFi.SSID(i), WiFi.RSSI(i));
        // if (i < n-1) logger.append(", ");
      }
      // logger.flush();
    }

    found = -1;
    // Only if we have found any network, we can search for a known one
    if (n != 0) {
      int linkQuality = -1000; // The smaller the worse the quality (in dBm)
      for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < _config->numAPs; j++) {
          if (strcmp(WiFi.SSID(i).c_str(), _config->wifiSSIDs[j]) == 0) {
            if (WiFi.RSSI(i) > linkQuality) {
              linkQuality = WiFi.RSSI(i);
              found = j;
            }
            break; // Only break inner for loop, to check rest for better rssi
          }
        }
      }
    }

    if (found != -1) {
      logger.log("Strongest known network: %s", _config->wifiSSIDs[found]);
      // connect
      WiFi.mode(WIFI_STA);
      WiFi.begin(_config->wifiSSIDs[found], _config->wifiPWDs[found]);
    } else {
      logger.log(WARNING, "No known network");
      // We setup an access point
      setupAP();
    }
  }

  void checkNetwork() {
    // for compatibility reason the extra function
    scanNetwork();
  }
}
