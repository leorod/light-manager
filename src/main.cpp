#include <Arduino.h>

#include "Setup.h"

const char *configPath = "/config.txt";
const char *apSsid = "LightManager";
const char *apPassword = "ESP8266setup";
Config config;

void blinkSuccess() {
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
    }
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(9600);
    SetupManager setupManager(configPath, apSsid, apPassword);
    config = setupManager.getConfig();
    Serial.println(config.mqtt_host);
    Serial.println(config.mqtt_port);
    blinkSuccess();
}

void loop() {
}