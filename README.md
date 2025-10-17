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

## 韌體模組與資料流程
- `CollectData/CollectData.ino`：按鈕選擇 `case0~case3`，錄製 1 秒 IMU CSV 至 `caseX.sampleY.csv`。
- `Inferencing/Inferencing.ino`：載入 Edge Impulse 匯出的推論函式並於序列埠顯示分類結果。
- `MQTTwithAI/MQTTwithAI.ino`：提供 Wi-Fi 熱點設定、MQTT 發布與即時推論上傳。

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

### Firmware Modules & Flow
- `CollectData/CollectData.ino`: button-driven capture for `case0`–`case3`, writes `case<id>.sample<index>.csv` batches.
- `Inferencing/Inferencing.ino`: runs the exported Edge Impulse classifier and prints live predictions via USB serial.
- `MQTTwithAI/MQTTwithAI.ino`: offers Wi-Fi provisioning, MQTT publishing, and on-device inference streaming.

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
