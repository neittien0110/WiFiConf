#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP32)
    #include <WiFi.h>
#elif defined(ARDUINO_ARCH_ESP8266)
    #include <ESP8266WiFi.h>
#endif

class WiFiSelfEnroll;

#define WIFI_SSID_NAME "WIFI_ADHOC"
#define WIFI_SSID_PASS ""
#define WIFI_CHECK_INTERVAL 60000 

#define DEBUG_WIFI_SETTING

enum class WiFiRegistrationMethod {
    SELF_STATION,
    SERIAL_INTERFACE
};

class WiFiManager {
private:
    bool wifiStatus = false;
    unsigned long lastWifiCheck = 0;
    WiFiSelfEnroll *myWiFi = nullptr;

public:
    WiFiManager();
    ~WiFiManager();

    bool checkAndEstablishWiFiConnection(unsigned long interval = WIFI_CHECK_INTERVAL);
    bool isInternetReady();
    void wakeupWiFi();
    void shutdownWiFi();
    bool registerWiFi(WiFiRegistrationMethod method = WiFiRegistrationMethod::SELF_STATION);
    void setNewMac(const char* newMacStr);

    /**
     * @brief Lấy trạng thái kết nối WiFi hiện tại của hệ thống.
     * @return true nếu đã kết nối thành công, false nếu chưa/mất kết nối.
     */
    bool getWifiStatus() const { return wifiStatus; }
    
    // Tương đương với getWifiStatus() nếu thích dùng cú pháp kiểu boolean
    bool isConnected() const { return wifiStatus; }
};

extern WiFiManager wifiMgr;

#endif