#include "Setup.h"

#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <LittleFS.h>
#include <ArduinoJson.h>

SetupManager::SetupManager(const char* configPath, const char* apSsid, const char* apPassword) {
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
    WiFiManagerParameter mqttPortParam("Port", "MQTT Port", _currentConfig.mqtt_port, 6);

    WiFiManager wifiManager;
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.addParameter(&mqttHostParam);
    wifiManager.addParameter(&mqttPortParam);

    wifiManager.setConnectTimeout(60);
    Serial.println("Attempting to connect...");
    if(!wifiManager.autoConnect(_apSsid, _apPassword)) {
        Serial.println("failed to connect and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(5000);
    } else {
        Serial.println("Successfully connected to SSID: " + WiFi.SSID());

        //read updated parameters
        Config newConfig;
        strlcpy(newConfig.mqtt_host, mqttHostParam.getValue(), sizeof(newConfig.mqtt_host));
        strlcpy(newConfig.mqtt_port, mqttPortParam.getValue(), sizeof(newConfig.mqtt_port));
        
        if (shouldSaveConfig) {
            saveConfig(newConfig);
        }

        while (WiFi.status() != WL_CONNECTED) {
            Serial.print('.');
            delay(500);
        }
        Serial.println(".");
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
                configFile.setTimeout(5000);
                const size_t jsonCapacity = JSON_OBJECT_SIZE(2) + 40;
                StaticJsonDocument<jsonCapacity> jsonDoc;
                DeserializationError error = deserializeJson(jsonDoc, configFile);
                if (error) {
                    Serial.print("Cannot parse config file: ");
                    Serial.println(error.c_str());
                } else {
                    strlcpy(config.mqtt_host, jsonDoc["mqttHost"], sizeof(config.mqtt_host));
                    strlcpy(config.mqtt_port, jsonDoc["mqttPort"] | "1883", sizeof(config.mqtt_port));
                }
            }
            configFile.close();
        } else {
            Serial.println("No preexisting config found.");
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
            const int capacity = JSON_OBJECT_SIZE(2);
            StaticJsonDocument<capacity> jsonDoc;
            jsonDoc["mqttHost"] = config.mqtt_host;
            jsonDoc["mqttPort"] = config.mqtt_port;
            serializeJson(jsonDoc, configFile);
        }
        _currentConfig = config;
        configFile.close();
    }
    return shouldUpdate;
}