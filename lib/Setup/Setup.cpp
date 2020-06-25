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
  shouldSaveConfig = true;
}

void configModeCallback(WiFiManager *myWiFiManager) {
  DB_PRINTLN("Entered config mode");
  DB_PRINTLN(WiFi.softAPIP());

  DB_PRINTLN(myWiFiManager->getConfigPortalSSID());
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
    DB_PRINTLN("Attempting to connect...");
    if(!wifiManager.autoConnect(_apSsid, _apPassword)) {
        DB_PRINTLN("Failed to connect and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(5000);
    } else {
        DB_PRINTLN("Successfully connected to SSID: " + WiFi.SSID());

        //read updated parameters
        Config newConfig;
        strlcpy(newConfig.mqtt_host, mqttHostParam.getValue(), sizeof(newConfig.mqtt_host));
        strlcpy(newConfig.mqtt_port, mqttPortParam.getValue(), sizeof(newConfig.mqtt_port));
        
        if (shouldSaveConfig) {
            saveConfig(newConfig);
        }

        while (WiFi.status() != WL_CONNECTED) {
            DB_PRINT('.');
            delay(500);
        }
        DB_PRINTLN(".");
    }
}

Config SetupManager::loadConfig() {
    Config config;
    if (LittleFS.begin()) {
        DB_PRINTLN("Filesystem successfully mounted");
        if (LittleFS.exists(_configPath)) {
            DB_PRINTLN("Reading saved config");
            File configFile = LittleFS.open(_configPath, "r");
            if (configFile) {
                configFile.setTimeout(5000);
                const size_t jsonCapacity = JSON_OBJECT_SIZE(2) + 40;
                StaticJsonDocument<jsonCapacity> jsonDoc;
                DeserializationError error = deserializeJson(jsonDoc, configFile);
                if (error) {
                    DB_PRINT("Cannot parse config file: ");
                    DB_PRINTLN(error.c_str());
                } else {
                    strlcpy(config.mqtt_host, jsonDoc["mqttHost"], sizeof(config.mqtt_host));
                    strlcpy(config.mqtt_port, jsonDoc["mqttPort"] | "1883", sizeof(config.mqtt_port));
                }
            }
            configFile.close();
        } else {
            DB_PRINTLN("No preexisting config found.");
        }
    } else {
        DB_PRINTLN("Cannot mount filesystem");
    }
    
    return config;
}

bool SetupManager::saveConfig(Config config) {
    bool shouldUpdate = strcmp(_currentConfig.mqtt_host, config.mqtt_host) != 0 
        || strcmp(_currentConfig.mqtt_port, config.mqtt_port) != 0;
    if (shouldUpdate) {
        DB_PRINTLN("Settings updated. Saving to file.");
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