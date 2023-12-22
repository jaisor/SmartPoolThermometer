#if !( defined(ESP32) ) && !( defined(ESP8266) )
  #error This code is intended to run on ESP8266 or ESP32 platform!
#endif

#include <Arduino.h>
#include <WiFiClient.h>
#include <Time.h>
#include <ezTime.h>
#include <AsyncElegantOTA.h>
#include <StreamUtils.h>

#include "wifi/WifiManager.h"
#include "Configuration.h"

#define MAX_CONNECT_TIMEOUT_MS 15000 // 10 seconds to connect before creating its own AP

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

#ifdef ESP8266_old
bool getLocalTime(struct tm * info) {
  uint32_t start = millis();
  time_t now;
  while((millis()-start) <= 5000) {
    time(&now);
    localtime_r(&now, info);
    if(info->tm_year > (2016 - 1900)){
      return true;
    }
    delay(10);
  }
  return false;
}
#endif

const String htmlTop = "<html>\
  <head>\
  <title>%s</title>\
  <style>\
    body { background-color: #303030; font-family: 'Anaheim',sans-serif; Color: #d8d8d8; }\
  </style>\
  </head>\
  <body>\
  <h1>%s - Smart Pool Thermometer</h1>%s";

const String htmlBottom = "<br><br><hr>\
  <p><b>%s</b><br>\
  Uptime: <b>%02d:%02d:%02d</b><br>\
  WiFi Signal Strength: <b>%i%%</b>\
  </p></body>\
</html>";

const String htmlWifiApConnectForm = "<hr><h2>Connect to WiFi Access Point (AP)</h2>\
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
    <label for='tempUnit'>Temperature units:</label><br>\
    <select name='tempUnit' id='tempUnit'>\
    %s\
    </select><br>\
    <br>\
    <label for='mqttServer'>MQTT server:</label><br>\
    <input type='text' id='mqttServer' name='mqttServer' value='%s'><br>\
    <label for='mqttPort'>MQTT port:</label><br>\
    <input type='text' id='mqttPort' name='mqttPort' value='%u'><br>\
    <label for='mqttTopic'>MQTT topic:</label><br>\
    <input type='text' id='mqttTopic' name='mqttTopic' value='%s'><br>\
    <label for='mqttDataType'>Publish MQTT data as:</label><br>\
    <select name='mqttDataType' id='mqttDataType'>\
    %s\
    </select><br>\
    <br>\
    <label for='battVoltsDivider'>Battery volt measurement divider:</label><br>\
    <input type='text' id='battVoltsDivider' name='battVoltsDivider' value='%.2f'><br>\
    <br>\
    <label for='deepSleepDurationSec'>Deep sleep cycle duration:</label><br>\
    <input type='text' id='deepSleepDurationSec' name='deepSleepDurationSec' value='%u'> sec.<br>\
    <small><i>0 = disable sleep, keep awake, drain the battery</i></small><br>\
    <br>\
    <input type='submit' value='Set...'>\
  </form>";

CWifiManager::CWifiManager(ISensorProvider *sp): 
rebootNeeded(false), postedSensorUpdate(false), sensorProvider(sp), wifiRetries(0) {  

  // Start capturing voltage
  batteryVoltage = sensorProvider->getBatteryVoltage(NULL);

  sensorJson["name"] = configuration.name;
  sensorJson["deepSleepDurationSec"] = configuration.deepSleepDurationSec;
  sensorJson["battVoltsDivider"] = configuration.battVoltsDivider;

  strcpy(SSID, configuration.wifiSsid);
  server = new AsyncWebServer(WEB_SERVER_PORT);
  mqtt.setClient(espClient);
  connect();
}

