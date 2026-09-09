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
 * @brief Quản lý vòng đời kết nối WiFi, tự động thử lại khi mất mạng theo chu kỳ
 */
bool WiFiManager::checkAndEstablishWiFiConnection(unsigned long interval) {
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
    unsigned long currentMillis = millis();
    wifiStatus = (WiFi.status() == WL_CONNECTED);

    // 3. Nếu đã đủ thời gian interval hoặc lần đầu tiên chạy (lastWifiCheck == 0)
    if (currentMillis - lastWifiCheck >= interval || lastWifiCheck == 0) {
        lastWifiCheck = currentMillis;

        // Nếu mất kết nối thì tiến hành thử kết nối lại
        if (!wifiStatus) {
            WiFi.mode(WIFI_STA);
            WiFi.begin(configMgr.params.ssid.c_str(), configMgr.params.password.c_str());
            #if defined(DEBUG_WIFI_SETTING)
                Serial.println(F("[WiFi] Attempting to reconnect WiFi..."));
            #endif
        }
    }
    return wifiStatus; 
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