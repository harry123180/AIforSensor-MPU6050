# MPU6050Code — MPU6050 加速度計自測

以 I2C1 讀取 MPU6050 三軸加速度，1 kHz 取樣並以 CSV 格式印到序列埠，可直接餵入 Arduino IDE「序列埠繪圖家」觀察訊號。**非主流程使用**，確認感測器接線正確時跑此草稿最直接。

## 硬體

| 訊號 | GPIO | 備註 |
| ---- | ---- | ---- |
| SDA  | GP14 | I2C1 資料線 |
| SCL  | GP15 | I2C1 時脈線 |

- MPU6050 I2C 位址：`0x68`
- I2C 時脈：400 kHz
- 量程：加速度 ±2 g、陀螺儀 ±250 °/s
- 取樣率：1 kHz（SMPLRT_DIV = 0、DLPF off）

## 相依函式庫

內建 `Wire`，無需額外安裝。

## 編譯與上傳

```bash
arduino-cli compile --fqbn rp2040:rp2040:pico_w MPU6050Code/MPU6050Code.ino
arduino-cli upload  --fqbn rp2040:rp2040:pico_w --port <COMx> MPU6050Code/MPU6050Code.ino
```

## 輸出格式（115200 baud）

每毫秒一列，單位為 g：

```
AccelX,AccelY,AccelZ
-0.032,0.015,0.998
-0.031,0.016,0.997
...
```

## 相關模組

- `CollectData/` 與 `CollectDataV2/` 以同一組 I2C 腳位將資料批次寫入 SD 卡並以 1 秒為一筆樣本。
- `Inferencing/` 與 `MQTTwithAI/` 於推論階段重用相同的 MPU6050 初始化流程。
