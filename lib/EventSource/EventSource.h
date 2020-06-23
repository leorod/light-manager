#include <PubSubClient.h>

struct Event {
    char uuid[36];
    uint64_t timestamp;
    char* source;
    char* type;
    char* value;
    char* topic;
};

class EventHandler {
    public:
        void handle(Event);
};

class EventSource {
    private:
        PubSubClient* _mqttClient;
        char* _topic;
        void init();
        void process(char* topic, byte* payload, unsigned int length);
        Event deserialize(char* message);
        char* serialize(Event event);
        void log(Event event);
        boolean reconnect(char* topic);
        boolean reconnect();
    public:
        EventSource(PubSubClient& mqttClient, const char* topic);
        void poll();
};