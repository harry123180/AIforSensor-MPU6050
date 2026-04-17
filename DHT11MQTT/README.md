# DHT11MQTT — DHT11 溫濕度 MQTT 上傳

平行於 `MQTTwithAI/` 的教學範例：以 bit-bang 方式讀取 DHT11 溫濕度（無需額外函式庫），每 3 秒上傳一次。適合示範「非 IMU」感測器如何套用同一套 Wi-Fi 配網與 MQTT 管線。

## 硬體腳位

| 訊號 | GPIO |
| ---- | ---- |
| DHT11 DATA | GP0 |
| Reset 按鈕 | GP2 → GND |

DHT11 VCC 接 3V3，DATA 需接 10 kΩ 上拉到 3V3。

## 預設參數

| 項目 | 值 |
| ---- | ---- |
| 配網 AP SSID | `Pico2W-Config` |
| 配網 AP 密碼 | `12345678`（⚠ 教學用弱密碼） |
| 預設 MQTT broker | `mqtt.trillionsr.com.tw:1883` |
| 感測器讀取 / 發佈間隔 | 每 3 秒 |

## 相依函式庫

```bash
arduino-cli lib install PubSubClient ArduinoJson
```

DHT11 以內建的 GPIO 計時實作讀取，不需 DHT 函式庫。

## 配網與 MQTT 主題

- 配網流程同 [`MQTTwithAI/README.md`](../MQTTwithAI/README.md)。
- 發佈主題：`<device_id>/dht11` → `{temperature, humidity, timestamp}`。

## 編譯與上傳

```bash
arduino-cli compile --fqbn rp2040:rp2040:pico_w DHT11MQTT/DHT11MQTT.ino
arduino-cli upload  --fqbn rp2040:rp2040:pico_w --port <COMx> DHT11MQTT/DHT11MQTT.ino
```

## 與 MQTTwithAI 的關係

兩份草稿共享大量 Wi-Fi/EEPROM/MQTT 樣板（約 600 行重疊）。教學上可擇一示範；若要抽離共用邏輯為函式庫，建議以 [`WifiConnector/`](../WifiConnector/) 為基礎自行擴充。
