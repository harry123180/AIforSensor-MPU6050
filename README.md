# Raspberry Pi Pico 2 W IMU AIoT 教學專案
[English Version](#english-version)

# Arduino IDE 安裝URL
``` 
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
```
## 專案簡介
- 使用 Raspberry Pi Pico 2 W 與 MPU6050 三軸 IMU，以 1 kHz 取樣速度蒐集動作資料並保存於 SD 卡。
- 透過 Edge Impulse 訓練 TinyML 模型，於裝置端即時推論並透過 Wi-Fi/MQTT 上傳結果。
- 教學示範涵蓋資料擷取、標註、模型訓練、部署與 AIoT 通訊整合。

## 硬體連接示意 (ASCII)
```
+------------------------------+        +-----------------------+
| Raspberry Pi Pico 2 W        |        | Sensor / Peripheral   |
|                              | GP0  ->| RGB LED R (紅)        |
| USB 5V/3V3/GND --- 共用地線  | GP1  ->| RGB LED G (綠)        |
|                              | GP2  ->| RGB LED B (藍)        |
|                              | GP4  ->| BTN1 錄製按鈕         |
|                              | GP5  ->| BTN2 模式按鈕         |
|                              | GP9  ->| SD Card CS            |
|                              | GP10 ->| SD Card SCK           |
|                              | GP11 ->| SD Card MOSI          |
|                              | GP12 ->| SD Card MISO          |
|                              | GP14 ->| MPU6050 SDA (I2C1)    |
|                              | GP15 ->| MPU6050 SCL (I2C1)    |
+------------------------------+        +-----------------------+
```

## 目錄總覽

| 目錄 | 類型 | 用途 |
| ---- | ---- | ---- |
| [`CollectData/`](CollectData)     | 主流程 | 按鈕驅動、case0–case3 錄製 1 秒 IMU CSV 至 SD 卡（主要資料擷取入口） |
| [`CollectDataV2/`](CollectDataV2) | 主流程 | 與 V1 功能相同、命名風格簡化（`R/G/B`、`SIG_DATA`） |
| [`Inferencing/`](Inferencing)     | 主流程 | 載入 Edge Impulse 匯出的推論函式庫、序列埠輸出分類結果 |
| [`MQTTwithAI/`](MQTTwithAI)       | 主流程 | 推論 + Wi-Fi 配網 + MQTT 發佈三合一整合 |
| [`DHT11MQTT/`](DHT11MQTT)         | 延伸  | DHT11 溫濕度透過 MQTT 上傳（平行示範非 IMU 感測器） |
| [`BTNCode/`](BTNCode)             | 示範  | `Button2` 四種按鈕事件測試 |
| [`RGBLEDCode/`](RGBLEDCode)       | 示範  | RGB LED 基本點亮測試 |
| [`SDCardWrite/`](SDCardWrite)     | 示範  | SD 卡讀寫自測 |
| [`MPU6050Code/`](MPU6050Code)     | 示範  | MPU6050 1 kHz 讀值自測（可配序列埠繪圖家） |
| [`WifiConnector/`](WifiConnector) | 樣板  | Wi-Fi AP 配網骨架，供其他草稿抽用 |
| [`example/`](example)             | 入門  | 新學員第一份草稿：按鈕 + RGB LED 驗證接線 |
| [`Data/`](Data)                   | 資料  | 約 727 筆 CSV 訓練樣本（由 `CollectData` 產生） |
| [`PCB/`](PCB)                     | 硬體  | 教學板 Gerber、schematic PDF 與佈線圖 |
| [`docs/`](docs)                   | 文件  | 繁中／英文教學文件、投影片、Edge Impulse 延伸教材 |

每個目錄皆附 `README.md` 說明硬體腳位、相依函式庫與編譯指令。

## 主流程資料鏈路

```
example ── 確認接線 ──▶ CollectData ── 錄 CSV ──▶ Data/
                                                   │
                                                   ▼
                                          Edge Impulse 訓練
                                                   │
                                                   ▼
                                        匯出 Arduino library
                                                   │
                              ┌────────────────────┴────────────────────┐
                              ▼                                         ▼
                        Inferencing                               MQTTwithAI
                     （本地序列埠輸出）                          （遠端 MQTT 上傳）
```

更多硬體佈線、資料格式與 Edge Impulse 操作流程，請參閱 [docs/zh-TW](docs/zh-TW) 內的專題文件。

## 開發環境與關鍵指令
- 安裝 Arduino IDE 2.x 或 `arduino-cli`，使用開發板套件 `rp2040:rp2040:pico_w`。
- 編譯：`arduino-cli compile --fqbn rp2040:rp2040:pico_w CollectData/CollectData.ino`
- 上傳：`arduino-cli upload --fqbn rp2040:rp2040:pico_w --port <COMx> Inferencing/Inferencing.ino`
- 函式庫同步：`arduino-cli lib install Button2 PubSubClient ArduinoJson`
- Edge Impulse 函式庫：更新 `lib/EdgeAI_inferencing` 後需重新編譯相關草稿。

## 資料蒐集與模型部署流程
1. 依 [資料格式規範](docs/zh-TW/dataset-format.md) 建立 `case<id>.sample<index>.csv` 並確認時間戳連續。
2. 將資料匯入 Edge Impulse，參考 [Edge Impulse 整合教學](docs/zh-TW/edge-impulse-guide.md) 訓練模型。
3. 以「Arduino Library」部署並覆蓋專案中的 Edge Impulse 程式庫後重新燒錄。
4. 透過序列埠或 MQTT 驗證輸出，必要時重新標註或調整取樣流程。

## 文件索引
- 中文文件：[docs/zh-TW/](docs/zh-TW) — Wi-Fi 設定、擴充板配置、資料格式與 Edge Impulse 教學
- English Docs：[docs/en/](docs/en) — Wi-Fi guide, hardware layout, dataset format, Edge Impulse workflow

---

## English Version
[中文版本](#專案簡介)

### Project Overview
- Raspberry Pi Pico 2 W logs 1 kHz MPU6050 motion data to SD, producing Edge Impulse-ready CSV datasets.
- TinyML models trained in Edge Impulse run locally for gesture inference and publish results over Wi-Fi/MQTT.
- Covers the full classroom AIoT flow: sensing, labeling, training, deployment, and connectivity.

### Hardware Wiring (ASCII)
```
+------------------------------+        +-----------------------+
| Raspberry Pi Pico 2 W        |        | Sensor / Peripheral   |
|                              | GP0  ->| RGB LED R             |
| USB 5V/3V3/GND --- common GND| GP1  ->| RGB LED G             |
|                              | GP2  ->| RGB LED B             |
|                              | GP4  ->| BTN1 Record           |
|                              | GP5  ->| BTN2 Mode             |
|                              | GP9  ->| SD Card CS            |
|                              | GP10 ->| SD Card SCK           |
|                              | GP11 ->| SD Card MOSI          |
|                              | GP12 ->| SD Card MISO          |
|                              | GP14 ->| MPU6050 SDA (I2C1)    |
|                              | GP15 ->| MPU6050 SCL (I2C1)    |
+------------------------------+        +-----------------------+
```

### Repository Layout

| Directory | Type | Purpose |
| --------- | ---- | ------- |
| [`CollectData/`](CollectData)     | main    | Button-driven capture for `case0`–`case3`, writes `case<id>.sample<index>.csv` to SD |
| [`CollectDataV2/`](CollectDataV2) | main    | Same function as V1, simplified naming (`R/G/B`, `SIG_DATA`) |
| [`Inferencing/`](Inferencing)     | main    | Runs the exported Edge Impulse classifier, prints predictions over USB serial |
| [`MQTTwithAI/`](MQTTwithAI)       | main    | Inference + Wi-Fi provisioning + MQTT publishing in one sketch |
| [`DHT11MQTT/`](DHT11MQTT)         | extra   | DHT11 temperature/humidity via MQTT (parallel demo with non-IMU sensor) |
| [`BTNCode/`](BTNCode)             | demo    | `Button2` click / double / triple / long-press test |
| [`RGBLEDCode/`](RGBLEDCode)       | demo    | Minimal RGB LED blink |
| [`SDCardWrite/`](SDCardWrite)     | demo    | SD card read/write self-test |
| [`MPU6050Code/`](MPU6050Code)     | demo    | MPU6050 1 kHz read loop (Serial Plotter friendly) |
| [`WifiConnector/`](WifiConnector) | template | Wi-Fi AP provisioning skeleton for reuse |
| [`example/`](example)             | starter | First sketch for new students: button + RGB LED sanity check |
| [`Data/`](Data)                   | dataset | ~727 CSV samples produced by `CollectData` |
| [`PCB/`](PCB)                     | hardware | Teaching board Gerber, schematic PDF, layout images |
| [`docs/`](docs)                   | docs    | zh-TW / English guides, slide decks, Edge Impulse course archive |

Every directory ships a `README.md` describing pins, libraries and build commands.

### Main Pipeline

```
example ── verify wiring ──▶ CollectData ── write CSV ──▶ Data/
                                                          │
                                                          ▼
                                                Edge Impulse training
                                                          │
                                                          ▼
                                          export Arduino library
                                                          │
                                ┌─────────────────────────┴─────────────────────────┐
                                ▼                                                   ▼
                          Inferencing                                          MQTTwithAI
                      (USB serial output)                                (remote MQTT publish)
```

For detailed hardware wiring, Wi-Fi provisioning, and Edge Impulse instructions, explore the resources under [docs/en](docs/en).

### Toolchain & Commands
- Install Arduino IDE 2.x or `arduino-cli` with `rp2040:rp2040:pico_w` support.
- Build: `arduino-cli compile --fqbn rp2040:rp2040:pico_w CollectData/CollectData.ino`
- Flash: `arduino-cli upload --fqbn rp2040:rp2040:pico_w --port <COMx> Inferencing/Inferencing.ino`
- Libraries: `arduino-cli lib install Button2 PubSubClient ArduinoJson`
- Refresh the Edge Impulse library export before rebuilding data logging or MQTT sketches.

### Dataset & Model Loop
1. Follow the [dataset format guide](docs/en/dataset-format.md) for timestamp validation and labeling.
2. Train in Edge Impulse using the [workflow checklist](docs/en/edge-impulse-workflow.md).
3. Deploy via the Arduino library export and upload to the Pico 2 W.
4. Verify predictions through the serial console or configured MQTT topic, iterate as needed.

### Documentation Index
- Traditional Chinese resources: [docs/zh-TW/](docs/zh-TW)
- English resources: [docs/en/](docs/en)
