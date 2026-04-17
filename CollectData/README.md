# CollectData — 主要資料擷取流程

按鈕驅動、分類別（case0–case3）錄製 1 秒 IMU 樣本（1000 Hz × 3 軸）並以 CSV 寫入 SD 卡。**本目錄為專案主要資料擷取入口**，`Data/` 內的 727 筆 CSV 即由本草稿產生。

## 硬體腳位

| 訊號 | GPIO |
| ---- | ---- |
| LED R | GP0 |
| LED G | GP1 |
| LED B | GP2 |
| BTN1（錄製） | GP4 |
| BTN2（切換 case） | GP5 |
| SD CS / SCK / MOSI / MISO | GP9 / GP10 / GP11 / GP12 |
| MPU6050 SDA / SCL | GP14 / GP15 |

## 操作流程

1. **設定狀態（STATE_SETUP）**：開機後 RGB 閃爍兩次 → 進入待機。
2. **切換 case**：短按 BTN2 於 `case0` → `case1` → `case2` → `case3` 循環，LED 以不同顏色代表目前 case。
3. **錄製樣本**：短按 BTN1 開始錄製 1 秒（1000 筆樣本），LED 亮起綠色；錄製完成自動寫入 SD 卡 `case<id>.sample<n>.csv`。
4. **連續錄製**：長按 BTN1 進入連續錄製；再次長按停止。
5. **刪除**：長按 BTN2 觸發 5 秒刪除倒數（LED 紅色閃爍），期間任一按鈕可取消；倒數結束後清空對應 case 的所有樣本。

## 檔案命名

```
case<id>.sample<index>.csv      例：case0.sample42.csv
```

每筆 CSV 為 1000 列、3 欄（AccX、AccY、AccZ，單位 g）。格式規範見 [docs/zh-TW/dataset-format.md](../docs/zh-TW/dataset-format.md)。

## 相依函式庫

```bash
arduino-cli lib install Button2
```

`Wire`、`SPI`、`SD` 皆為內建。

## 編譯與上傳

```bash
arduino-cli compile --fqbn rp2040:rp2040:pico_w CollectData/CollectData.ino
arduino-cli upload  --fqbn rp2040:rp2040:pico_w --port <COMx> CollectData/CollectData.ino
```

## 與 CollectDataV2 的差異

功能完全相同；V2 改用較短的腳位巨集（`R`／`G`／`B`）與資料緩衝名稱（`SIG_DATA`）以配合後續教學簡寫。新學員請優先使用本版。
