# WifiConnector — Wi-Fi AP 配網樣板

獨立可編譯的 Wi-Fi 配網骨架：首次開機開啟 AP、以 WebServer 提供表單收 SSID／密碼 → 寫入 EEPROM → 下次開機以 STA 連線。**本目錄不包含感測器或 MQTT**，純粹作為「Wi-Fi 配網邏輯」的參考實作，方便抽離到新專案。

## 硬體腳位

| 訊號 | GPIO |
| ---- | ---- |
| 狀態 LED | `LED_BUILTIN` |
| Reset 按鈕 | GP2 → GND |

## 預設參數

| 項目 | 值 |
| ---- | ---- |
| AP SSID | `Pico2W-Config` |
| AP 密碼 | `12345678`（⚠ 教學用弱密碼） |
| EEPROM 大小 | 512 bytes |

## 相依函式庫

內建 `WiFi`、`WebServer`、`EEPROM`，無需額外安裝。

## 編譯與上傳

```bash
arduino-cli compile --fqbn rp2040:rp2040:pico_w WifiConnector/WifiConnector.ino
arduino-cli upload  --fqbn rp2040:rp2040:pico_w --port <COMx> WifiConnector/WifiConnector.ino
```

## 流程

1. `loadConfig()` 從 EEPROM 讀回 `WiFiConfig`，檢查 `valid` 旗標。
2. 有效且 `connectWiFi()` 成功 → 進入 `startNormalMode()`。
3. 失敗或按住 RESET_PIN 開機 → 清空 EEPROM 並進入 `startConfigMode()`，AP 啟動於 `192.168.4.1`。

## EEPROM 配置

```
offset 0..31   wifi_ssid       (char[32])
offset 32..95  wifi_password   (char[64])
offset 96      valid           (bool)
```

## 何時使用

- 要把 Wi-Fi 配網功能移植到新草稿：從本檔案剪下 `loadConfig / saveConfig / startConfigMode / connectWiFi` 四個函式即可。
- `MQTTwithAI/` 與 `DHT11MQTT/` 目前各自複製了這份配網邏輯；未來若要抽成共用 library，以本目錄為起點。
