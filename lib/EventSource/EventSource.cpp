#define ARDUINOJSON_USE_LONG_LONG 1
#include "EventSource.h"

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

EventSource::EventSource(PubSubClient& mqttClient, EventHandler& handler, const char* topic) {
    _mqttClient = &mqttClient;
    _handler = &handler;
    _topic = strdup(topic);
    init();
}

void EventSource::poll() {
    if (!_mqttClient->connected()) {
        DB_PRINTLN("MQTT Client connection start");
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
    char* strPayload = (char*) payload;
    DB_PRINTLN("Event received:");
    DB_PRINTLN(strPayload);
    char srcPayload[length + 1];
    strlcpy(srcPayload, strPayload, sizeof(srcPayload));
    Event event = deserialize(strPayload);
    if (_handler->accepts(event)) {
        _handler->handle(event);
    }
    log(srcPayload, topic);
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

void EventSource::log(char* payload, char* sourceTopic) {
    String auditTopic = "/audit" + String(sourceTopic);
    _mqttClient->publish(auditTopic.c_str(), payload);
    DB_PRINT("Audit message sent to topic: ");
    DB_PRINTLN(auditTopic);
}

boolean EventSource::reconnect(char* topic) {
    boolean success = reconnect();
    if (success) {
        _mqttClient->subscribe(topic);
        DB_PRINT("Subscribed to topic: ");
        DB_PRINTLN(topic);
    }
    return success;
}

boolean EventSource::reconnect() {
    boolean success = false;
    while (!_mqttClient->connected()) {
        DB_PRINTLN("Attempting connection to MQTT server");
        String clientId = "ESP8266-";
        clientId += String(random(0xffff), HEX);
        _mqttClient->setSocketTimeout(60);
        DB_PRINTLN("Timeout set to 60s");
        DB_PRINTLN("Connecting...");
        if (success = _mqttClient->connect(clientId.c_str())) {
            DB_PRINTLN("Successfully connected to MQTT server");
        } else {
            DB_PRINT("MQTT connection failed: ");
            DB_PRINTLN(_mqttClient->state());
            DB_PRINTLN("Attempting retry in 5s...");
            delay(5000);
        }
    }
    return success;
}