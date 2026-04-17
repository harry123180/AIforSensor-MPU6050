# Data — IMU 訓練資料集

由 `CollectData/CollectData.ino` 於課堂收集的樣本資料，匯入 Edge Impulse 後即可訓練動作分類模型。

## 檔名規則

```
case<id>.sample<index>.csv
例：case0.sample1.csv、case3.sample120.csv
```

- `case<id>`：動作類別（目前使用 0 ~ 3，共 4 類）。
- `sample<index>`：該類別內的第 N 筆樣本。

## 內容格式

每檔為 **1000 列 × 3 欄**，欄位順序為 `AccX,AccY,AccZ`，單位 g（依 MPU6050 ±2 g 量程、1 kHz 取樣）。對應 1 秒時間窗。

詳細欄位規範、時間戳與欄位檢查清單請見 [../docs/zh-TW/dataset-format.md](../docs/zh-TW/dataset-format.md)。

## 目前統計

- 總檔案數：約 727 個 CSV
- 類別：case0、case1、case2、case3

## 匯入 Edge Impulse

1. Edge Impulse Studio → **Data acquisition** → **Upload existing data**。
2. 選擇本目錄，label 欄填 `case0 / case1 / case2 / case3` 對應的值，上傳模式選 `CSV with header`。
3. 上傳後於 **Impulse design** 依 [edge-impulse-guide.md](../docs/zh-TW/edge-impulse-guide.md) 繼續訓練流程。

## 檔案時間戳

檔案系統時間戳為裝置未校時狀態下所寫入（可能顯示為 2098 或其他非現實年份），分析時請以檔名索引順序而非 mtime。