void CWifiManager::connect() {

  status = WF_CONNECTING;
  strcpy(softAP_SSID, "");
  tMillis = millis();

  uint32_t deviceId = sensorProvider->getDeviceId();
  sensorJson["deviceId"] = deviceId;
  Log.infoln("Device ID: '%i'", deviceId);

  if (strlen(SSID)) {

    // Join AP from Config
    Log.infoln("Connecting to WiFi: '%s'", SSID);
    WiFi.begin(SSID, configuration.wifiPassword);
    wifiRetries = 0;

  } else {

    // Create AP using fallback and chip ID
    sprintf_P(softAP_SSID, "%s_%i", WIFI_FALLBACK_SSID, deviceId);
    Log.infoln("Creating WiFi: '%s' / '%s'", softAP_SSID, WIFI_FALLBACK_PASS);

    if (WiFi.softAP(softAP_SSID, WIFI_FALLBACK_PASS)) {
      wifiRetries = 0;
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
  server->on("/config", HTTP_POST, std::bind(&CWifiManager::handleConfig, this, std::placeholders::_1));
  server->on("/factory_reset", HTTP_POST, std::bind(&CWifiManager::handleFactoryReset, this, std::placeholders::_1));
  server->begin();
  Log.infoln("Web server listening on %s port %i", WiFi.localIP().toString().c_str(), WEB_SERVER_PORT);
  
  sensorJson["ip"] = WiFi.localIP();

  // NTP
  Log.infoln("Configuring time from %s at %i (%i)", configuration.ntpServer, configuration.gmtOffset_sec, configuration.daylightOffset_sec);
  configTime(configuration.gmtOffset_sec, configuration.daylightOffset_sec, configuration.ntpServer);
  struct tm timeinfo;
  if(getLocalTime(&timeinfo)){
    Log.noticeln("The time is %i:%i", timeinfo.tm_hour,timeinfo.tm_min);
  }

  // OTA
  AsyncElegantOTA.begin(server);

  // MQTT
  mqtt.setServer(configuration.mqttServer, configuration.mqttPort);

  using std::placeholders::_1;
  using std::placeholders::_2;
  using std::placeholders::_3;
  mqtt.setCallback(std::bind( &CWifiManager::mqttCallback, this, _1,_2,_3));

  if (strlen(configuration.mqttServer) && strlen(configuration.mqttTopic) && !mqtt.connected()) {
    Log.noticeln("Attempting MQTT connection to '%s:%i' ...", configuration.mqttServer, configuration.mqttPort);
    if (mqtt.connect(String(sensorProvider->getDeviceId()).c_str())) {
      Log.noticeln("MQTT connected");
      
      sprintf_P(mqttSubcribeTopicConfig, "%s/%u/config", configuration.mqttTopic, sensorProvider->getDeviceId());
      bool r = mqtt.subscribe(mqttSubcribeTopicConfig);
      Log.noticeln("Subscribed for config changes to MQTT topic '%s' success = %T", mqttSubcribeTopicConfig, r);

      postSensorUpdate();
    } else {
      Log.warningln("MQTT connect failed, rc=%i", mqtt.state());
    }
  }
}

void CWifiManager::loop() {

  batteryVoltage = (float)(batteryVoltage + sensorProvider->getBatteryVoltage(NULL)) / 2.0;

  if (rebootNeeded && millis() - tMillis > 200) {
  Log.noticeln("Rebooting...");
#ifdef ESP32
  ESP.restart();
#elif ESP8266
  ESP.reset();
#endif
  return;
  }

  if (WiFi.status() == WL_CONNECTED || isApMode() ) {
  // WiFi is connected

  if (status != WF_LISTENING) {  
    // Start listening for requests
    listen();
    return;
  }

  mqtt.loop();

  if (millis() - tMillis > (postedSensorUpdate || isApMode() ? 30000 : 1000) &&
    strlen(configuration.mqttServer) && strlen(configuration.mqttTopic) && mqtt.connected()) {
    tMillis = millis();
    postSensorUpdate();
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
        tMillis = millis();
        if (wifiRetries++ > 3) {
          Log.warningln("Connecting failed (wifi status %i) after %l ms, create an AP instead", (millis() - tMillis), WiFi.status());
          strcpy(SSID, "");
        }
        connect();
      }
    } break;

  }

  }
  
}

