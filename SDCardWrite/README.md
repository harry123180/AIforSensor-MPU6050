# SDCardWrite — SD 卡讀寫自測

以 SPI1 介面驗證 SD 卡讀寫：建立 `hello.txt` 寫入一行字串，接著讀回並印到序列埠。**非主流程使用**，僅為教學範例，排查 SD 卡故障時可先跑此草稿。

## 硬體

| 訊號 | GPIO |
| ---- | ---- |
| CS   | GP9  |
| SCK  | GP10 |
| MOSI | GP11 |
| MISO | GP12 |

使用 `SPI1`；SD 卡需格式化為 FAT16/FAT32。

## 相依函式庫

內建 `SPI`、`SD`，無需額外安裝。

## 編譯與上傳

```bash
arduino-cli compile --fqbn rp2040:rp2040:pico_w SDCardWrite/SDCardWrite.ino
arduino-cli upload  --fqbn rp2040:rp2040:pico_w --port <COMx> SDCardWrite/SDCardWrite.ino
```

## 預期輸出

```
初始化SD卡...
SD卡初始化成功
檔案寫入完成
檔案內容:
hello world
```

## 相關模組

- `CollectData/` 與 `CollectDataV2/` 沿用同一組 SPI 腳位將 IMU CSV 寫入 SD 卡。
