# Inferencing — Edge Impulse 裝置端推論

載入由 Edge Impulse 匯出的推論函式庫（`a20260414_inferencing.h`），以 1 kHz 取樣 MPU6050 三軸加速度，收滿 1 秒（3000 點）後呼叫分類器，於序列埠印出每個 case 的信心值。

## 硬體腳位

| 訊號 | GPIO |
| ---- | ---- |
| MPU6050 SDA / SCL | GP14 / GP15（I2C1） |

不需要 SD 卡、按鈕或 LED。

## 相依函式庫

- Edge Impulse 匯出的 Arduino library（本專案檔名為 `a20260414_inferencing`）。更新模型時須先在 Arduino 函式庫管理員移除舊版，再將新匯出的 zip 加入。
- 內建 `Wire`。

## 編譯與上傳

```bash
arduino-cli compile --fqbn rp2040:rp2040:pico_w Inferencing/Inferencing.ino
arduino-cli upload  --fqbn rp2040:rp2040:pico_w --port <COMx> Inferencing/Inferencing.ino
```

## 執行流程

1. 開機初始化 Wire1、確認 MPU6050 位址回應 (`0x68`)。
2. 以 `SAMPLE_INTERVAL_MS = 1` 讀取加速度並填入 `features[]`，總長度 `EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE = 3000`。
3. 呼叫 `run_classifier()`，列印每個分類標籤的機率與異常分數（若模型含 anomaly detection）。
4. 清空緩衝，進入下一輪。

## 模型換版步驟

1. 在 Edge Impulse 儀表板 → Deployment → Arduino library，下載 zip。
2. Arduino IDE：草稿 → 匯入函式庫 → 加入 .ZIP 函式庫。
3. 修改本檔第 5 行 `#include <新檔名_inferencing.h>` 對齊新匯出的命名。
4. 重新編譯、上傳。

詳細流程見 [docs/zh-TW/edge-impulse-guide.md](../docs/zh-TW/edge-impulse-guide.md)。

## 相關模組

- 訓練資料由 `CollectData/` 產生。
- 若需將推論結果透過 MQTT 上傳，改用 [`MQTTwithAI/`](../MQTTwithAI/)。
