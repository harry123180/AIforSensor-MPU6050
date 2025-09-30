﻿# Raspberry Pi Pico 2 W Wi-Fi 使用指南

## 概述
Raspberry Pi Pico 2 W 內建 Infineon CYW43439 無線模組，可支援 2.4 GHz IEEE 802.11 b/g/n 網路標準。模組同時提供 Station 與 Soft-AP 模式，適合在 Edge Impulse AIoT 教學情境中進行感測資料上傳與現場設定工作。

## 連線能力與功耗
- **頻段**：2.4 GHz（2400–2484 MHz）
- **理論傳輸速率**：最高 65 Mbps；實測室內約 6–9 Mbps
- **功耗**：全速傳輸約 70–80 mA；待命模式低於 50 mA
- **天線**：板載 PCB 天線，需避免與金屬外殼或電源線太近
- **安全性**：建議使用 WPA2 或 WPA3，密碼長度控制在 8–31 字元以符合 EEPROM 緩衝限制

## Arduino IDE 設定步驟
1. 安裝 Arduino IDE 2.x，並在「偏好設定」中加入開發板網址：`https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`
2. 開啟「開發板管理員」，搜尋並安裝 **Raspberry Pi Pico/RP2040** 套件。
3. 選擇開發板：`Raspberry Pi Pico W`，建議將 CPU 速度設為 150 MHz、Flash 選擇 4 MB (default)。

## Station 模式連線範例
```cpp
#include <WiFi.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);              // 設定為 Station 模式
    WiFi.begin(ssid, password);       // 開始連線
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop() {}
```

## Soft-AP 模式範例
```cpp
#include <WiFi.h>
#include <WebServer.h>

const char* ap_ssid = "Pico2W-Config";
const char* ap_password = "12345678";
WebServer server(80);

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_password);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", [](){
        server.send(200, "text/html", "<h1>Pico 2 W AP Mode</h1>");
    });
    server.begin();
}

void loop() {
    server.handleClient();
}
```

## 疑難排解與最佳化建議
- 量測 RSSI：`WiFi.RSSI()` 低於 -80 dBm 時建議調整裝置位置或加裝外部天線。
- 定期檢查狀態：於主迴圈偵測 `WiFi.status()` 並在斷線後呼叫 `WiFi.reconnect()`。
- Soft-AP 模式建議手動指定通道，避免與同場設備互相干擾。
- 若需長時間省電，可搭配 `WiFi.setSleep(true)` 與 Watchdog 監控確保連線穩定。

以上設定可作為本專案中 CollectData 與 MQTTwithAI 韌體的連線基礎，請依實際網路環境調整參數。
