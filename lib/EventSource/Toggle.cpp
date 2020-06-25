#include "Toggle.h"
#include <ArduinoJson.h>

ToggleEventHandler::ToggleEventHandler(Toggle* toggles, size_t toggleSize) {
    _toggles = toggles;
    _toggleSize = toggleSize;
}

boolean ToggleEventHandler::accepts(Event event) {
    return strcmp("TOGGLE", event.type) == 0;
}

void ToggleEventHandler::handle(Event event) {
    DB_PRINTLN("TOGGLE event received");
    const int capacity = JSON_OBJECT_SIZE(2) + 13;
    StaticJsonDocument<capacity> jsonValue;
    deserializeJson(jsonValue, strdup(event.value));
    const int targetPin = jsonValue["target"];
    if (targetPin == 0) {
        const int state = jsonValue["state"];
        DB_PRINT("Toggling all ");
        DB_PRINTLN(state == HIGH ? "ON" : "OFF");
        switchAll(state);
    } else if (Toggle* toggle = getToggle(targetPin)) {
        DB_PRINT("Toggling #");
        DB_PRINTLN(toggle->id);
        switchToggle(*toggle);
    }
}

Toggle* ToggleEventHandler::getToggle(const int target) {
    Toggle* toggle = nullptr;
    for (int i = 0; i < _toggleSize && !toggle; i++) {
        if (_toggles[i].id == target) {
            toggle = &_toggles[i];
        }
    }
    return toggle;
}

void ToggleEventHandler::switchToggle(Toggle &toggle) {
    toggle.state = toggle.state == HIGH ? LOW : HIGH;
    digitalWrite(toggle.pin, toggle.state);
}

void ToggleEventHandler::switchAll(const int state) {
    int newState = state == 1 ? LOW : HIGH;
    for (int i = 0; i < _toggleSize; i++) {
        digitalWrite(_toggles[i].pin, newState);
    }
}