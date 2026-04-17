# MQTTwithAI — 推論 + Wi-Fi + MQTT 整合

在 `Inferencing/` 的基礎上加入 Wi-Fi 設定頁、EEPROM 儲存、MQTT 發佈：裝置端完成推論後，將分類結果與信心值透過 MQTT 上傳到 broker。適合課程後段示範 AIoT 完整鏈路。

## 硬體腳位

| 訊號 | GPIO |
| ---- | ---- |
| MPU6050 SDA / SCL | GP14 / GP15 |
| Reset 按鈕（清除 Wi-Fi 設定） | GP2 → GND |

## 預設參數

| 項目 | 值 |
| ---- | ---- |
| 配網 AP SSID | `Pico2W-Config` |
| 配網 AP 密碼 | `12345678`（⚠ 教學用弱密碼，正式部署務必修改） |
| 預設 MQTT broker | `mqtt.trillionsr.com.tw:1883` |
| 狀態回報間隔 | 每 5 分鐘 |
| 發佈節流下限 | 每 1 秒 |

## 相依函式庫

```bash
arduino-cli lib install Button2 PubSubClient ArduinoJson
```

Edge Impulse 函式庫：`EdgeAI_inferencing`（請確認草稿首行 `#include` 對齊實際匯出名稱）。

## 配網流程

1. 首次開機或 EEPROM 無效 → 啟動 AP 模式。
2. 手機／筆電連上 `Pico2W-Config`（密碼 `12345678`），瀏覽器打開 `http://192.168.4.1`。
3. 於表單填入 Wi-Fi SSID、密碼、MQTT server、port、device_id → 送出。
4. 裝置寫入 EEPROM 並重啟，進入 STA 模式連上目標網路。
5. 如需重新配網：按住 GP2 (RESET_PIN) 接地後開機，EEPROM 會被清空。

詳細操作見 [docs/zh-TW/pico2w-wifi-guide.md](../docs/zh-TW/pico2w-wifi-guide.md)。

## MQTT 主題

| 主題 | 方向 | 內容 |
| ---- | ---- | ---- |
| `<device_id>/inference` | 上傳 | 推論 JSON：`{predicted_class, confidence, timestamp}` |
| `<device_id>/status`    | 上傳 | 每 5 分鐘心跳：`{uptime, rssi, free_heap}` |

## 編譯與上傳

```bash
arduino-cli compile --fqbn rp2040:rp2040:pico_w MQTTwithAI/MQTTwithAI.ino
arduino-cli upload  --fqbn rp2040:rp2040:pico_w --port <COMx> MQTTwithAI/MQTTwithAI.ino
```

## 已知限制

- Wi-Fi 與 MQTT 共用 Core 0，連線異常時會阻塞推論迴圈。
- 本草稿同時涵蓋設定 / 網路 / 推論三種關注點，行數約 1000 行。若要重用 Wi-Fi 配網邏輯，另見 [`WifiConnector/`](../WifiConnector/)。
