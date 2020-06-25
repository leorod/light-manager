#include "Commons.h"
#include "EventSource.h"

struct Toggle {
    int id;
    int pin;
    int state;
};

class ToggleEventHandler : public EventHandler {
    private:
        size_t _toggleSize;
        Toggle* _toggles;
        void switchToggle(Toggle &toggle);
        void switchAll(int state);
        Toggle* getToggle(const int target);
    public:
        ToggleEventHandler(Toggle* toggles, size_t toggleSize);
        boolean accepts(Event event);
        void handle(Event event);
};