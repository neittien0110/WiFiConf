#include <LittleFS.h>
#include "WiFiManager.h"
#include "WiFiSelfEnroll.h"     // Module quản lý đăng ký WiFi bằng Web Captive Portal
#include "WiFiEnrollBySerial.h" // Module quản lý đăng ký WiFi qua câu lệnh Serial
#include "ConfigManager.h"

#if defined(ARDUINO_ARCH_ESP8266)
extern "C" {
  #include <user_interface.h>  // Thư viện low-level cần thiết để đổi MAC trên ESP8266
}
#endif

/** @brief Khởi tạo đối tượng toàn cục wifiMgr */
WiFiManager wifiMgr;

/**
 * @brief Constructor: Thiết lập các giá trị khởi tạo ban đầu cho các thuộc tính private
 */
WiFiManager::WiFiManager() {
    wifiStatus = false;
    lastWifiCheck = 0;
    myWiFi = nullptr;
    currentNetIndex = -1;
    attemptsOnCurrentSsid = 0;
}

/**
 * @brief Destructor: Thu hồi bộ nhớ dynamic đã cấp phát cho myWiFi
 */
WiFiManager::~WiFiManager() {
    if (myWiFi != nullptr) {
        delete myWiFi;
        myWiFi = nullptr;
    }
}

/**
 * @brief Tìm vị trí của SSID "ok" cuối cùng trong danh sách configMgr.params.net
 */
int WiFiManager::findOkSsidIndex() {
    if (configMgr.params.netCount == 0) return 0;

    for (uint8_t i = 0; i < configMgr.params.netCount; i++) {
        if (configMgr.params.net[i].ssid == configMgr.params.lastOkSsid) {
            return (int)i;
        }
    }
    return 0; // Fallback to index 0 if not found
}

/**
 * @brief Thực hiện kiểm tra DNS tới một domain cụ thể để xác nhận kết nối Internet
 */
bool WiFiManager::isInternetReady() {
    // Nếu chưa nối vào Router thì chắc chắn không có Internet
    if (WiFi.status() != WL_CONNECTED) return false;
    
    // Thử phân giải tên miền của Server
    IPAddress remote_ip;
    if (WiFi.hostByName("toolhub.app", remote_ip) == 1) {
        return true; // Phân giải DNS thành công -> Có Internet thực
    }
    return false;
}

/**
 * @brief Quản lý vòng đời kết nối WiFi, hỗ trợ xoay vòng SSID theo cấu hình non-blocking.
 */
bool WiFiManager::checkAndEstablishWiFiConnection(unsigned long reconnect_interval, unsigned int ssid_interval) {
    // 1. Kiểm tra nếu tính năng WiFi bị vô hiệu hóa trong cấu hình
    if (!configMgr.params.wifiEnabled) {
        if (wifiStatus) {
            WiFi.disconnect();
            wifiStatus = false;
            #if defined(DEBUG_WIFI_SETTING)
                Serial.println(F("[WiFi] WiFi Disabled in config. Disconnected."));
            #endif
        }
        return false;
    }

    // 2. Kiểm tra trạng thái thực tế từ thư viện WiFi
    if (WiFi.status() == WL_CONNECTED) {
        wifiStatus = true;
        attemptsOnCurrentSsid = 0; // Reset counter upon active connection
        return true;
    }

    wifiStatus = false;

    // 3. Throttle execution based on reconnect_interval (Passthrough non-blockingly)
    unsigned long currentMillis = millis();
    if (lastWifiCheck != 0 && (currentMillis - lastWifiCheck < reconnect_interval)) {
        return false; // Return immediately to avoid blocking main loop()
    }
    lastWifiCheck = currentMillis;

    // Sanity check: Ensure we have registered network configurations
    if (configMgr.params.netCount == 0) {
        #if defined(DEBUG_WIFI_SETTING)
            Serial.println(F("[WiFi] No WiFi credentials available."));
        #endif
        return false;
    }

    // 4. Determine which SSID to try
    if (ssid_interval == 0) {
        // Mode 0: Only use the designated "ok" SSID
        currentNetIndex = findOkSsidIndex();
    } else {
        // Mode != 0: Initialize index if unset or out of bounds
        if (currentNetIndex < 0 || currentNetIndex >= (int)configMgr.params.netCount) {
            currentNetIndex = findOkSsidIndex();
        }
    }

    String targetSsid = configMgr.params.net[currentNetIndex].ssid;
    String targetPass = configMgr.params.net[currentNetIndex].password;

    #if defined(DEBUG_WIFI_SETTING)
        Serial.printf("[WiFi] Attempting connection to index %d: '%s'...\n", currentNetIndex, targetSsid.c_str());
    #endif

    // Trigger connection request
    WiFi.mode(WIFI_STA);
    WiFi.begin(targetSsid.c_str(), targetPass.c_str());

    // Brief check after trigger
    if (WiFi.status() == WL_CONNECTED) {
        wifiStatus = true;
        attemptsOnCurrentSsid = 0;
        
        // Success: update "ok" SSID in config if modified
        if (configMgr.params.lastOkSsid != targetSsid) {
            #if defined(DEBUG_WIFI_SETTING)
                Serial.printf("[WiFi] Connected to new network! Updating 'ok' SSID to '%s'\n", targetSsid.c_str());
            #endif
            configMgr.setLastOkSsid(targetSsid);
        }
        return true;
    }

    // 5. Connection failed on this tick
    if (ssid_interval != 0) {
        attemptsOnCurrentSsid++;
        
        // Advance to next SSID if attempt limit reached
        if (attemptsOnCurrentSsid >= ssid_interval) {
            attemptsOnCurrentSsid = 0;
            currentNetIndex = (currentNetIndex + 1) % configMgr.params.netCount;
            #if defined(DEBUG_WIFI_SETTING)
                Serial.printf("[WiFi] Max attempts reached. Rotating to next SSID index: %d\n", currentNetIndex);
            #endif
        }
    }

    return false; 
}

