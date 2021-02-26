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
  bool preConnected = false;
  bool apMode = false;
  bool staConnected = false;
  bool allowNetworkChange = true;
  bool ethernet = false;
  #define MAX_MAC_LEN 19
  char bssid[MAX_MAC_LEN] = {'\0'};
  
  MultiLogger * logger = NULL; 
  static NetworkConf * _config;
  static void (*_onConnect)(void);
  static void (*_onDisconnect)(void);

#if defined(ESP8266)
  static Ticker checker;
  WiFiEventHandler stationModeConnectedHandler;
  WiFiEventHandler stationModeDisconnectedHandler;
  WiFiEventHandler stationModeGotIPHandler;
  WiFiEventHandler softAPModeStationConnectedHandler;
  WiFiEventHandler softAPModeStationDisconnectedHandler;

  void onSoftAPModeStationConnected(const WiFiEventSoftAPModeStationConnected& evt) {
    connected = true;
    apMode = true;
    if (logger != NULL) logger->log("AP_START");
    if (_onConnect) _onConnect();
  }

  void onSoftAPModeStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& evt) {
    connected = false;
    apMode = false;
    if (logger != NULL) logger->log("AP_STOP");
  }

  void onStationModeGotIP(const WiFiEventStationModeGotIP& evt) {
    connected = true;
    apMode = false;
    checker.detach();
    if (logger != NULL) logger->log("STA_GOT_IP");
    if (_onConnect) _onConnect();
  }

  void onStationModeConnected(const WiFiEventStationModeConnected& evt) {
    apMode = false;
    connected = true;
    if (logger != NULL) logger->log("STA_CONNECTED");
  }
  
  void onStationModeDisconnected(const WiFiEventStationModeDisconnected& evt) {
    connected = false;
    apMode = false;
    // lets check for networks regularly
    checker.attach(CHECK_PERIODE_MS/1000, checkNetwork);
    if (logger != NULL) logger->log("STA_DISCONNECTED");
    if (_onDisconnect) _onDisconnect();
    setupAP();
  }
   
  void init(NetworkConf * config, void (*onConnect)(void), void (*onDisconnect)(void), bool usingEthernet, MultiLogger * the_logger) {
    _config = config;
    _onConnect = onConnect;
    _onDisconnect = onDisconnect;
    ethernet = usingEthernet;
    connected = false;
    logger = the_logger;

    stationModeConnectedHandler          = WiFi.onStationModeConnected(&onStationModeConnected);
    stationModeDisconnectedHandler       = WiFi.onStationModeDisconnected(&onStationModeDisconnected);
    stationModeGotIPHandler              = WiFi.onStationModeGotIP(&onStationModeGotIP);
    softAPModeStationConnectedHandler    = WiFi.onSoftAPModeStationConnected(&onSoftAPModeStationConnected);
    softAPModeStationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(&onSoftAPModeStationDisconnected);


    // Forget any previously set configuration
    // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    // Disconnect if we were connected
    // WiFi.disconnect();
    WiFi.hostname(_config->name);
    WiFi.mode(WIFI_STA);
    // Disable wifi power saving
    wifi_set_sleep_type(NONE_SLEEP_T);
    checkNetwork();
    checker.attach(CHECK_PERIODE_MS/1000, checkNetwork);
  }

  void scanNetwork() {
    // blocking call
    // NOTE: non blocking call did not work properly
    WiFi.mode(WIFI_AP_STA);
    size_t n = WiFi.scanNetworks();
    int found = -1;
    if (logger != NULL) {
      logger->log("Scan done %u networks found", n);
      if (n != 0) {
        for (size_t i = 0; i < n; ++i) {
          if (logger != NULL) logger->log("%s (%i)", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
          // Print SSID and RSSI for each network found
          // if (logger != NULL) logger->append("%s (%i)", WiFi.SSID(i), WiFi.RSSI(i));
          // if (i < n-1) if (logger != NULL) logger->append(", ");
        }
        // if (logger != NULL) logger->flush();
      }
    }

    found = -1;
    // Only if we have found any network, we can search for a known one
    if (n != 0) {
      int linkQuality = -1000; // The smaller the worse the quality (in dBm)
      for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < _config->numAPs; j++) {
          if (strcmp(WiFi.SSID(i).c_str(), _config->SSIDs[j]) == 0) {
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
      if (logger != NULL) logger->log("Strongest known network: %s", _config->SSIDs[found]);
      // connect
      WiFi.mode(WIFI_STA);
      WiFi.begin(_config->SSIDs[found], _config->PWDs[found]);
    } else {
      if (logger != NULL) logger->log(WARNING, "No known network");
      // We setup an access point
      setupAP();
    }
  }

  void checkNetwork() {
    // for compatibility reason the extra function
    scanNetwork();
  }
#endif

#if defined(ESP32)
  void wifiEvent(WiFiEvent_t event) {
    #ifdef DEBUG_DEEP
    Serial.printf("Info:[WiFi-event] event: %d\n", event);
    switch (event) {
      case SYSTEM_EVENT_WIFI_READY:
        Serial.println("Info:WiFi interface ready");
        break;
      case SYSTEM_EVENT_SCAN_DONE:
        Serial.println("Info:Completed scan for access points");
        break;
      case SYSTEM_EVENT_STA_START:
        Serial.println("Info:WiFi client started");
        break;
      case SYSTEM_EVENT_STA_STOP:
        Serial.println("Info:WiFi clients stopped");
        break;
      case SYSTEM_EVENT_STA_CONNECTED:
        Serial.println("Info:Connected to access point");
        break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("Info:Disconnected from WiFi access point");
        break;
      case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
        Serial.println("Info:Authentication mode of access point has changed");
        break;
      case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("Info:Obtained IP address");
        break;
      case SYSTEM_EVENT_STA_LOST_IP:
        Serial.println("Info:Lost IP address and IP address is reset to 0");
        break;
      case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
        Serial.println("Info:WiFi Protected Setup (WPS): succeeded in enrollee mode");
        break;
      case SYSTEM_EVENT_STA_WPS_ER_FAILED:
        Serial.println("Info:WiFi Protected Setup (WPS): failed in enrollee mode");
        break;
      case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
        Serial.println("Info:WiFi Protected Setup (WPS): timeout in enrollee mode");
        break;
      case SYSTEM_EVENT_STA_WPS_ER_PIN:
        Serial.println("Info:WiFi Protected Setup (WPS): pin code in enrollee mode");
        break;
      case SYSTEM_EVENT_AP_START:
        Serial.println("Info:WiFi access point started");
        break;
      case SYSTEM_EVENT_AP_STOP:
        Serial.println("Info:WiFi access point stopped");
        break;
      case SYSTEM_EVENT_AP_STACONNECTED:
        Serial.println("Info:Client connected");
        break;
      case SYSTEM_EVENT_AP_STADISCONNECTED:
        Serial.println("Info:Client disconnected");
        break;
      case SYSTEM_EVENT_AP_STAIPASSIGNED:
        Serial.println("Info:Assigned IP address to client");
        break;
      case SYSTEM_EVENT_AP_PROBEREQRECVED:
        Serial.println("Info:Received probe request");
        break;
      case SYSTEM_EVENT_GOT_IP6:
        Serial.println("Info:IPv6 is preferred");
        break;
      case SYSTEM_EVENT_ETH_START:
        Serial.println("Info:Ethernet started");
        break;
      case SYSTEM_EVENT_ETH_STOP:
        Serial.println("Info:Ethernet stopped");
        break;
      case SYSTEM_EVENT_ETH_CONNECTED:
        Serial.println("Info:Ethernet connected");
        break;
      case SYSTEM_EVENT_ETH_DISCONNECTED:
        Serial.println("Info:Ethernet disconnected");
        break;
      case SYSTEM_EVENT_ETH_GOT_IP:
        Serial.println("Info:Ethernet obtained IP address");
        break;
      default:
        Serial.println("Info:Unknown event");
    }
    #endif

    if (logger != NULL) {
      if (ethernet) {
        switch (event) {
          case SYSTEM_EVENT_ETH_START: 
            logger->log("ETH Started");
            break;
          case SYSTEM_EVENT_ETH_CONNECTED:
            logger->log("ETH Connected");
            break;
          case SYSTEM_EVENT_ETH_GOT_IP:
            logger->log("ETH MAC: %s, IP: %s, Speed: %iMbps", ETH.macAddress().c_str(), ETH.localIP().toString().c_str(), ETH.linkSpeed());
            break;
          case SYSTEM_EVENT_ETH_DISCONNECTED:
            logger->log("ETH Disconnected");
            break;
          case SYSTEM_EVENT_ETH_STOP:
            logger->log("ETH Stopped");
            break;
          default:
            break;
        }
      } else {
        switch (event) {
          case SYSTEM_EVENT_SCAN_DONE:
            break;
          case SYSTEM_EVENT_STA_CONNECTED:
            logger->log("STA_CONNECTED");
            break;
          case SYSTEM_EVENT_STA_DISCONNECTED:
            // lets check for networks regularly
            logger->log("STA_DISCONNECTED");
            break;
          case SYSTEM_EVENT_STA_GOT_IP:
            logger->log("STA_GOT_IP");
            break;
          case SYSTEM_EVENT_AP_START:
            if (apMode) break;
            logger->log("AP_START");
            break;
          case SYSTEM_EVENT_AP_STOP:
            if (not apMode) break;
            logger->log("AP_STOP");
            break;
          default:
            break;
        }
      }
    }
    if (ethernet) {
      switch (event) {
        case SYSTEM_EVENT_ETH_START:
          //set eth hostname here
          ETH.setHostname(_config->name);
          break;
        case SYSTEM_EVENT_ETH_CONNECTED:
          preConnected = true;
          break;
        case SYSTEM_EVENT_ETH_GOT_IP:
          connected = true;
          if (_onConnect) _onConnect();
          break;
        case SYSTEM_EVENT_ETH_DISCONNECTED:
          connected = false;
          if (_onDisconnect) _onDisconnect();
          break;
        case SYSTEM_EVENT_ETH_STOP:
          connected = false;
          break;
        default:
          break;
      }
    } else {
      switch (event) {
        case SYSTEM_EVENT_SCAN_DONE:
          break;
        case SYSTEM_EVENT_STA_CONNECTED:
          apMode = false;
          connected = false;
          preConnected = true;
          break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
          // Recheck for wifi networks
          if (connected and apMode == false) checkNetwork();
          connected = false;
          preConnected = false;
          apMode = false;
          // lets check for networks regularly
          if (_onDisconnect) _onDisconnect();
          break;
        case SYSTEM_EVENT_STA_GOT_IP:
          WiFi.softAPdisconnect(false);
          WiFi.mode(WIFI_STA);
          connected = true;
          staConnected = true;
          apMode = false;
          if (_onConnect) _onConnect();
          break;
        case SYSTEM_EVENT_AP_START:
          if (apMode) break;
          connected = true;
          apMode = true;
          if (_onConnect) _onConnect();
          break;
        case SYSTEM_EVENT_AP_STOP:
          if (not apMode) break;
          connected = false;
          apMode = false;
          break;
        default:
          break;
      }
    }
  }


  void init(NetworkConf * config, void (*onConnect)(void), void (*onDisconnect)(void), bool usingEthernet, MultiLogger * the_logger) {
    _config = config;
    _onConnect = onConnect;
    _onDisconnect = onDisconnect;
    ethernet = usingEthernet;
    connected = false;
    logger = the_logger;
    // We do not need bluetooth, so disable it
    esp_bt_controller_disable();
    // Forget any previously set configuration
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    // Disconnect if we were connected
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.onEvent(&wifiEvent);
    if (ethernet) {
      apMode = false;
      WiFi.mode(WIFI_OFF);
      btStop();
      // ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
    } else {
      WiFi.setHostname(_config->name);
      // Disable wifi power saving
      esp_wifi_set_ps(WIFI_PS_NONE);
      checkNetwork();
    }
  }

#ifdef _ETH_H_
  void initPHY(uint8_t addr, uint8_t pwr, uint8_t mdc, uint8_t mdio, eth_phy_type_t type, eth_clock_mode_t clk_mode) {
      ETH.begin(addr, pwr, mdc, mdio, type, clk_mode);
  }
#endif

  void scanNetwork( void * pvParameters ) {
    // Indicating if we are connected to a wifi station
    staConnected = false;
    bool apInited = false;

    while (not staConnected) {
      if (preConnected or not allowNetworkChange) {
        vTaskDelay(CHECK_PERIODE_MS); 
        continue;
      } 
      // If already connected
      // This is not true for AP mode
      if (WiFi.status() == WL_CONNECTED) {
        staConnected = true;
        break;
      }
      if (logger != NULL) logger->log(INFO, "Scanning for Wifi Networks");
      int n = WiFi.scanNetworks();
      if (n == -2) {
        staConnected = false;
        apInited = false;
        if (logger != NULL) logger->log(ERROR, "Scan failed");
        //  Give it up ...
        ESP.restart();
        // Reconfigure wifi
        // Delete results of previous scan
        WiFi.scanDelete();
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        WiFi.disconnect();
        WiFi.mode(WIFI_STA);
        WiFi.onEvent(&wifiEvent);
        esp_wifi_set_ps(WIFI_PS_NONE);
      } else if (n == -1) {
        WiFi.scanDelete();
        if (logger != NULL) logger->log(WARNING, "Scan already in progress");
        //  Give it up ...
        ESP.restart();
      } else if (n == 0) {
        WiFi.scanDelete();
        if (logger != NULL) logger->log(INFO, "No network found");
      } else {
        if (logger != NULL) logger->log("Scan done %i networks found", n);
        for (size_t i = 0; i < (size_t)n; ++i) {
          if (logger != NULL) logger->log("%s (%i)", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
        }
      }

      int found = -1;
      // Only if we have found any network, we can search for a known one
      if (n > 0) {
        int linkQuality = -1000; // The smaller the worse the quality (in dBm)
        for (size_t i = 0; i < (size_t)n; ++i) {
          for (size_t j = 0; j < _config->numAPs; j++) {
            if (strcmp(WiFi.SSID(i).c_str(), _config->SSIDs[j]) == 0) {
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
        if (logger != NULL) logger->log("Strongest known network: %s", _config->SSIDs[found]);
        if (connect(_config->SSIDs[found], _config->PWDs[found])) {
          staConnected = true;
        } else {
          apInited = false;
        }
      } else {
        if (logger != NULL) logger->log(WARNING, "No known network");
      }

      if (not staConnected and not apInited) {
        // We setup an access point
        setupAP();
        apInited = true;
      }
      // Delete results of previous scan
      WiFi.scanDelete();
      // If we are connected break, otherwise wait and continue with next scan
      if (staConnected) break;
      vTaskDelay(CHECK_PERIODE_MS);
    }
    vTaskDelete( NULL );
  }

  void checkNetwork() {
    // let it check for networks on second core (not in loop)
    xTaskCreatePinnedToCore(
                      scanNetwork,   /* Function to implement the task */
                      "scanTask", /* Name of the task */
                      10000,      /* Stack size in words */
                      NULL,       /* Task input parameter */
                      1,          /* Priority of the task */
                      NULL,       /* Task handle. */
                      1);  /* Core where the task should run */

  }
#endif

  void init(NetworkConf * config) {
    init(config, NULL, NULL);
  }


  char * getBSSID() {
    if (connected and not apMode) {
      snprintf(bssid, MAX_MAC_LEN, "%s", WiFi.BSSIDstr().c_str());
    } else {
      snprintf(bssid, MAX_MAC_LEN, "-");
    }
    return &bssid[0];
  }


  IPAddress localIP() {
#if defined(ESP32)
    if (ethernet) return ETH.localIP();
#endif
    return WiFi.localIP();
  }

  bool connect(char * network, char * pswd) {
    if (logger != NULL) logger->log("Connecting to: %s", network);
    WiFi.mode(WIFI_STA);
    WiFi.begin(network, pswd);
    // Set the hostname
    #if defined(ESP8266)
    WiFi.hostname(_config->name);
    #else
    WiFi.setHostname(_config->name);
    #endif
    long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
      yield();
      // After trying to connect for 8s continue without wifi
      if (preConnected) {
        if (millis() - start > 8000) {
          if (logger != NULL) logger->log(ERROR, "Cannot get IP from %s", network);
          return false;
        }
      } else {
        if (millis() - start > 5000) {
          if (logger != NULL) logger->log(ERROR, "Connection to %s failed!", network);
          return false;
        }
      }
      delay(100);
    }
    return true;
  }

  void setupAP() {
    WiFi.mode(WIFI_AP_STA);
    if (logger != NULL) logger->log("Setting up AP: %s", _config->name);
    WiFi.softAP(_config->name);
  }

}
