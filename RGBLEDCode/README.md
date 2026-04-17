# RGBLEDCode — RGB LED 基本點亮

最小化 RGB LED 範例：依序點亮 R → G → B，每段 100 ms，供課堂確認 LED 接線正確。**非主流程使用**，僅為教學範例。

## 硬體

| 訊號 | GPIO |
| ---- | ---- |
| R    | GP0  |
| G    | GP1  |
| B    | GP2  |

共陰極接地；本範例以 `digitalWrite(HIGH)` 點亮，若使用共陽極模組需反向邏輯。

## 相依函式庫

無（僅使用 Arduino core）。

## 編譯與上傳

```bash
arduino-cli compile --fqbn rp2040:rp2040:pico_w RGBLEDCode/RGBLEDCode.ino
arduino-cli upload  --fqbn rp2040:rp2040:pico_w --port <COMx> RGBLEDCode/RGBLEDCode.ino
```

## 命名說明

本檔案以 `R`、`G`、`B` 作為腳位巨集，與 `CollectDataV2/` 一致；主流程 `CollectData/` 則使用 `LED_PIN_R / G / B`。兩種風格功能相同，可自由引用。
