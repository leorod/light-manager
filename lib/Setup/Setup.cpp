#include "Setup.h"

#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <LittleFS.h>
#include <ArduinoJson.h>

SetupManager::SetupManager(const char *configPath, const char *apSsid, const char *apPassword) {
    _configPath = strdup(configPath);
    _apSsid = strdup(apSsid);
    _apPassword = strdup(apPassword);
    init();
}

Config SetupManager::getConfig() {
    return _currentConfig;
}

bool shouldSaveConfig = false;

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());

  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void SetupManager::init() {
    _currentConfig = loadConfig();

    WiFiManagerParameter mqttHostParam("Host", "MQTT Host", _currentConfig.mqtt_host, 40);
    WiFiManagerParameter mqttPortParam("1883", "MQTT Port", _currentConfig.mqtt_port, 6);

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

Config SetupManager::loadConfig() {
    Config config;
    if (LittleFS.begin()) {
        Serial.println("Filesystem successfully mounted");
        if (LittleFS.exists(_configPath)) {
            Serial.println("Reading saved config");
            File configFile = LittleFS.open(_configPath, "r");
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
    return config;
}

bool SetupManager::saveConfig(Config config) {
    bool shouldUpdate = strcmp(_currentConfig.mqtt_host, config.mqtt_host) != 0 
        || strcmp(_currentConfig.mqtt_port, config.mqtt_port) != 0;
    if (shouldUpdate) {
        Serial.println("Settings updated. Saving to file.");
        File configFile = LittleFS.open(_configPath, "w");
        if (configFile) {
            Serial.print("Writing updated config to: ");
            Serial.println(_configPath);
            DynamicJsonDocument jsonDoc(79);
            jsonDoc["mqttHost"] = config.mqtt_host;
            jsonDoc["mqttPort"] = config.mqtt_port;
            serializeJson(jsonDoc, configFile);
            Serial.println("Config saved");
        }
        _currentConfig = config;
        configFile.close();
    }
    return shouldUpdate;
}