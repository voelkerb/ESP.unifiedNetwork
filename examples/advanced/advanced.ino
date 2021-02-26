/***************************************************
 Example file for using the unifiedNetwork library.
 
 License: Creative Common V1. 

 Benjamin Voelker, voelkerb@me.com
 Embedded Systems Engineer
 ****************************************************/

#include "multiLogger.h"
#include "network.h"

// Printed to console in front of log text to indicate logging
char * LOG_PREFIX_SERIAL = "";

// Create singleton here
MultiLogger& logger = MultiLogger::getInstance();
StreamLogger serialLog((Stream*)&Serial, NULL, &LOG_PREFIX_SERIAL[0], DEBUG);

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
  // Add seriallogger to multilogger
  logger.addLogger(&serialLog);
  initWifiConfig();
  
  // Init network connection
  Network::init(&config, onWifiConnect, onWifiDisconnect, false, &logger);

  logger.log("Setup done");
}

void loop() {
  delay(1000);
  logger.log("ping!");
}


/****************************************************
 * If ESP is connected to wifi successfully
 ****************************************************/
void onWifiConnect() {
  if (not Network::apMode) {
    logger.log("Wifi Connected: %s ", Network::getBSSID());
    logger.log("IP: %s\n", Network::localIP().toString().c_str());
  } else {
    logger.log(WARNING, "Network AP Opened");
  }
}

/****************************************************
 * If ESP disconnected from wifi
 ****************************************************/
void onWifiDisconnect() {
 logger.log("Wifi Disconnected");
}