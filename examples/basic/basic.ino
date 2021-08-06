/***************************************************
 Example file for using the unifiedNetwork library.
 
 License: Creative Common V1. 

 Benjamin Voelker, voelkerb@me.com
 Embedded Systems Engineer
 ****************************************************/

#include "network.h"

NetworkConf config;

// Allow to e.g. load the config from EEProm and such
void initWifiConfig() {
  // A maximum of MAX_WIFI_APS is allowed
  snprintf(&config.SSIDs[0][0], MAX_NETWORK_LEN, "YourNetworkName");
  snprintf(&config.PWDs[0][0], MAX_PWD_LEN, "YourPassword");
  snprintf(&config.SSIDs[1][0], MAX_NETWORK_LEN, "YourNetworkName");
  snprintf(&config.PWDs[1][0], MAX_PWD_LEN, "YourPassword");
  snprintf(&config.name[0], MAX_DEVICE_NAME_LEN, "myESP");
  config.numAPs = 2;
}

void setup() {
  Serial.begin(115200);
  
  initWifiConfig();
  
  // Init network connection
  Network::init(&config, onWifiConnect, onWifiDisconnect);

  Serial.println("Setup done");
}

void loop() {
  #if defined(ESP8266)
  Network::update();
  #endif
  delay(1000);
  Serial.println("ping!");
}


/****************************************************
 * If ESP is connected to wifi successfully
 ****************************************************/
void onWifiConnect() {
  if (not Network::apMode) {
    Serial.printf("Wifi Connected: %s ", Network::getBSSID());
    Serial.printf("IP: %s\n", Network::localIP().toString().c_str());
  } else {
    Serial.println("Network AP Opened");
  }
}

/****************************************************
 * If ESP disconnected from wifi
 ****************************************************/
void onWifiDisconnect() {
  Serial.println("Wifi Disconnected");
}