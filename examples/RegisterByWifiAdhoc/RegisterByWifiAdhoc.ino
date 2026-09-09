#include <ConfigManager.h>  // Cấu hình ghi bộ nhớ Flash
#include <WiFiManager.h>    // Quản lý wifi (Class mới)
#include <ButtonGestures.h> // Quản lý nút bấm

#define MAC_SPOOFING_ADDRESS "24:24:B7:D0:B6:1C"

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

  Serial.print(F("\n - New MAC: "));
  Serial.println(MAC_SPOOFING_ADDRESS);
  wifiMgr.setNewMac(MAC_SPOOFING_ADDRESS);

  Serial.print(F("\n - Is WiFi enabled: "));
  Serial.println(configMgr.params.wifiEnabled ? "Enabled" : "Disable");

  if (configMgr.params.wifiEnabled) {
    // Thử kết nối lần đầu
    wifiMgr.checkAndEstablishWiFiConnection();
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
  wifiMgr.checkAndEstablishWiFiConnection();

  // Kiểm tra trạng thái bằng phương thức trong class
  if (wifiMgr.getWifiStatus()) {
    Serial.println(F("\nWiFi is ok."));
  }

  if (true) {
    Serial.println(F("Tắt Wifi"));
    wifiMgr.shutdownWiFi();
    delay(5000);
  }

  if (true) {
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