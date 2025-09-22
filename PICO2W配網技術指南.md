# Raspberry Pi Pico 2 W WiFi 使用指南

## 概述

Raspberry Pi Pico 2 W 採用 Infineon CYW43439 無線晶片，支援 IEEE 802.11b/g/n WiFi 和藍牙 5.2。本指南提供完整的 WiFi 功能實現方法，包含自動配網系統。

## 硬體規格

### WiFi 技術參數
- **標準**: IEEE 802.11b/g/n (WiFi 4)
- **頻段**: 2.4 GHz (2400-2484 MHz)
- **傳輸速率**: 最高 65 Mbps (理論值)
- **實際性能**: 9+ Mbps (近距離), 6+ Mbps (室內)
- **覆蓋範圍**: 開放空間最遠 30 公尺
- **天線**: 板載 PCB 天線
- **安全性**: 支援 WPA3/WPA2/WPA

### 功耗規格
- **運作功耗**: 360-412 mW (70-80 mA @ 5.15V)
- **待機功耗**: <50 mA
- **支援省電模式**: WiFi 省電模式、睡眠模式

## Arduino IDE 環境設置

### 1. 安裝開發板支援
```
檔案 → 偏好設定 → 額外的開發板管理員網址
添加: https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

工具 → 開發板 → 開發板管理員
搜尋: Raspberry Pi Pico/RP2040
安裝: Raspberry Pi Pico/RP2040 by Earle F. Philhower, III
```

### 2. 開發板選擇
```
工具 → 開發板 → Raspberry Pi Pico/RP2040 → Generic RP2350
或
工具 → 開發板 → Raspberry Pi Pico/RP2040 → Raspberry Pi Pico 2 W
```

### 3. 基本設定
```
工具 → CPU Speed → 150 MHz
工具 → Flash Size → 4MB (默認)
工具 → Boot Stage 2 → Generic SPI /4 (默認)
```

## 基本 WiFi 使用

### 簡單連接範例
```cpp
#include <WiFi.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

void setup() {
    Serial.begin(115200);
    
    // 設定為 Station 模式
    WiFi.mode(WIFI_STA);
    
    // 開始連接
    WiFi.begin(ssid, password);
    
    // 等待連接
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop() {
    // 主程式邏輯
}
```

### WiFi 狀態檢查
```cpp
// 檢查連接狀態
if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi is connected");
    Serial.println("SSID: " + WiFi.SSID());
    Serial.println("IP: " + WiFi.localIP().toString());
    Serial.println("Signal: " + String(WiFi.RSSI()) + " dBm");
    Serial.println("MAC: " + WiFi.macAddress());
}

// 重新連接
if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    // 或重新開始連接
    WiFi.begin(ssid, password);
}
```

### Access Point 模式
```cpp
#include <WiFi.h>
#include <WebServer.h>

const char* ap_ssid = "Pico2W-AP";
const char* ap_password = "12345678";

WebServer server(80);

void setup() {
    Serial.begin(115200);
    
    // 設定為 AP 模式
    WiFi.mode(WIFI_AP);
    
    // 啟動 Access Point
    WiFi.softAP(ap_ssid, ap_password);
    
    Serial.println("AP started");
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
    
    // 設定 Web 伺服器
    server.on("/", []() {
        server.send(200, "text/html", "<h1>Pico 2 W AP Mode</h1>");
    });
    
    server.begin();
}

void loop() {
    server.handleClient();
}
```

## 完整配網系統實現

### 核心功能特性
1. **自動配網**: 首次使用自動進入配網模式
2. **憑證儲存**: EEPROM 持久化儲存 WiFi 憑證
3. **網路掃描**: 自動掃描並顯示可用 WiFi 網路
4. **狀態監控**: 連接狀態監控和自動重連
5. **Web 介面**: 直觀的配網和管理介面
6. **重置功能**: 硬體按鈕和軟體重置

