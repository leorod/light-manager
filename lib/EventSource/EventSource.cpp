#define ARDUINOJSON_USE_LONG_LONG 1
#include "EventSource.h"

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

EventSource::EventSource(PubSubClient& mqttClient, const char* topic) {
    _mqttClient = &mqttClient;
    _topic = strdup(topic);
    init();
}

void EventSource::poll() {
    if (!_mqttClient->connected()) {
        Serial.println("MQTT Client connection start");
        reconnect(_topic);
    }
    _mqttClient->loop();
}


void EventSource::init() {
    _mqttClient->setCallback([this](char* topic, byte* payload, unsigned int length) {
        this -> process(topic, payload, length);
    });
}

void EventSource::process(char* topic, byte* payload, unsigned int length) {
    Serial.println("Deserializing...");
    Event event = deserialize((char*) payload);
    Serial.println("Setting topic...");
    event.topic = topic;
    Serial.println("Sending to log...");
    log(event);
}

Event EventSource::deserialize(char* message) {
    Event event;
    const size_t capacity = JSON_OBJECT_SIZE(5) + sizeof(message);
    StaticJsonDocument<capacity> json;
    deserializeJson(json, message);
    strlcpy(event.uuid, json["uuid"], sizeof(event.uuid));
    event.type = strdup(json["type"]);
    event.timestamp = json["timestamp"];
    event.source = strdup(json["source"]);
    event.value = strdup(json["value"]);
    return event;
}

char* EventSource::serialize(Event event) {
    Serial.println("Serializing");
    const int bufferSize = 256;
    const int capacity = JSON_OBJECT_SIZE(6) + bufferSize;
    StaticJsonDocument<capacity> json;
    json["uuid"] = event.uuid;
    json["type"] = event.type;
    json["timestamp"] = event.timestamp;
    json["source"] = event.source;
    json["value"] = event.value;
    json["topic"] = event.topic;
    char buffer[bufferSize];
    serializeJson(json, buffer);
    Serial.println(buffer);
    return buffer;
}

void EventSource::log(Event event) {
    _mqttClient->publish("/audit", serialize(event));
}

boolean EventSource::reconnect(char* topic) {
    boolean success = reconnect();
    if (success) {
        _mqttClient->subscribe(topic);
        Serial.print("Subscribed to topic: ");
        Serial.println(topic);
    }
    return success;
}

boolean EventSource::reconnect() {
    boolean success = false;
    while (!_mqttClient->connected()) {
        Serial.println("Attempting connection to MQTT server");
        String clientId = "ESP8266-";
        clientId += String(random(0xffff), HEX);
        _mqttClient->setSocketTimeout(60);
        Serial.println("Timeout set to 60s");
        Serial.println(ESP.getFreeHeap());
        Serial.println("Connecting...");
        if (success = _mqttClient->connect(clientId.c_str())) {
            Serial.println("Successfully connected to MQTT server");
        } else {
            Serial.print("MQTT connection failed: ");
            Serial.println(_mqttClient->state());
            Serial.println("Attempting retry in 5s...");
            delay(5000);
        }
    }
    return success;
}