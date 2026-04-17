# example — 入門起手式

最小可編譯範例：短按 BTN1 觸發「紅 2 秒 → 綠 2 秒 → 藍 2 秒」依序點亮。適合當作新學員接上硬體後的第一份草稿，用來同時驗證按鈕與 RGB LED 是否正確接線。

## 硬體腳位

| 訊號 | GPIO |
| ---- | ---- |
| BTN1 | GP4 |
| LED R | GP0 |
| LED G | GP1 |
| LED B | GP2 |

與 `CollectData/` 使用相同的接線，後續直接切換草稿即可進入資料擷取流程。

## 相依函式庫

```bash
arduino-cli lib install Button2
```

## 編譯與上傳

```bash
arduino-cli compile --fqbn rp2040:rp2040:pico_w example/example.ino
arduino-cli upload  --fqbn rp2040:rp2040:pico_w --port <COMx> example/example.ino
```

## 預期行為

開機序列埠（115200 baud）顯示 `System Ready. Press BTN1 to start.` 後，按一次 BTN1 即依序點亮 R → G → B 各 2 秒。