void CWifiManager::handleRoot(AsyncWebServerRequest *request) {

  Log.infoln("handleRoot");

  AsyncResponseStream *response = request->beginResponseStream("text/html");
  printHTMLTop(response);

  if (isApMode()) {
    response->printf(htmlWifiApConnectForm.c_str());
  } else {
    response->printf("<p>Connected to '%s'</p>", SSID);
  }

  char mqttDataType[256];
  snprintf(mqttDataType, 256, "<option %s value='0'>JSON - single message</option>\
    <option %s value='1'>Scalar - each value a different message</option>\
    <option %s value='2'>Both</option>", 
    configuration.mqttDataType == MQTT_DATA_JSON ? "selected" : "", 
    configuration.mqttDataType == MQTT_DATA_SCALAR ? "selected" : "", 
    configuration.mqttDataType == MQTT_DATA_BOTH ? "selected" : "");
  
  char tempUnit[256];
  snprintf(tempUnit, 256, "<option %s value='0'>Celsius</option>\
    <option %s value='1'>Fahrenheit</option>", 
    configuration.tempUnit == TEMP_UNIT_CELSIUS ? "selected" : "", 
    configuration.tempUnit == TEMP_UNIT_FAHRENHEIT ? "selected" : "");

  response->printf(htmlDeviceConfigs.c_str(), configuration.name, tempUnit, 
    configuration.mqttServer, configuration.mqttPort, configuration.mqttTopic, mqttDataType,
    configuration.battVoltsDivider, configuration.deepSleepDurationSec);

  printHTMLBottom(response);
  request->send(response);
}

void CWifiManager::handleConnect(AsyncWebServerRequest *request) {

  Log.infoln("handleConnect");

  String ssid = request->arg("ssid");
  String password = request->arg("password");
  
  AsyncResponseStream *response = request->beginResponseStream("text/html");
  
  printHTMLTop(response);
  response->printf("<p>Connecting to '%s' ... see you on the other side!</p>", ssid.c_str());
  printHTMLBottom(response);

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
  Log.infoln("MQTT Port: %u", mqttPort);

  String mqttTopic = request->arg("mqttTopic");
  mqttTopic.toCharArray(configuration.mqttTopic, sizeof(configuration.mqttTopic));
  Log.infoln("MQTT Topic: %s", mqttTopic);

  uint16_t mqttDataType = atoi(request->arg("mqttDataType").c_str());
  configuration.mqttDataType = mqttDataType;
  Log.infoln("MQTT Data Type: %u", mqttDataType);

  uint16_t battVoltsDivider = atoi(request->arg("battVoltsDivider").c_str());
  configuration.battVoltsDivider = battVoltsDivider;
  Log.infoln("battVoltsDivider: %u", battVoltsDivider);

  float deepSleepDurationSec = atof(request->arg("deepSleepDurationSec").c_str());
  configuration.deepSleepDurationSec = deepSleepDurationSec;
  Log.infoln("deepSleepDurationSec: %.2f", deepSleepDurationSec);

  uint16_t tempUnit = atoi(request->arg("tempUnit").c_str());
  configuration.tempUnit = tempUnit;
  Log.infoln("Temperatuer unit: %u", tempUnit);

  EEPROM_saveConfig();
  
  rebootNeeded = true;
  request->redirect("/");
}

void CWifiManager::handleFactoryReset(AsyncWebServerRequest *request) {
  Log.infoln("handleFactoryReset");
  
  AsyncResponseStream *response = request->beginResponseStream("text/html");
  response->setCode(200);
  response->printf("OK");

  EEPROM_wipe();
  rebootNeeded = true;
  
  request->send(response);
}

