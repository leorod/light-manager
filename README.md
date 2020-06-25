# Light Manager
IoT lights controller project for ESP8266.

Through this, the D1 Mini receives toggle instructions via MQTT and sends signals to one or more relays (or any other module that gets toggled through digital output) in order to switch lights on and off in a room.

## Based on:
* [tzapu](https://github.com/tzapu/)'s [WiFiManager](https://github.com/tzapu/WiFiManager)
* [knolleary](https://github.com/knolleary)'s [PubSubClient](https://github.com/knolleary/pubsubclient)
* [ArduinoJson](https://arduinojson.org/)

## Internal libraries
Everything in this process is modularized into libraries so the same code is easily reusable for different projects.
`main.cpp` should only be used to initialize and inject dependencies (and `loop()`, of course). Any project-specific data or code should be placed there, while the core logic would be handled by the libs (`Setup` could actually use some refactoring to be 100% decoupled from config parameters).

### Setup
This is basically a wrapper for [WiFiManager](https://github.com/tzapu/WiFiManager), with a predefined `Config` struct for storing MQTT connection data & handling WiFi network connection through WiFiManager -I recommend checking its [README](https://github.com/tzapu/WiFiManager/blob/master/README.md) to see how it works)-.
#### Usage
Simply initialize a SetupManager object through its constructor, supplying a path for the config file that will eventually store MQTT connection config and a SSID and Password for the Configuration AP (WiFiManager automatically switches to AP mode and exposes a webserver in order to setup a WiFi network for the ESP8266 to connect to. This is also where our MQTT config will be input).
This will automatically initialize WiFiManager and connect to the WiFi (or setup the Config AP).
MQTT data will be stored in the file stated in `configPath` in the flash memory.

```cpp
#include <Arduino.h>
#include "Setup.h"

void setup() {
  Serial.begin(9600);
  
  const char* configPath = "/config.txt";
  const char* apSsid = "LightManager";
  const char* apPassword = "ESP8266setup";
  
  SetupManager setupManager(configPath, apSsid, apPassword); //Initialize the setup manager with predefined config
  Config config = setupManager.getConfig(); //Get the MQTT connection config 
  
  Serial.println(config.mqtt_host);
  Serial.println(config.mqtt_port);
}
```

The `getConfig()` method will provide a `Config` instance (see https://github.com/leorod/light-manager/blob/master/lib/Setup/Setup.h) with the configured `mqtt_host` and `mqtt_port`.

### EventSource
This is the core of this project. The `EventSource` will be in charge of connecting to MQTT and consuming, parsing and handling messages.
Messages should come in JSON format with the following structure:
```json
{
  "uuid": "b60653b1-869b-4196-9282-c83fc19f401a",
  "type": "TOGGLE",
  "timestamp": 1593039043247,
  "source": "source-device",
  "value": "{\"target\": 0, \"state\": 0}"
}
```

Once deserialized, `Event`s (see [EventSource.h](https://github.com/leorod/light-manager/blob/master/lib/EventSource/EventSource.h)) are passed to a previously defined `EventHandler` &mdash;EventSource currently supports only one EventHandler, it will support adding more with different types in a future version&mdash;, tied to a message type. In this example we're using `TOGGLE` messages which are processed by the `ToggleEventHandler` in [Toggle.h](https://github.com/leorod/light-manager/blob/master/lib/EventSource/Toggle.h).

Once they are processed, messages are published to an *audit* topic (the same source topic the message came from, prefixed by `/audit`) for them to be consumed somewhere else and stored in some kind of audit log that could work as a registry of messages acknowledged by the ESP8266 (or any other use you may find for it). Since this is oddly specific an probably not needed for all uses, this feature may be removed or deactivated by configuration on future versions.

#### EventHandlers
So the `EventSource` works as an event listener and a &mdash;soon-to-be&mdash; orchestrator for `EventHandler`s.
`EventHandler` is an abstract class defined in [EventSource.h](https://github.com/leorod/light-manager/blob/master/lib/EventSource/EventSource.h), which defines the contract that any implementations of handlers must follow for `EventSource` to be able to interact with them.
Per its definition, every `EventHandler` must implement two public methods:
* `boolean accepts(Event event)`: This will determine, based on the passed `Event`'s `type` (or any other condition that may be required), whether the message can be handled by this `EventHandler` or not. Right now this just allows processing only known messages, in the future it will be used to process with the correct handler. 
* `void handle(Event event)`: This is where the actual processing of the `Event`'s instructions will happen.

##### ToggleEventHandler
Handles `TOGGLE` events by switching `Toggle`s based on the `value` attribute in the incoming event. This `value` must be a JSON string with the following structure for switching a single toggle:
```json
{"target": 1}
```
Where `target`'s value is a previously defined `Toggle` ID (see the `Toggle` struct in [Toggle.h](https://github.com/leorod/light-manager/blob/master/lib/EventSource/Toggle.h)). The referenced `Toggle`'s `state` will be inverted, meaning that if it's turned on it will be switched off, and viceversa.

This handler also supports switching on or off all of the `Toggle`s associated to the Handler, by using the following message structure:
```json
{"target": 0, "state": 1}
```
Setting the `target` to `0` means that *all* the `Toggle`s will be targeted.
The `state` property defines whether they will be turned ON (set to `1`) or OFF (set to `0`). If `state` is not specified, it will turn everything OFF.

#### Usage
The following example shows how to use `EventSource` with a `ToggleEventHandler`
```cpp
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Toggle.h"

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

EventSource *eventSource; //Set EventSource as a pointer, since it will be initialized in setup() once the MQTT config is ready.
Toggle toggles[]{
    {.id = 1, .pin = D1, .state = HIGH },
    {.id = 2, .pin = D2, .state = HIGH }
};
ToggleEventHandler eventHandler(toggles, 2); //Initialize the EventHandler with two toggles, 
                                             //wired to ESP8266's D1 and D2 pins, with internal IDs 1 and 2 respectively
                                             //both set to HIGH (OFF for my Relay module) by default

void setup() {
  const char* topic = "/my/topic/name";
  
  mqttClient.setServer(mqttHost, mqttPort); //Setup PubSubClient
  eventSource = new EventSource(mqttClient, eventHandler, topic); //Initialize EventSource
}

void loop() {
  eventSource->poll(); //Start polling for new messages in the subscribed topic.
}
```
