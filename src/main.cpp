#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "Setup.h"
#include "EventSource.h"

EventSource *eventSource;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void blinkSuccess() {
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
    }
}

void process(char* topic, byte* payload, unsigned int length) {
    Serial.println((char*)payload);
}

void setup() {
    const char* configPath = "/config.txt";
    const char* apSsid = "LightManager";
    const char* apPassword = "ESP8266setup";
    const char* topic = "/hq/main/lights";

    delay(5000);

    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(9600);
    SetupManager setupManager(configPath, apSsid, apPassword);
    Config config = setupManager.getConfig();
    IPAddress ipAddress;
    ipAddress.fromString(config.mqtt_host); 
    mqttClient.setServer("192.168.0.31", 1883);
    //mqttClient.setServer(ipAddress, atoi(config.mqtt_port));
    eventSource = new EventSource(mqttClient, topic);

    Serial.println("Setup complete!");
    blinkSuccess();
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    Serial.println(ESP.getFreeHeap());
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish("outTopic", "hello world");
      // ... and resubscribe
      mqttClient.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
    eventSource->poll();
}