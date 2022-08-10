#if !( defined(ESP32) ) && !( defined(ESP8266) )
  #error This code is intended to run on ESP8266 or ESP32 platform!
#endif

#include <Arduino.h>
#include <WiFiClient.h>
//#include <ezTime.h>
#include <AsyncElegantOTA.h>

#include "wifi/WifiManager.h"
#include "Configuration.h"

#define MAX_CONNECT_TIMEOUT_MS 15000 // 10 seconds to connect before creating its own AP
#define BOARD_LED_PIN 2

const int RSSI_MAX =-50;// define maximum straighten of signal in dBm
const int RSSI_MIN =-100;// define minimum strength of signal in dBm

WiFiClient espClient;

int dBmtoPercentage(int dBm) {
  int quality;
  if(dBm <= RSSI_MIN) {
    quality = 0;
  } else if(dBm >= RSSI_MAX) {  
    quality = 100;
  } else {
    quality = 2 * (dBm + 100);
  }
  return quality;
}

const String htmlTop = "<html>\
  <head>\
    <title>%s</title>\
    <style>\
      body { background-color: #303030; font-family: 'Anaheim',sans-serif; Color: #d8d8d8; }\
    </style>\
  </head>\
  <body>\
    <h1>%s LED Controller</h1>";

const String htmlBottom = "<br><br><hr>\
  <p>Uptime: %02d:%02d:%02d | Device: %s</p>\
  %s\
  </body>\
</html>";

const String htmlWifiApConnectForm = "<h2>Connect to WiFi Access Point (AP)</h2>\
    <form method='POST' action='/connect' enctype='application/x-www-form-urlencoded'>\
      <label for='ssid'>SSID (AP Name):</label><br>\
      <input type='text' id='ssid' name='ssid'><br><br>\
      <label for='pass'>Password (WPA2):</label><br>\
      <input type='password' id='pass' name='password' minlength='8' autocomplete='off' required><br><br>\
      <input type='submit' value='Connect...'>\
    </form>";

const String htmlDeviceConfigs = "<hr><h2>Configs</h2>\
    <form method='POST' action='/config' enctype='application/x-www-form-urlencoded'>\
      <label for='deviceName'>Device name:</label><br>\
      <input type='text' id='deviceName' name='deviceName' value='%s'><br>\
      <br>\
      <label for='mqttServer'>MQTT Server:</label><br>\
      <input type='text' id='mqttServer' name='mqttServer' value='%s'><br>\
      <label for='mqttPort'>MQTT Port:</label><br>\
      <input type='text' id='mqttPort' name='mqttPort' value='%i'><br>\
      <label for='mqttTopic'>MQTT Topic:</label><br>\
      <input type='text' id='mqttTopic' name='mqttTopic' value='%s'><br>\
      <br>\
      <input type='submit' value='Set...'>\
    </form>";

CWifiManager::CWifiManager(): 
apMode(false), rebootNeeded(false) {    
  pinMode(BOARD_LED_PIN,OUTPUT);
  strcpy(SSID, configuration.wifiSsid);
  server = new AsyncWebServer(WEB_SERVER_PORT);
  connect();
}

void CWifiManager::connect() {

  status = WF_CONNECTING;
  strcpy(softAP_SSID, "");
  tMillis = millis();

  if (strlen(SSID)) {

    // Join AP from Config
    Log.infoln("Connecting to WiFi: '%s'", SSID);
    WiFi.begin(SSID, configuration.wifiPassword);
    apMode = false;
    
  } else {

    // Create AP using fallback and chip ID
    uint32_t chipId = 0;
    #ifdef ESP32
      for(int i=0; i<17; i=i+8) {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
      }
    #elif ESP8266
      chipId = ESP.getChipId();
    #endif
  
    Log.infoln("Chip ID: '%i'", chipId);
    sprintf_P(softAP_SSID, "%s_%i", WIFI_FALLBACK_SSID, chipId);
    Log.infoln("Creating WiFi: '%s' / '%s'", softAP_SSID, WIFI_FALLBACK_PASS);
    
    if (WiFi.softAP(softAP_SSID, WIFI_FALLBACK_PASS)) {
      apMode = true;
      Log.infoln("Wifi AP '%s' created, listening on '%s'", softAP_SSID, WiFi.softAPIP().toString().c_str());
    } else {
      Log.errorln("Wifi AP faliled");
    };

  }
  
}

void CWifiManager::listen() {

  status = WF_LISTENING;

  // Web
  server->on("/", std::bind(&CWifiManager::handleRoot, this, std::placeholders::_1));
  server->on("/connect", HTTP_POST, std::bind(&CWifiManager::handleConnect, this, std::placeholders::_1));
  server->on("/led_mode", HTTP_POST, std::bind(&CWifiManager::handleConfig, this, std::placeholders::_1));
  server->begin();
  Log.infoln("Web server listening on %s port %i", WiFi.localIP().toString().c_str(), WEB_SERVER_PORT);

  // NTP
  Log.infoln("Configuring time from %s at %i (%i)", configuration.ntpServer, configuration.gmtOffset_sec, configuration.daylightOffset_sec);

  // OTA
  AsyncElegantOTA.begin(server);

  configTime(configuration.gmtOffset_sec, configuration.daylightOffset_sec, configuration.ntpServer);
  
  /*
  struct tm timeinfo;
  //time()
  if(getLocalTime(&timeinfo)){
    Log.infoln("%i:%i", timeinfo.tm_hour,timeinfo.tm_min);
  }
  */
  
}

