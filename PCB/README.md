# PCB — 教學板硬體設計檔

本目錄提供 AI 培育計畫教學板的量產檔與線路圖。

## 內容

| 檔案 | 說明 |
| ---- | ---- |
| `AI_project_Gerber.zip`        | PCB 量產用 Gerber 檔（可直接送 JLCPCB／PCBWay 打樣） |
| `AI培育計畫線路原理圖.pdf`     | 完整 schematic，含 Pico 2 W、MPU6050、RGB LED、按鈕、SD 卡座接線 |
| `PCB_Front_Layout.jpg`         | 正面佈線預覽圖 |
| `PCB_Back_Layout.jpg`          | 背面佈線預覽圖 |

## 對應韌體

電路板的腳位分配與根目錄 [`README.md`](../README.md) 的「硬體連接示意」完全相同：GP0–GP2 RGB、GP4/GP5 按鈕、GP9–GP12 SD 卡、GP14/GP15 MPU6050。

## 打樣建議

- 板材：FR-4，厚度 1.6 mm。
- 最小線寬／孔距：0.2 mm / 0.3 mm。
- 表面處理：HASL（鉛）或 ENIG；教學用 HASL 即可。

如需修改版型，建議以 schematic PDF 為準重新在 KiCad／EasyEDA 建檔；本目錄未保留原始 .kicad_pro 專案。
