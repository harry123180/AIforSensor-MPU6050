# Raspberry Pi Pico 2 W IMU AIoT ?飛撠?
[English Version](#english-version)

## 撠?蝪∩?
- 雿輻 Raspberry Pi Pico 2 W ??MPU6050 銝遘 IMU嚗誑 1 kHz ?見?漲????鞈?銝虫?摮 SD ?～?
- ?? Edge Impulse 閮毀 TinyML 璅∪?嚗鋆蔭蝡臬?隢蒂?? Wi-Fi/MQTT 銝蝯???
- ?飛蝷箇?瘨菔?鞈??瑕???閮颯芋??蝺氬蝵脰? AIoT ???游???

## 蝖祇???蝷箸? (ASCII)
```
+------------------------------+        +-----------------------+
| Raspberry Pi Pico 2 W        |        | Sensor / Peripheral   |
|                              | GP0  ->| RGB LED R (蝝?        |
| USB 5V/3V3/GND --- ?梁?啁?  | GP1  ->| RGB LED G (蝬?        |
|                              | GP2  ->| RGB LED B (??        |
|                              | GP4  ->| BTN1 ?ˊ??         |
|                              | GP5  ->| BTN2 璅∪???         |
|                              | GP9  ->| SD Card CS            |
|                              | GP10 ->| SD Card SCK           |
|                              | GP11 ->| SD Card MOSI          |
|                              | GP12 ->| SD Card MISO          |
|                              | GP14 ->| MPU6050 SDA (I2C1)    |
|                              | GP15 ->| MPU6050 SCL (I2C1)    |
+------------------------------+        +-----------------------+
```

## ??璅∠?????蝔?
- `CollectData/CollectData.ino`嚗????`case0~case3`嚗?鋆?1 蝘?IMU CSV ??`caseX.sampleY.csv`??
- `Inferencing/Inferencing.ino`嚗???Edge Impulse ?臬?隢撘蒂?澆???憿舐內??蝯???
- `MQTTwithAI/MQTTwithAI.ino`嚗?靘?Wi-Fi ?梢?閮剖??QTT ?澆???隢??喋?

## ??啣????菜?隞?
- 摰? Arduino IDE 2.x ??`arduino-cli`嚗蝙?券??潭憟辣 `rp2040:rp2040:pico_w`??
- 蝺刻陌嚗arduino-cli compile --fqbn rp2040:rp2040:pico_w CollectData/CollectData.ino`
- 銝嚗arduino-cli upload --fqbn rp2040:rp2040:pico_w --port <COMx> Inferencing/Inferencing.ino`
- ?賢?摨怠?甇伐?`arduino-cli lib install Button2 PubSubClient ArduinoJson`
- Edge Impulse ?賢?摨怨??湔 `lib/EdgeAI_inferencing` 敺??啁楊霅舐??蝔踴?

## 鞈????芋?蝵脫?蝔?
1. 靘?`docs/zh-TW/dataset-format.md` 撱箇? `case<id>.sample<index>.csv` 銝衣Ⅱ隤???????
2. 撠????Edge Impulse嚗???`docs/zh-TW/edge-impulse-guide.md` 閮毀璅∪???
3. 隞乓rduino Library?蝵脖蒂閬?撠?銝剔? Edge Impulse 蝔?摨怠??????
4. ??摨??? MQTT 撽?頛詨嚗?閬??璅酉?矽?游?璅??蝔?

## ?辣蝝Ｗ?
- 銝剜??辣嚗docs/zh-TW/`嚗i-Fi 閮剖????蔭???撘dge Impulse ?飛嚗?
- English Docs嚗docs/en/`嚗i-Fi guide, hardware layout, dataset format, Edge Impulse workflow嚗?

---

## English Version

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
- `CollectData/CollectData.ino`: button-driven capture for `case0`?case3`, writes `case<id>.sample<index>.csv` batches.
- `Inferencing/Inferencing.ino`: runs the exported Edge Impulse classifier and prints live predictions via USB serial.
- `MQTTwithAI/MQTTwithAI.ino`: offers Wi-Fi provisioning, MQTT publishing, and on-device inference streaming.

### Toolchain & Commands
- Install Arduino IDE 2.x or `arduino-cli` with `rp2040:rp2040:pico_w` support.
- Build: `arduino-cli compile --fqbn rp2040:rp2040:pico_w CollectData/CollectData.ino`
- Flash: `arduino-cli upload --fqbn rp2040:rp2040:pico_w --port <COMx> Inferencing/Inferencing.ino`
- Libraries: `arduino-cli lib install Button2 PubSubClient ArduinoJson`
- Refresh the Edge Impulse library export before rebuilding data logging or MQTT sketches.

### Dataset & Model Loop
1. Follow `docs/en/dataset-format.md` for timestamp validation and labeling.
2. Train in Edge Impulse using `docs/en/edge-impulse-workflow.md` as a checklist.
3. Deploy via the Arduino library export and upload to the Pico 2 W.
4. Verify predictions through the serial console or configured MQTT topic, iterate as needed.

### Documentation Index
- Traditional Chinese resources: `docs/zh-TW/`
- English resources: `docs/en/`