### 代碼架構設計

#### 資料結構
```cpp
struct WiFiConfig {
    char ssid[32];        // WiFi 網路名稱
    char password[64];    // WiFi 密碼
    bool valid;           // 資料有效性標記
};
```

#### 狀態管理
```cpp
bool configMode = false;     // 配網模式狀態
WiFiConfig wifiConfig;       // WiFi 配置結構
```

#### GPIO 配置
```cpp
const int LED_PIN = LED_BUILTIN;  // 狀態指示 LED
const int RESET_PIN = 2;          // 重置按鈕 GPIO
```

### 程式流程圖

```
開機啟動
    ↓
載入 EEPROM 配置
    ↓
檢查重置按鈕 → [按下] → 清除配置
    ↓
憑證有效? → [否] → 啟動配網模式
    ↓ [是]
嘗試連接已儲存 WiFi
    ↓
連接成功? → [否] → 啟動配網模式
    ↓ [是]
進入正常運作模式
```

### 配網模式工作流程

```
配網模式啟動
    ↓
建立 AP "Pico2W-Config"
    ↓
啟動 Web 伺服器 (192.168.42.1)
    ↓
使用者連接熱點
    ↓
瀏覽器訪問配網頁面
    ↓
掃描可用 WiFi 網路
    ↓
選擇網路並輸入密碼
    ↓
測試連接目標 WiFi
    ↓
連接成功 → 儲存憑證 → 切換正常模式
連接失敗 → 返回配網頁面
```

## 設計注意事項

### 1. EEPROM 資料驗證
```cpp
void loadConfig() {
    EEPROM.get(0, wifiConfig);
    
    // 檢查 SSID 有效性
    bool validSSID = true;
    int ssidLen = strlen(wifiConfig.ssid);
    
    if (ssidLen == 0 || ssidLen > 31) {
        validSSID = false;
    } else {
        // 檢查是否為可列印字符
        for (int i = 0; i < ssidLen; i++) {
            if (wifiConfig.ssid[i] < 32 || wifiConfig.ssid[i] > 126) {
                validSSID = false;
                break;
            }
        }
    }
    
    if (!validSSID) {
        // 清除損壞資料
        clearConfig();
    }
}
```

### 2. WiFi 模式切換
```cpp
// AP 模式啟動
WiFi.mode(WIFI_AP);
WiFi.softAP(ssid, password);

// STA 模式連接
WiFi.mode(WIFI_STA);
WiFi.begin(ssid, password);

// 混合模式 (不建議用於配網)
WiFi.mode(WIFI_AP_STA);
```

### 3. Web 伺服器管理
```cpp
// 停止現有伺服器
server.stop();
server.close();

// 清除路由後重新設置
setupWebServer();
server.begin();
```

### 4. 記憶體管理
- 避免大型 HTML 字串佔用過多記憶體
- 使用 String 拼接而非靜態字串
- 限制掃描結果數量 (建議 ≤10 個)

### 5. 錯誤處理
```cpp
// 連接超時處理
int attempts = 0;
while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    attempts++;
}

// 重連次數限制
static int reconnectAttempts = 0;
if (reconnectAttempts < 3) {
    // 嘗試重連
} else {
    // 返回配網模式
    startConfigMode();
}
```

## 完整範例程式

### 基本 WiFi 連接
```cpp
#include <WiFi.h>

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nConnected!");
    Serial.println("IP: " + WiFi.localIP().toString());
}

void loop() {
    // 檢查連接狀態
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi OK");
    } else {
        Serial.println("WiFi Lost");
    }
    delay(5000);
}
```

### Web 伺服器範例
```cpp
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

WebServer server(80);

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("Connected!");
    Serial.println("IP: " + WiFi.localIP().toString());
    
    // 設定路由
    server.on("/", []() {
        server.send(200, "text/html", "<h1>Hello from Pico 2 W!</h1>");
    });
    
    server.on("/status", []() {
        String json = "{";
        json += "\"ssid\":\"" + WiFi.SSID() + "\",";
        json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI());
        json += "}";
        server.send(200, "application/json", json);
    });
    
    server.begin();
}

void loop() {
    server.handleClient();
}
```

