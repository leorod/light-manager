#include "Commons.h"

struct Config {
  char mqtt_host[40];
  char mqtt_port[6];
};

class SetupManager {
    private:
        char* _configPath;
        char* _apSsid;
        char* _apPassword;
        Config _currentConfig;

        void init();
        Config loadConfig();
        bool saveConfig(Config config);
    public:
        SetupManager(const char* configPath, const char* apSsid, const char* apPassword);
        Config getConfig();
};