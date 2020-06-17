#include <Arduino.h>
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <LittleFS.h>
#include <ArduinoJson.h>

struct Config {
  char mqtt_host[40];
  char mqtt_port[6];
};
const char *configPath = "/config.txt";
Config config;

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());

  Serial.println(myWiFiManager->getConfigPortalSSID());
}

bool saveConfig(Config newConfig) {
  bool shouldUpdate = strcmp(config.mqtt_host, newConfig.mqtt_host) != 0 || strcmp(config.mqtt_port, newConfig.mqtt_port) != 0;
  if (shouldUpdate) {
    Serial.println("Settings updated. Saving to file.");
    File configFile = LittleFS.open(configPath, "w");
    if (configFile) {
      Serial.print("Writing updated config to: ");
      Serial.println(configPath);
      DynamicJsonDocument jsonDoc(79);
      jsonDoc["mqttHost"] = newConfig.mqtt_host;
      jsonDoc["mqttPort"] = newConfig.mqtt_port;
      serializeJson(jsonDoc, configFile);
      Serial.println("Config saved");
    }
    configFile.close();
  }
  return shouldUpdate;
}

void setup() {
  Serial.begin(9600);

  if (LittleFS.begin()) {
    Serial.println("Filesystem successfully mounted");
    if (LittleFS.exists(configPath)) {
      Serial.println("Reading saved config");
      File configFile = LittleFS.open(configPath, "r");
      if (configFile) {
        DynamicJsonDocument jsonDoc(configFile.size());
        DeserializationError error = deserializeJson(jsonDoc, configFile);
        if (error) {
          Serial.println("Cannot parse config file");
        } else {
          strcpy(config.mqtt_host, jsonDoc["mqttHost"]);
          strcpy(config.mqtt_port, jsonDoc["mqttPort"] | "1883");
        }
      }
      configFile.close(); 
    }
  } else {
    Serial.println("Cannot mount filesystem");
  }

  WiFiManagerParameter mqttHostParam("Host", "MQTT Host", config.mqtt_host, 40);
  WiFiManagerParameter mqttPortParam("1883", "MQTT Port", config.mqtt_port, 6);

  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.addParameter(&mqttHostParam);
  wifiManager.addParameter(&mqttPortParam);

  if(!wifiManager.autoConnect("LightManager", "ESP8266setup")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  } 
  Serial.println("Successfully connected to SSID: " + WiFi.SSID());

  //read updated parameters
  Config newConfig;
  strcpy(newConfig.mqtt_host, mqttHostParam.getValue());
  strcpy(newConfig.mqtt_port, mqttPortParam.getValue());

  if (shouldSaveConfig) {
    saveConfig(newConfig);
  }

}

void loop() {
}