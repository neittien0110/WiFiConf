#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define CONFIG_FILE "/_sys_config.json"
#ifndef NUM_OF_SSID
#define NUM_OF_SSID 5
#endif

struct WiFiCredential {
    String ssid;
    String password;
};

struct PersistentParams {
    WiFiCredential net[NUM_OF_SSID];
    uint8_t netCount = 0;
    String lastOkSsid;
    String macAddress;
    String deviceID;
    bool wifiEnabled;
};

class ConfigManager {
public:
    PersistentParams params;

    ConfigManager();
    bool begin();
    void loadAll();
    void saveAll();

    // Helper operations
    void setWifiEnabled(bool enabled);
    void setDeviceID(String id);
    void setMacAddress(String mac);
    void setLastOkSsid(String ssid);
    void addOrUpdateWiFi(String ssid, String pass);

private:
    void loadDefaults();
};

extern ConfigManager configMgr; 

#endif // CONFIG_MANAGER_H