void CWifiManager::loop() {

  if (rebootNeeded && millis() - tMillis > 200) {
    Log.noticeln("Rebooting...");
#ifdef ESP32
    ESP.restart();
#elif ESP8266
    ESP.reset();
#endif
    return;
  }

  if (WiFi.status() == WL_CONNECTED || apMode ) {
    // WiFi is connected

    if (status != WF_LISTENING) {  
      // Start listening for requests
      listen();
    }

  } else {
    // WiFi is down

    switch (status) {
      case WF_LISTENING: {
        Log.infoln("Disconnecting %i", status);
        server->end();
        status = WF_CONNECTING;
        connect();
      } break;
      case WF_CONNECTING: {
        if (millis() - tMillis > MAX_CONNECT_TIMEOUT_MS) {
          Log.warning("Connecting failed (wifi status %i) after %l ms, create an AP instead", (millis() - tMillis), WiFi.status());
          tMillis = millis();
          strcpy(SSID, "");
          connect();
        }
      } break;

    }

  }
  
}

void CWifiManager::handleRoot(AsyncWebServerRequest *request) {

  Log.infoln("handleRoot");
  
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  AsyncResponseStream *response = request->beginResponseStream("text/html");
  response->printf(htmlTop.c_str(), configuration.name, configuration.name);

  if (apMode) {
    response->printf(htmlWifiApConnectForm.c_str());
  } else {
    response->printf("<p>Connected to '%s'</p>", SSID);
  }
  
  response->printf(htmlDeviceConfigs.c_str(), configuration.name, configuration.mqttServer, 
    configuration.mqttPort, configuration.mqttTopic);

  response->printf(htmlBottom.c_str(), hr, min % 60, sec % 60, String(DEVICE_NAME), String("TODO"));
  request->send(response);
}

void CWifiManager::handleConnect(AsyncWebServerRequest *request) {

  Log.infoln("handleConnect");

  String ssid = request->arg("ssid");
  String password = request->arg("password");
  
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  AsyncResponseStream *response = request->beginResponseStream("text/html");
  response->printf(htmlTop.c_str(), configuration.name, configuration.name);
  response->printf("<p>Connecting to '%s' ... see you on the other side!</p>", ssid.c_str());
  response->printf(htmlBottom.c_str(), hr, min % 60, sec % 60, String(DEVICE_NAME), String("TODO"));
  request->send(response);

  ssid.toCharArray(configuration.wifiSsid, sizeof(configuration.wifiSsid));
  password.toCharArray(configuration.wifiPassword, sizeof(configuration.wifiPassword));

  Log.noticeln("Saved config SSID: '%s'", configuration.wifiSsid);

  EEPROM_saveConfig();

  strcpy(SSID, configuration.wifiSsid);
  connect();
}

void CWifiManager::handleConfig(AsyncWebServerRequest *request) {

  Log.infoln("handleConfig");

  String deviceName = request->arg("deviceName");
  deviceName.toCharArray(configuration.name, sizeof(configuration.name));
  Log.infoln("Device req name: %s", deviceName);
  Log.infoln("Device size %i name: %s", sizeof(configuration.name), configuration.name);

  String mqttServer = request->arg("mqttServer");
  mqttServer.toCharArray(configuration.mqttServer, sizeof(configuration.mqttServer));
  Log.infoln("MQTT Server: %s", mqttServer);

  uint16_t mqttPort = atoi(request->arg("mqttPort").c_str());
  configuration.mqttPort = mqttPort;
  Log.infoln("MQTT Port: %i", mqttPort);

  String mqttTopic = request->arg("mqttTopic");
  mqttTopic.toCharArray(configuration.mqttTopic, sizeof(configuration.mqttTopic));
  Log.infoln("MQTT Topic: %s", mqttTopic);
  
  EEPROM_saveConfig();

  /*
  // FIXME: Redudent code
  // MQTT
  client.disconnect();
  client.setServer(configuration.mqttServer, configuration.mqttPort);

  if (!client.connected()) {
    Log.noticeln("Attempting MQTT connection to '%s:%i' as '%s' ...", configuration.mqttServer, configuration.mqttPort, getDeviceId());
    if (client.connect(getDeviceId())) {
      if (strlen(configuration.mqttTopic)) {
        #ifdef TEMP_SENSOR
          char topic[255];
          sprintf_P(topic, "%s/sensor/temperature", configuration.mqttTopic);
          client.publish(topic,String(device->temperature(), 2).c_str());
          sprintf_P(topic, "%s/sensor/humidity", configuration.mqttTopic);
          client.publish(topic,String(device->humidity(), 2).c_str());
        #endif
      } 

    } else {
      Log.noticeln("MQTT connect failed, rc=%i", client.state());
    }
  }
  */
  
  request->redirect("/");
}