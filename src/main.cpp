#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "Setup.h"
#include "Toggle.h"

EventSource *eventSource;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Toggle toggles[]{
    {.id = 1, .pin = D1, .state = HIGH },
    {.id = 2, .pin = D2, .state = HIGH }
};
ToggleEventHandler eventHandler(toggles, 2);

void blinkSuccess() {
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
    }
}

void setup() {
    const char* configPath = "/config.txt";
    const char* apSsid = "LightManager";
    const char* apPassword = "ESP8266setup";
    const char* topic = "/hq/main/lights";

    Serial.begin(9600);
    pinMode(D1, OUTPUT);
    pinMode(D2, OUTPUT);
    digitalWrite(D1, HIGH);
    digitalWrite(D2, HIGH);
    pinMode(LED_BUILTIN, OUTPUT);
    
    SetupManager setupManager(configPath, apSsid, apPassword);
    Config config = setupManager.getConfig();
    mqttClient.setServer(strdup(config.mqtt_host), atoi(config.mqtt_port));
    eventSource = new EventSource(mqttClient, eventHandler, topic);
    DB_PRINTLN("Setup complete!");
    blinkSuccess();
}

void loop() {
    eventSource->poll();
}