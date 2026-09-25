#include "ConfigManager.h"

ConfigManager::ConfigManager() {}

bool ConfigManager::begin() {
    return LittleFS.begin();
}

void ConfigManager::loadDefaults() {
    params.netCount = 1;
    params.net[0].ssid = "SOICT_CORE_BOARD";
    params.net[0].password = "12345678";
    params.lastOkSsid = "SOICT_CORE_BOARD";
    params.macAddress = "24:24:B7:D0:B6:1C";
    params.deviceID = "tiennd";
    params.wifiEnabled = true;
}

void ConfigManager::loadAll() {
    if (!LittleFS.exists(CONFIG_FILE)) {
        loadDefaults();
        saveAll();
        return;
    }

    File f = LittleFS.open(CONFIG_FILE, "r");
    if (!f) {
        loadDefaults();
        saveAll();
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, f);
    f.close();

    if (error) {
        loadDefaults();
        saveAll();
        return;
    }

    // Read general configurations
    params.lastOkSsid = doc["ok"] | "SOICT_CORE_BOARD";
    params.macAddress = doc["mac"] | "24:24:B7:D0:B6:1C";
    params.deviceID = doc["dev_id"] | "tiennd";
    params.wifiEnabled = doc["wifi_en"] | true;

    // Read WiFi Array
    JsonArray netArr = doc["net"].as<JsonArray>();
    params.netCount = 0;

    for (JsonObject obj : netArr) {
        if (params.netCount >= NUM_OF_SSID) break;
        params.net[params.netCount].ssid = obj["s"].as<String>();
        params.net[params.netCount].password = obj["p"].as<String>();
        params.netCount++;
    }

    if (params.netCount == 0) {
        loadDefaults();
        saveAll();
    }
}

void ConfigManager::saveAll() {
    JsonDocument doc;

    JsonArray netArr = doc["net"].to<JsonArray>();
    for (uint8_t i = 0; i < params.netCount; i++) {
        JsonObject item = netArr.add<JsonObject>();
        item["s"] = params.net[i].ssid;
        item["p"] = params.net[i].password;
    }

    doc["ok"] = params.lastOkSsid;
    doc["mac"] = params.macAddress;
    doc["dev_id"] = params.deviceID;
    doc["wifi_en"] = params.wifiEnabled;

    File f = LittleFS.open(CONFIG_FILE, "w");
    if (f) {
        serializeJson(doc, f);
        f.close();
    }
}

void ConfigManager::setLastOkSsid(String ssid) {
    params.lastOkSsid = ssid;
    saveAll();
}

void ConfigManager::setMacAddress(String mac) {
    params.macAddress = mac;
    saveAll();
}

void ConfigManager::setDeviceID(String id) {
    params.deviceID = id;
    saveAll();
}

void ConfigManager::setWifiEnabled(bool enabled) {
    params.wifiEnabled = enabled;
    saveAll();
}

void ConfigManager::addOrUpdateWiFi(String ssid, String pass) {
    // 1. If SSID already exists, update password and move it to index 0 (newest)
    for (uint8_t i = 0; i < params.netCount; i++) {
        if (params.net[i].ssid == ssid) {
            params.net[i].password = pass;
            WiFiCredential temp = params.net[i];
            for (int j = i; j > 0; j--) {
                params.net[j] = params.net[j - 1];
            }
            params.net[0] = temp;
            saveAll();
            return;
        }
    }

    // 2. If below capacity, shift right and insert at index 0
    if (params.netCount < NUM_OF_SSID) {
        for (int i = params.netCount; i > 0; i--) {
            params.net[i] = params.net[i - 1];
        }
        params.net[0].ssid = ssid;
        params.net[0].password = pass;
        params.netCount++;
    } 
    // 3. Array full: Replace the oldest SSID (not matching params.lastOkSsid)
    else {
        int replaceIdx = -1;
        // Search from back (oldest) to front
        for (int i = NUM_OF_SSID - 1; i >= 0; i--) {
            if (params.net[i].ssid != params.lastOkSsid) {
                replaceIdx = i;
                break;
            }
        }

        // Safety fallback if all entries equal lastOkSsid
        if (replaceIdx == -1) replaceIdx = NUM_OF_SSID - 1;

        // Shift items after replaceIdx up, then shift preceding items right to insert at front
        for (int i = replaceIdx; i < NUM_OF_SSID - 1; i++) {
            params.net[i] = params.net[i + 1];
        }
        for (int i = NUM_OF_SSID - 1; i > 0; i--) {
            params.net[i] = params.net[i - 1];
        }

        params.net[0].ssid = ssid;
        params.net[0].password = pass;
    }

    saveAll();
}

ConfigManager configMgr;