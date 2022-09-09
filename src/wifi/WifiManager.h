#pragma once

#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#elif ESP8266
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Print.h>

#include "BaseManager.h"
#include "wifi/SensorProvider.h"

typedef enum {
  WF_CONNECTING = 0,
  WF_LISTENING = 1
} wifi_status;

class CWifiManager: public CBaseManager {

private:
  unsigned long tMillis;
  wifi_status status;
  char softAP_SSID[32];
  char SSID[32];
  char mqttSubcribeTopicConfig[255];
  bool rebootNeeded;
  bool postedSensorUpdate;
  
  float batteryVoltage;

  AsyncWebServer* server;
  PubSubClient mqtt;
  ISensorProvider *sensorProvider;

  StaticJsonDocument<2048> sensorJson;
  StaticJsonDocument<2048> configJson;

  void connect();
  void listen();

  void handleRoot(AsyncWebServerRequest *request);
  void handleConnect(AsyncWebServerRequest *request);
  void handleConfig(AsyncWebServerRequest *request);
  void handleFactoryReset(AsyncWebServerRequest *request);

  void printHTMLTop(Print *p);
  void printHTMLBottom(Print *p);

  void postSensorUpdate();
  bool isApMode();

  void mqttCallback(char *topic, uint8_t *payload, unsigned int);
    
public:
	CWifiManager(ISensorProvider *sp);
  virtual void loop();

  bool isRebootNeeded() { return rebootNeeded; }
  bool isJobDone() { return !isApMode() && postedSensorUpdate; }
};
