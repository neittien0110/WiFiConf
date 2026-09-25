#include <ConfigManager.h>  // Cấu hình ghi bộ nhớ Flash
#include <WiFiManager.h>    // Quản lý wifi (Class mới)

// --------------------------------------------------------
// HÀM CHÍNH: setup()
// --------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  Serial.println(F("\nKhoi dong Wemos D1 Mini"));
  Serial.println(F("\nsetup()"));

  if (configMgr.begin())
  {
    Serial.println(F("\n - Load configuration from Flash"));
    configMgr.loadAll();
  }

  // Set MAC Address loaded from JSON config
  Serial.print(F("\n - New MAC: "));
  Serial.println(configMgr.params.macAddress);
  wifiMgr.setNewMac(configMgr.params.macAddress.c_str());

  Serial.print(F("\n - Is WiFi enabled: "));
  Serial.println(configMgr.params.wifiEnabled ? "Enabled" : "Disable");

  if (configMgr.params.wifiEnabled) {
    // Thử kết nối lần đầu
    wifiMgr.checkAndEstablishWiFiConnection(5000,1);
    Serial.print(F(" - - status: "));
    Serial.println(wifiMgr.getWifiStatus() ? "Ok" : "Dis");

    if (wifiMgr.getWifiStatus()) {
      bool internetOk = wifiMgr.isInternetReady();
      Serial.print(F(" - - try to internet: "));
      Serial.println(internetOk ? "Ok" : "Dis");
    }
  }

  delay(2000);
}

// --------------------------------------------------------
// HÀM CHÍNH: loop()
// --------------------------------------------------------
void loop()
{
  Serial.println(F("\nLOOP"));

  // Cập nhật và duy trì kết nối WiFi
  wifiMgr.checkAndEstablishWiFiConnection(60000,3);

  // Kiểm tra trạng thái bằng phương thức trong class
  if (wifiMgr.getWifiStatus()) {
    Serial.println(F("\nWiFi is ok."));
  }

  if (false) {
    Serial.println(F("Tắt Wifi"));
    wifiMgr.shutdownWiFi();
    delay(5000);
  }

  if (false) {
    Serial.println(F("Bật WiFi"));
    wifiMgr.wakeupWiFi();
    delay(5000);
  }

  // Nếu bật WiFi trong config mà hiện tại mất kết nối -> Mở trang đăng ký
  if (configMgr.params.wifiEnabled && !wifiMgr.getWifiStatus()) {
    Serial.println(F("  "));
    wifiMgr.registerWiFi(WiFiRegistrationMethod::SELF_STATION);
  }  
}