/**
 * @brief Khôi phục hoạt động của RF WiFi chip
 */
void WiFiManager::wakeupWiFi() {
#if defined(DEBUG_WIFI_SETTING)
    Serial.println(F("[WiFi] Waking up WiFi..."));
#endif

#if defined(ARDUINO_ARCH_ESP8266)
    // Đánh thức ESP8266 khỏi chế độ forceSleep
    WiFi.forceSleepWake();
    delay(1);
#endif

    // Khởi động lại chế độ Station
    WiFi.mode(WIFI_STA);

    // Lưu trạng thái đã bật WiFi vào cấu hình
    configMgr.setWifiEnabled(true);
}

/**
 * @brief Tắt WiFi và đưa chip vào chế độ tiết kiệm điện tối đa
 */
void WiFiManager::shutdownWiFi() {
#if defined(DEBUG_WIFI_SETTING)
    Serial.println(F("[WiFi] Shutting down WiFi to save power..."));
#endif

    WiFi.disconnect(true); // Ngắt kết nối và xóa cấu hình tạm thời
    WiFi.mode(WIFI_OFF);   // Tắt hoàn toàn chip Radio RF

#if defined(ARDUINO_ARCH_ESP8266)
    WiFi.forceSleepBegin(); // Lệnh đặc biệt giúp ESP8266 giảm tối đa dòng tiêu thụ
#endif

    // Lưu trạng thái đã tắt WiFi vào cấu hình
    configMgr.setWifiEnabled(false);
}

/**
 * @brief Điều hướng quy trình đăng ký WiFi dựa vào enum phương thức truyền vào
 */
bool WiFiManager::registerWiFi(WiFiRegistrationMethod method) {
    switch (method) {
        case WiFiRegistrationMethod::SELF_STATION: {
            // Khởi tạo lười (Lazy initialization) đối tượng WiFiSelfEnroll nếu chưa có
            if (myWiFi == nullptr) {
                myWiFi = new WiFiSelfEnroll();
            }
            // Bật trạm phát AP / Web Portal để người dùng nhập thông tin
            myWiFi->setup();
            return true;
        }
        case WiFiRegistrationMethod::SERIAL_INTERFACE: {
            // Gọi hàm xử lý đọc câu lệnh Serial từ module WiFiEnrollBySerial
            return EnrollBySerial();
        }
    }
    return false;
}

/**
 * @brief Tách chuỗi MAC Hex thành mảng 6 byte và áp dụng vào phần cứng WiFi
 */
void WiFiManager::setNewMac(const char* newMacStr) {
    // Kiểm tra tính hợp lệ của chuỗi truyền vào
    if (newMacStr == NULL || strlen(newMacStr) == 0) {
        #if defined(DEBUG_WIFI_SETTING)
            Serial.println(F("[MAC] No new MAC provided. Using default."));
        #endif
        return;
    }

    uint8_t mac[6];
    int values[6];
    
    // Đọc và tách 6 cặp số Hex từ dạng "XX:XX:XX:XX:XX:XX"
    if (sscanf(newMacStr, "%x:%x:%x:%x:%x:%x", 
        &values[0], &values[1], &values[2], 
        &values[3], &values[4], &values[5]) == 6) 
    {
        for (int i = 0; i < 6; i++) {
            mac[i] = (uint8_t)values[i];
        }

        WiFi.mode(WIFI_STA); 
#if defined(ARDUINO_ARCH_ESP8266)
        // Thiết lập địa chỉ MAC cho giao diện Station trên ESP8266
        if (wifi_set_macaddr(STATION_IF, mac)) {
            #if defined(DEBUG_WIFI_SETTING)
                Serial.printf("[MAC] Changed to: %s\n", newMacStr);
            #endif
        } else {
            #if defined(DEBUG_WIFI_SETTING)
                Serial.println(F("[MAC] Set failed!"));
            #endif
        }
#elif defined(ARDUINO_ARCH_ESP32)
        // Thiết lập địa chỉ MAC cho giao diện Station trên ESP32
        if (esp_wifi_set_mac(WIFI_IF_STA, mac) == ESP_OK) {
            #if defined(DEBUG_WIFI_SETTING)
                Serial.printf("[MAC] Changed to: %s\n", newMacStr);
            #endif
        } else {
            #if defined(DEBUG_WIFI_SETTING)
                Serial.println(F("[MAC] Set failed!"));
            #endif
        }
#endif
    } else {
        #if defined(DEBUG_WIFI_SETTING)
            Serial.println(F("[MAC] Invalid format! Use XX:XX:XX:XX:XX:XX"));
        #endif
    }
}