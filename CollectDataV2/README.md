# CollectDataV2 — 資料擷取（簡寫版）

與 [`CollectData/`](../CollectData/) 功能完全一致：按鈕驅動、case0–case3 錄製 1 秒 IMU 樣本並寫入 SD 卡。差異僅在於命名風格：

| 項目 | CollectData | CollectDataV2 |
| ---- | ----------- | ------------- |
| LED 腳位巨集 | `LED_PIN_R / G / B` | `R / G / B` |
| 資料緩衝名稱 | `imuSamples[3][1000]` | `SIG_DATA[3][1000]` |
| 註解語言 | 中英雙語 | 繁體中文為主 |

## 何時選用

- 需要完整英文註解、沿用官方命名：用 `CollectData/`。
- 課程講義或簡報對齊 `R / G / B`、`SIG_DATA` 等縮寫：用 `CollectDataV2/`。

兩者硬體接線、按鈕行為、檔名規則皆相同，產生的 CSV 可混用，請勿在同一裝置上並行維護兩者。

## 編譯與上傳

```bash
arduino-cli compile --fqbn rp2040:rp2040:pico_w CollectDataV2/CollectDataV2.ino
arduino-cli upload  --fqbn rp2040:rp2040:pico_w --port <COMx> CollectDataV2/CollectDataV2.ino
```

完整操作流程與硬體接線請見 [`CollectData/README.md`](../CollectData/README.md)。
