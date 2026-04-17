# BTNCode — Button2 多事件示範

示範 `Button2` 函式庫的四種按鈕事件（短按、雙擊、三擊、長按），供課堂確認按鈕接線與事件回呼機制。**非主流程使用**，僅為教學範例。

## 硬體

| 訊號 | GPIO | 備註 |
| ---- | ---- | ---- |
| BTN1 | GP4  | 按鈕 1 |
| BTN2 | GP5  | 按鈕 2 |

按鈕另一端接 GND；`Button2` 預設啟用內部上拉。

## 相依函式庫

```bash
arduino-cli lib install Button2
```

## 編譯與上傳

```bash
arduino-cli compile --fqbn rp2040:rp2040:pico_w BTNCode/BTNCode.ino
arduino-cli upload  --fqbn rp2040:rp2040:pico_w --port <COMx> BTNCode/BTNCode.ino
```

## 預期輸出（115200 baud）

```
Button2 Multi-Function Example
================================
Ready! Press buttons to test:
- Single click
- Double click
- Triple click
- Long press (hold > 1 second)
================================
BTN1: Single Click
BTN2: Long Press Detected
...
```

## 相關模組

- `CollectData/` 與 `CollectDataV2/` 皆沿用相同按鈕接線並擴充為錄製／切換邏輯。
