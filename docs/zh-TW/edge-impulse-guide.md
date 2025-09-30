﻿# Edge Impulse + Raspberry Pi Pico 2 W 整合教學

## 目錄
1. [建立 Edge Impulse 專案](#建立-edge-impulse-專案)
2. [準備與上傳資料](#準備與上傳資料)
3. [設計 Impulse 流程](#設計-impulse-流程)
4. [訓練與驗證模型](#訓練與驗證模型)
5. [部署為 Arduino 函式庫](#部署為-arduino-函式庫)
6. [韌體整合重點](#韌體整合重點)
7. [常見問題與排錯](#常見問題與排錯)
8. [版本資訊](#版本資訊)

---

## 建立 Edge Impulse 專案
1. 登入 [Edge Impulse Studio](https://studio.edgeimpulse.com)。
2. 點選 **Create new project**，輸入專案名稱，開發類型建議使用 **Developer**。
3. 在 **Dashboard → Project info** 設定資料來源：
   - **Sensor**：Accelerometer。
   - **Frequency**：`1000 Hz`（與韌體每秒蒐集 1000 筆資料一致）。
   - **Axes**：3（X、Y、Z）。

---

## 準備與上傳資料
1. 使用 `CollectData/CollectData.ino` 錄製資料，檔案格式為 `case<id>.sample<index>.csv`（例如 `case1.sample3.csv`）。
2. 確認 CSV 內容格式：
   ```csv
   time_ms,acc_x_g,acc_y_g,acc_z_g
   0,0.012,-0.034,0.998
   1,0.015,-0.032,0.999
   ```
3. 導入 Studio：
   - 前往 **Data acquisition → Upload data**。
   - 選擇檔案後設定 **Label**（如 `case1`）、**Sample rate** (`1000 Hz`)、**Sensor** (`Accelerometer`)。
4. 在 **Dashboard → Manage data** 中分割資料集，建議 **Training 80% / Testing 20%**。

---

## 設計 Impulse 流程
1. 進入 **Create impulse**，設定：
   - **Window size**：`1000 ms`。
   - **Window increase**：`1000 ms`（滑動視窗與資料視窗相同，避免重疊）。
   - **Frequency**：`1000 Hz`。
2. 新增處理模組 **Spectral Analysis**，輸入軸體為 `acc_x_g, acc_y_g, acc_z_g`。
3. 新增學習模組 **Classification (Keras)**。
4. 儲存並生成特徵：
   - 在 **Spectral features** 頁面選擇 **Generate features**。
   - 確認不同案例的特徵投影分布明顯分群。

---

## 訓練與驗證模型
1. 開啟 **Classifier** 頁面，建議設定：
   - **Number of training cycles**：`100`。
   - **Learning rate**：`0.005`。
   - **Validation set size**：`20%`。
2. 點選 **Start training**，等待訓練完成。
3. 在結果頁面檢視：
   - **Accuracy**：建議高於 85%。
   - **Confusion matrix**：檢查各案例是否準確分類。
   - **Model testing**：使用測試資料確認實際效能。

---

## 部署為 Arduino 函式庫
1. 前往 **Deployment** 頁面，選擇 **Arduino library**。
2. 建議勾選：
   - **Quantized (int8)**：減少模型佔用的 RAM/Flash。
   - **Advanced options → EON Compiler**：需要時啟用，以獲得更小的模型。
3. 下載 `edge-impulse-<project-name>-arduino.zip`。
4. 將解壓後的 `EdgeAI_inferencing` 目錄複製到專案的 `lib/` 或 `MQTTwithAI/lib/` 中，覆蓋舊版本。

---

## 韌體整合重點
- **資料擷取**：`CollectData.ino` 使用 `imuSamples[3][1000]` 緩衝區儲存 1 秒的資料，請確保緩衝寫入與清除邏輯與模型視窗一致。
- **即時推論**：`Inferencing.ino` 與 `MQTTwithAI.ino` 透過 `run_classifier()` 執行分類，並於序列埠或 MQTT 發送結果。
- **Edge Impulse SDK 設定**：若模型窗口數與預設不同，需更新：
  ```cpp
  #define EI_CLASSIFIER_INTERVAL_MS 1
  #define EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW 1
  ```
- **MPU6050 初始化**：確保 I2C 仍設定為 `SDA = GP14`、`SCL = GP15`，量測範圍維持 ±2g，取樣頻率 1 kHz。

---

## 常見問題與排錯
| 問題 | 可能原因 | 建議解法 |
|------|----------|----------|
| `ERR: Unknown extract function` | 介面類型選錯（如將加速度資料當音訊） | 在 `Dashboard` 再次確認 `Sensor` 為 Accelerometer，Impulse 使用 Spectral Analysis。 |
| 訓練後混淆矩陣偏離 | 標籤不平衡或資料品質差 | 補充每個案例的資料量，確保時間戳連續且沒有 NaN。 |
| Serial 顯示 RAM 不足 | 模型過大或未量化 | 啟用 Quantized (int8) 與 EON Compiler，或減少 Dense Layer 神經元數量。 |
| 推論延遲過高 | MCU 需處理 Wi-Fi/MQTT 與 AI | 減少傳輸頻率，或改採批次推論後統整結果。 |

---

## 版本資訊
- **撰寫日期**：2025-09-30
- **適用設備**：Raspberry Pi Pico 2 W (RP2350)
- **對應韌體**：`CollectData/CollectData.ino`、`Inferencing/Inferencing.ino`、`MQTTwithAI/MQTTwithAI.ino`

如流程更新，請同步修訂本指南與 README 中的相關連結與說明。
