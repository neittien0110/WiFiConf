# SOICT CORE Version 2

## Enviroment

- IDE: Visual Studio Code + Platform IO
- Architecture: esp32,esp8266
  - Tested: [esp32doit-devkit-v1](https://docs.platformio.org/en/latest/boards/espressif32/esp32doit-devkit-v1.html), [esp32-c3-devkitm-1](https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32c3/hw-reference/esp32c3/user-guide-devkitm-1.html)
  - Tested: [Wemos D1 Mini V3.0.0](https://grobotronics.com/wemos-d1-mini-esp8266-v2.0.html?sl=en)

---

## Tính năng mới

- Lưu trữ lâu dài bộ tham số cấu hình gồm:
  1. 5 ssid/password
  2. deviceid do người dùng tự đặt
  3. địa chỉ mac để spoofing
  4. sử dụng, hoặc dừng wifi
- Có thể cấu hình qua 2 đường: wifi Adhoc, hoặc serial  
  
## Data Object lưu trữ cấu hình vĩnh viễn trong Flash

```json
  {
    "net": [
      {"s": "Home_WiFi", "p": "88888888"},
      {"s": "HUST_B1", "p": ""}
      {"s": "SOICT_LAB", "p": "12345678"}
    ],
    "ok": "HUST_B1",
    "mac": "24:24:B7:D0:B6:1C",
    "dev_id": "mydevice",
    "wifi_en": true
  }
```

Field | Description
-- | --
ok | tên mạng ssid gần đây nhất thành công. Khi thử kết nối mạng wifi, phải thử với mạng này đầu tiên. Nếu mạng này không thành công, hãy thừ với mạng wifi liền sau trong danh sách.
mac | địa chỉ mac để spoofing. Bỏ trống nếu không sử dụng.
dev_id | id định danh thiết bị, do người dùng tự đặt
wifi_en | cho phép bật wifi

## Mã nguồn

- [WiFiManger](./src/WiFiManager.h): lớp vỏ, đóng gói toàn bộ truy cập wifi trong vài hàm đơn giản/
- [WiFiEnrollByerial](./src/WiFiEnrollBySerial.h): cấu hình [bộ tham số](#data-object-lưu-trữ-cấu-hình-vĩnh-viễn-trong-flash) qua serial với các lệnh theo qui ước.
- [WiFiSelfEnroll](./src/WiFiSelfEnroll.h): cấu hình [bộ tham số] qua [WiFiAdhoc]() với các lệnh theo qui ước.
- [WiFiSelfEnroll](./src/WiFiSelfEnroll.h): xây dựng trang WiFi Adhoc

## Issues

- [Xem ở đây](https://github.com/neittien0110/WiFiConf/issues)