void CWifiManager::postSensorUpdate() {

  if (!mqtt.connected()) {
    if (mqtt.state() < MQTT_CONNECTED 
      && strlen(configuration.mqttServer) && strlen(configuration.mqttTopic)) { // Reconnectable
      Log.noticeln("Attempting to reconnect from MQTT state %i at '%s:%i' ...", mqtt.state(), configuration.mqttServer, configuration.mqttPort);
      if (mqtt.connect(String(sensorProvider->getDeviceId()).c_str())) {
        Log.noticeln("MQTT reconnected");
        sprintf_P(mqttSubcribeTopicConfig, "%s/%u/config", configuration.mqttTopic, sensorProvider->getDeviceId());
        bool r = mqtt.subscribe(mqttSubcribeTopicConfig);
        Log.noticeln("Subscribed for config changes to MQTT topic '%s' success = %T", mqttSubcribeTopicConfig, r);
      } else {
        Log.warningln("MQTT reconnect failed, rc=%i", mqtt.state());
      }
    }
    if (!mqtt.connected()) {
      Log.noticeln("MQTT not connected %i", mqtt.state());
      return;
    }
  }

  if (!strlen(configuration.mqttTopic)) {
    Log.warningln("Blank MQTT topic");
    return;
  }

  char topic[255];
  bool current = false;
  float v; int iv;

  bool pJson = configuration.mqttDataType == MQTT_DATA_JSON || configuration.mqttDataType == MQTT_DATA_BOTH;
  bool pScalar = configuration.mqttDataType == MQTT_DATA_SCALAR || configuration.mqttDataType == MQTT_DATA_BOTH;

  bool sensorReady = sensorProvider->isSensorReady();

  if (sensorReady) {
    v = sensorProvider->getTemperature(&current);
    if (configuration.tempUnit == TEMP_UNIT_FAHRENHEIT) {
      v = v * 1.8 + 32;
    }
    char tunit[32];
    snprintf(tunit, 32, (configuration.tempUnit == TEMP_UNIT_CELSIUS ? "Celsius" : (configuration.tempUnit == TEMP_UNIT_FAHRENHEIT ? "Fahrenheit" : "" )));
    
    if (current) {
      if (pScalar) {
        sprintf_P(topic, "%s/temperature", configuration.mqttTopic);
        mqtt.publish(topic,String(v, 2).c_str());
        Log.noticeln("Sent '%F' %s temp to MQTT topic '%s'", v, tunit, topic);
      }
      if (pJson) {
        sensorJson["temperature"] = v;
        sensorJson["temperature_unit"] = tunit;
      }
    }

    v = sensorProvider->getHumidity(&current);
    if (current) {
      if (pScalar) {
        sprintf_P(topic, "%s/humidity", configuration.mqttTopic);
        mqtt.publish(topic,String(v, 2).c_str());
        Log.noticeln("Sent '%F%' humidity to MQTT topic '%s'", v, topic);
      }
      if (pJson) {
        sensorJson["humidity"] = v;
        sensorJson["humidit_unit"] = "percent";
      }
    }
  }

#ifdef BATTERY_SENSOR
  if (configuration.battVoltsDivider > 0) {
    v = (float)(batteryVoltage + sensorProvider->getBatteryVoltage(NULL)) / 2.0;
    if (pScalar) {
      sprintf_P(topic, "%s/battery", configuration.mqttTopic);
      mqtt.publish(topic,String(v, 2).c_str());
      Log.noticeln("Sent '%Fv' battery voltage to MQTT topic '%s'", v, topic);
    }
    if (pJson) {
      sensorJson["battery_v"] = v;
    }

    iv = analogRead(BATTERY_SENSOR_ADC_PIN);
    if (pScalar) {
      sprintf_P(topic, "%s/adc_raw", configuration.mqttTopic);
      mqtt.publish(topic,String(iv).c_str());
      Log.noticeln("Sent '%i' raw ADC value to MQTT topic '%s'", iv, topic);
    }
    if (pJson) {
      sensorJson["adc_raw"] = iv;
    }
  }
#endif

  if (!isApMode()) {
    iv = dBmtoPercentage(WiFi.RSSI());
    if (pScalar) {
      sprintf_P(topic, "%s/wifi_percent", configuration.mqttTopic);
      mqtt.publish(topic,String(iv).c_str());
      Log.noticeln("Sent '%i%' WiFI signal to MQTT topic '%s'", iv, topic);
    }
    if (pJson) {
      sensorJson["wifi_percent"] = iv;
      sensorJson["wifi_rssi"] = WiFi.RSSI();
    }
  }

  postedSensorUpdate = sensorReady;

  time_t now; 
  time(&now);
  unsigned long uptimeMillis = sensorProvider->getUptime();

  if (pScalar) {
    sprintf_P(topic, "%s/timestamp", configuration.mqttTopic);
    mqtt.publish(topic,String(now).c_str());
    Log.noticeln("Sent '%u' timestamp to MQTT topic '%s'", (unsigned long)now, topic);

    //sprintf_P(topic, "%s/apmode", configuration.mqttTopic);
    //mqtt.publish(topic,String(isApMode()).c_str());
    //Log.noticeln("Sent '%i' AP mode to MQTT topic '%s'", isApMode(), topic);

    sprintf_P(topic, "%s/uptime_millis", configuration.mqttTopic);
    mqtt.publish(topic,String(uptimeMillis).c_str());
    Log.noticeln("Sent '%ums' uptime to MQTT topic '%s'", uptimeMillis, topic);
  }

  if (pJson) {
    sensorJson["sensorReady"] = sensorReady;
    sensorJson["uptime_millis"] = uptimeMillis;
    // Convert to ISO8601 for JSON
    char buf[sizeof "2011-10-08T07:07:09Z"];
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&now));
    sensorJson["timestamp_iso8601"] = String(buf);

    //sensorJson["jobDone"] = isJobDone();
    //sensorJson["apMode"] = isApMode();
    //sensorJson["postedSensorUpdate"] = postedSensorUpdate;
    sensorJson["mqttConfigTopic"] = mqttSubcribeTopicConfig;

    // sensor Json
    sprintf_P(topic, "%s/json", configuration.mqttTopic);
    mqtt.beginPublish(topic, measureJson(sensorJson), false);
    BufferingPrint bufferedClient(mqtt, 32);
    serializeJson(sensorJson, bufferedClient);
    bufferedClient.flush();
    mqtt.endPublish();

    String jsonStr;
    serializeJson(sensorJson, jsonStr);
    Log.noticeln("Sent '%s' json to MQTT topic '%s'", jsonStr.c_str(), topic);
  }
}