## 實際應用範例

### IoT 感測器節點
```cpp
#include <WiFi.h>
#include <HTTPClient.h>

void sendSensorData() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin("http://your-server.com/api/data");
        http.addHeader("Content-Type", "application/json");
        
        String payload = "{\"temperature\":25.6,\"humidity\":60.2}";
        int httpResponseCode = http.POST(payload);
        
        if (httpResponseCode > 0) {
            Serial.println("Data sent successfully");
        }
        
        http.end();
    }
}
```

### MQTT 客戶端
```cpp
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void setup() {
    // WiFi 連接後設定 MQTT
    mqtt.setServer("mqtt.broker.com", 1883);
    mqtt.setCallback(onMqttMessage);
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message received on topic: ");
    Serial.println(topic);
}

void loop() {
    if (!mqtt.connected()) {
        reconnectMQTT();
    }
    mqtt.loop();
}
```

## 除錯與故障排除

### 常見問題

#### 1. 無法掃描到熱點
**原因**: AP 模式啟動失敗
**解決方法**:
```cpp
// 確保正確的模式切換
WiFi.mode(WIFI_OFF);
delay(100);
WiFi.mode(WIFI_AP);
delay(500);
bool result = WiFi.softAP(ssid, password);
```

#### 2. 配網頁面無法訪問
**原因**: IP 位址不正確或 Web 伺服器未啟動
**解決方法**:
```cpp
// 獲取實際 IP 位址
IPAddress apIP = WiFi.softAPIP();
Serial.println("Access at: " + apIP.toString());

// 確保伺服器正確啟動
server.begin();
Serial.println("Web server started");
```

#### 3. WiFi 頻繁斷線
**原因**: 訊號不穩定或省電模式
**解決方法**:
```cpp
// 禁用省電模式
WiFi.setSleep(false);

// 設定重連機制
void checkWiFiConnection() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 30000) {
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.begin(ssid, password);
        }
        lastCheck = millis();
    }
}
```

#### 4. EEPROM 資料損壞
**原因**: 未初始化或寫入無效資料
**解決方法**:
```cpp
// 資料驗證
bool validateConfig(WiFiConfig& config) {
    if (!config.valid) return false;
    
    int len = strlen(config.ssid);
    if (len == 0 || len > 31) return false;
    
    // 檢查字符有效性
    for (int i = 0; i < len; i++) {
        if (config.ssid[i] < 32 || config.ssid[i] > 126) {
            return false;
        }
    }
    return true;
}
```

### 除錯工具

#### 序列埠監控
```cpp
// 詳細狀態輸出
void printWiFiStatus() {
    Serial.println("=== WiFi Status ===");
    Serial.println("Mode: " + String(WiFi.getMode()));
    Serial.println("Status: " + String(WiFi.status()));
    Serial.println("SSID: " + WiFi.SSID());
    Serial.println("IP: " + WiFi.localIP().toString());
    Serial.println("Gateway: " + WiFi.gatewayIP().toString());
    Serial.println("Subnet: " + WiFi.subnetMask().toString());
    Serial.println("DNS: " + WiFi.dnsIP().toString());
    Serial.println("MAC: " + WiFi.macAddress());
    Serial.println("RSSI: " + String(WiFi.RSSI()) + " dBm");
    Serial.println("==================");
}
```

#### 網路掃描除錯
```cpp
void debugNetworkScan() {
    Serial.println("Starting network scan...");
    int networks = WiFi.scanNetworks();
    
    Serial.println("Networks found: " + String(networks));
    for (int i = 0; i < networks; i++) {
        Serial.printf("%d: %s (%d dBm) %s\n", 
            i, 
            WiFi.SSID(i).c_str(), 
            WiFi.RSSI(i),
            WiFi.encryptionType(i) == ENC_TYPE_NONE ? "Open" : "Secured"
        );
    }
}
```

