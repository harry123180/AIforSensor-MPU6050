# docs — 專案文件索引

本目錄彙整全部教學文件。建議依學員學習進度依序閱讀。

## 教學投影片（PDF）

依課堂順序：

1. [1.振動訊號擷取技術.pdf](1.振動訊號擷取技術.pdf) — IMU／振動感測基礎、取樣率與解析度考量。
2. [2.AI模型基礎與應用.pdf](<2.AI模型基礎與應用 .pdf>) — TinyML／神經網路概論、Edge Impulse 工作流程概覽。
3. [3.系統整合與部屬準備.pdf](<3.系統整合與部屬準備 .pdf>) — 裝置端部署、Wi-Fi／MQTT 整合、量產前檢查清單。

## 繁體中文技術指南

位於 [`zh-TW/`](zh-TW)：

| 檔案 | 內容 |
| ---- | ---- |
| [hardware-layout.md](zh-TW/hardware-layout.md) | 硬體接線與 PCB 腳位說明 |
| [dataset-format.md](zh-TW/dataset-format.md)   | CSV 資料集命名與欄位規範 |
| [edge-impulse-guide.md](zh-TW/edge-impulse-guide.md) | Edge Impulse 完整操作教學（資料匯入 → 訓練 → 部署） |
| [pico2w-wifi-guide.md](zh-TW/pico2w-wifi-guide.md) | Pico 2 W Wi-Fi 配網與 MQTT 主題說明 |

## English Technical Guides

Located under [`en/`](en):

| File | Purpose |
| ---- | ------- |
| [hardware-layout.md](en/hardware-layout.md) | Hardware wiring and PCB pinout |
| [dataset-format.md](en/dataset-format.md)   | Dataset naming & field schema |
| [edge-impulse-workflow.md](en/edge-impulse-workflow.md) | End-to-end Edge Impulse workflow |
| [pico2w-wifi-guide.md](en/pico2w-wifi-guide.md) | Wi-Fi provisioning and MQTT topics |

## 延伸課程素材

[`EdgeImpulseCourse/`](EdgeImpulseCourse) 收錄 Edge Impulse 官方四大模組教材，對應進階自學：

- **Module 1** — Introduction to Machine Learning
- **Module 2** — Getting Started with Deep Learning
- **Module 3** — Machine Learning Workflow
- **Module 4** — Model Deployment

此資料夾內含投影片、學員講義與實作練習，不在主教學流程內，作為延伸閱讀。

## 閱讀順序建議

1. 根目錄 [`README.md`](../README.md)：快速掌握整體架構。
2. `hardware-layout.md`：確認接線。
3. 各模組目錄下的 `README.md`：逐一理解每份 `.ino` 的角色（BTNCode → MPU6050Code → SDCardWrite → CollectData）。
4. `dataset-format.md` + `edge-impulse-guide.md`：從資料到模型。
5. `pico2w-wifi-guide.md` + `MQTTwithAI/`：打通 AIoT 最後一哩。
