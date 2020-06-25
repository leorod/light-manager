#include "Commons.h"
#include <PubSubClient.h>

struct Event {
    char uuid[36];
    uint64_t timestamp;
    char* source;
    char* type;
    char* value;
};

class EventHandler {
    public:
        virtual boolean accepts(Event event) = 0;
        virtual void handle(Event event) = 0;
};

class EventSource {
    private:
        PubSubClient* _mqttClient;
        EventHandler* _handler;
        char* _topic;
        void init();
        void process(char* topic, byte* payload, unsigned int length);
        Event deserialize(char* message);
        char* serialize(Event event);
        void log(char* payload, char* sourceTopic);
        boolean reconnect(char* topic);
        boolean reconnect();
    public:
        EventSource(PubSubClient& mqttClient, EventHandler& handler, const char* topic);
        void poll();
};