## 最佳實務建議

### 1. 連接管理
- 使用連接超時機制避免無限等待
- 實現重連機制處理暫時性斷線
- 監控信號強度，低於 -80 dBm 時考慮重連

### 2. 記憶體優化
- 避免在 loop() 中頻繁建立大型字串
- 使用靜態變數減少記憶體分配
- 限制網路掃描結果數量

### 3. 安全性考量
- 使用強密碼保護 AP 模式
- 實現配網超時機制
- 考慮加密儲存 WiFi 憑證

### 4. 用戶體驗
- 提供清楚的 LED 狀態指示
- 簡化配網介面操作
- 實現自動重定向功能

### 5. 穩定性設計
```cpp
// 看門狗機制
void setupWatchdog() {
    // 如果支援硬體看門狗
    // watchdog_enable(8000, 1); // 8秒超時
}

// 狀態恢復
void recoverFromError() {
    Serial.println("Recovering from error...");
    WiFi.mode(WIFI_OFF);
    delay(1000);
    
    // 重新初始化
    loadConfig();
    
    if (wifiConfig.valid) {
        connectWiFi();
    } else {
        startConfigMode();
    }
}
```

## 效能調優

### WiFi 功率管理
```cpp
// 設定 WiFi 功率
WiFi.setTxPower(WIFI_POWER_19_5dBm);  // 最高功率
// WiFi.setTxPower(WIFI_POWER_2dBm);  // 最低功率省電

// 省電模式
WiFi.setSleep(true);   // 啟用省電模式
WiFi.setSleep(false);  // 禁用省電模式 (建議)
```

### 連接優化
```cpp
// 設定靜態 IP 加速連接
IPAddress local_IP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
}
```

### 天線效能優化
- 保持天線區域無金屬遮蔽
- 避免將設備放在金屬外殼內
- 天線方向朝向路由器方向

## 部署與維護

### 版本管理
```cpp
const char* FIRMWARE_VERSION = "1.0.0";

void handleVersion() {
    String json = "{";
    json += "\"firmware\":\"" + String(FIRMWARE_VERSION) + "\",";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"free_memory\":" + String(rp2040.getFreeHeap());
    json += "}";
    server.send(200, "application/json", json);
}
```

### OTA 更新準備
```cpp
// 預留 OTA 更新接口
void handleOTA() {
    server.send(200, "text/html", 
        "<h1>OTA Update</h1>"
        "<form method='post' action='/update' enctype='multipart/form-data'>"
        "<input type='file' name='firmware' accept='.bin'>"
        "<input type='submit' value='Update'>"
        "</form>"
    );
}
```

### 生產環境部署檢查清單
- [ ] 修改預設 AP 密碼
- [ ] 設定適當的連接超時值
- [ ] 實現錯誤日誌記錄
- [ ] 添加設備識別信息
- [ ] 測試各種網路環境
- [ ] 驗證重置功能正常
- [ ] 確認記憶體使用量

## 總結

Raspberry Pi Pico 2 W 的 WiFi 功能強大且穩定，通過適當的程式設計可以實現：

1. **可靠的網路連接**: 自動重連和錯誤恢復
2. **用戶友善的配網**: Web 介面配網系統
3. **企業級穩定性**: 狀態監控和故障恢復
4. **低功耗運作**: 可配置的省電模式

關鍵成功因素：
- 正確的模式切換和狀態管理
- 穩健的錯誤處理和恢復機制
- 簡潔高效的 Web 介面設計
- 完善的資料驗證和儲存機制

使用本指南的方法，可以快速建構穩定可靠的 WiFi 連接應用，適用於各種 IoT 專案需求。