bool CWifiManager::isApMode() { 
  return WiFi.getMode() == WIFI_AP; 
}

void CWifiManager::mqttCallback(char *topic, uint8_t *payload, unsigned int length) {

  if (length == 0) {
    return;
  }

  Log.noticeln("Received %u bytes message on MQTT topic '%s'", length, topic);
  if (!strcmp(topic, mqttSubcribeTopicConfig)) {
    deserializeJson(configJson, (const byte*)payload, length);

    String jsonStr;
    serializeJson(configJson, jsonStr);
    Log.noticeln("Received configuration over MQTT with json: '%s'", jsonStr.c_str());

    if (configJson.containsKey("deepSleepDurationSec")) {
      configuration.deepSleepDurationSec = configJson["deepSleepDurationSec"].as<uint16_t>();
    }

    if (configJson.containsKey("battVoltsDivider")) {
      configuration.battVoltsDivider = configJson["battVoltsDivider"].as<float>();
    }

    if (configJson.containsKey("name")) {
      strncpy(configuration.name, configJson["name"], 128);
    }

    if (configJson.containsKey("mqttTopic")) {
      strncpy(configuration.mqttTopic, configJson["mqttTopic"], 64);
    }

    // Delete the config message in case it was retained
    mqtt.publish(mqttSubcribeTopicConfig, NULL, 0, true);
    Log.noticeln("Deleted config message");

    EEPROM_saveConfig();
    postSensorUpdate();
  }
  
}

void CWifiManager::printHTMLTop(Print *p) {

  char s[300] = "";
  char vs[100];
  bool current;
  float fv;

  fv = sensorProvider->getTemperature(&current);
  if (configuration.tempUnit == TEMP_UNIT_FAHRENHEIT) {
    fv = fv * 1.8 + 32;
  }
  char tunit[32];
  snprintf(tunit, 32, (configuration.tempUnit == TEMP_UNIT_CELSIUS ? "C" : (configuration.tempUnit == TEMP_UNIT_FAHRENHEIT ? "F " : "" )));

  snprintf(vs, sizeof(vs), "<h1>Temperature: %0.2f&#176; %s %s</h1>", fv, tunit, current ? "" : "(stale)");
  strcat(s, vs);

#ifdef BATTERY_SENSOR
  fv = sensorProvider->getBatteryVoltage(&current);
  snprintf(vs, sizeof(vs), "<h3>Battery: %0.2fv %s</h3>", fv, current ? "" : "(stale)");
  strcat(s, vs);
#endif

  p->printf(htmlTop.c_str(), configuration.name, configuration.name, s);
}

void CWifiManager::printHTMLBottom(Print *p) {
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  p->printf(htmlBottom.c_str(), String(DEVICE_NAME), hr, min % 60, sec % 60, dBmtoPercentage(WiFi.RSSI()));
}