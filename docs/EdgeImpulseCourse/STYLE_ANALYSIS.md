# EdgeImpulseCourse 整體 PPT 風格分析報告

> 由 4 個並行 subagent 分析 4 個 Module 共 **42 個 .pptx 檔、897 張投影片**，彙整出共通風格與差異點。

## 一、規模總覽

| 模組 | .pptx 數 | 投影片總數 | 媒體檔數 |
|---|---|---|---|
| M1 Introduction to ML | 9 | 173 | 149 |
| M2 Getting Started with Deep Learning | 12 | 301 | ~230 |
| M3 Machine Learning Workflow | 12 | 219 | ~196 |
| M4 Model Deployment | 9 | 204 | 95 |
| **合計** | **42** | **897** | **~670** |

## 二、共通風格特徵（4 個模組高度一致）

### 1. 版面規格
- **一律 16:9 寬螢幕**，無 4:3
- 兩種尺寸並存：
  - **10 × 5.625 in**（標準 Google Slides 匯出，多數檔案）
  - **20 × 11.25 in**（放大 2 倍，部分 Google Slides 大畫布匯出檔）
- 兩種尺寸投影片擺一起，placeholder 仍能對齊 16:9，但縮放比例不同

### 2. 字體系統
- **theme1.xml 預設**：Arial（major + minor 皆為 Arial）
- **實際 run 覆寫**：
  - 標題：**Google Sans Medium**（粗體）
  - 內文：**Google Sans / Roboto**
  - 程式碼：**Roboto Mono**
- **完全沒有中文（東亞）字體設定** — 全英文教材
- 字級：封面 48-60pt / 內文標題 27-47pt / 內文 17-32pt / 標籤 8-18pt

### 3. 配色系統（兩套並存）

**A. Google Material 色盤**（M1/M2/M3 TinyMLx 系列、M4 部分）

| 用途 | Hex |
|---|---|
| 主文字 | `#3C4043`（深灰） |
| 次文字 | `#5F6368` |
| 線框灰 | `#BDC1C6` |
| 底色 | `#F8F9FA`（近白）/ `#FFFFFF` |
| 強調 Google 四色 | `#4285F4`（藍）`#EA4335`（紅）`#FBBC05`（黃）`#34A853`（綠） |
| 深色變體 | `#185ABC`、`#1A73E8`、`#B31412`、`#7B1FA2` |

**B. Edge Impulse 色盤**（M3/M4 Edge Impulse 系列）

| 用途 | Hex |
|---|---|
| 主色（橘） | `#FFAB40` |
| 次色（青） | `#0097A7` |
| 藍灰 | `#78909C` |
| 重點黑 | `#212121` |
| 檸檬黃 | `#EEFF41` |

**C. Highcharts 圖表預設色盤**（圖表用，散見 M2/M3/M4）  
`#058DC7` `#50B432` `#ED561B` `#EDEF00` `#24CBE5` `#64E572`

底色一律白色（master 寫死 `solidFill #FFFFFF`），**沒有漸層、背景圖樣、裝飾色塊**。

### 4. 版面結構

| 頁面類型 | 佈局 | 特徵 |
|---|---|---|
| **封面** | `CUSTOM_2_1_1` / `TITLE` | 大標靠左偏上，副標下方，講者署名（Laurence Moroney, Google）置中底部，純白底 |
| **章節分隔** | `CUSTOM_2_1_1_1` | 章節碼 + 大標 + 副標三段式 |
| **內容頁** | `TITLE_2` / `TITLE_2_2_1` / `TITLE_2_3_3` | **標題一律靠左頂端** + 下方圖文 2 欄或 3 區塊 |
| **程式碼頁** | `BLANK` + dark theme | 深底（近黑）+ 彩色語法高亮，與白底頁反差大 |
| **結尾頁** | `Fullscreen. Show presenter.` 全版 placeholder | Google Slides 簡報員操作提示，切到全螢幕示範 |
| **章節提問** | `CUSTOM` | 大字置中問句（rhetorical question） |

### 5. 圖像與視覺元素
- **PNG 為主**（約 78%）：去背 icon、向量插畫、UI 截圖
- **JPG 較少**：硬體實拍（晶片、感測器、PCB）
- **GIF**：少量動態示意（模型運作）
- **無 SVG / EMF 向量**：所有圖檔皆為點陣
- **大量原生 shape 拼圖**：流程圖、神經網路、混淆矩陣用 PowerPoint AutoShape + TextBox 手繪
  - 極端例：M3 `3.3.4` 第 7 頁有 **968 個 shapes** 拼成神經網路圖
- **平均每頁圖片數**：0.5-1 張，視內容主題差異大
- **無頁碼、無頁尾、無 logo 浮水印**（Edge Impulse 系列例外，底部有 `© 2021 EdgeImpulse, Inc.`）

### 6. 動畫與切換
- **完全沒有動畫**（無 `<p:transition>` 設定）
- 純靜態講授風格，依賴講者口頭解說

### 7. 起源指紋
- 所有檔案中圖片皆為 `Google Shape;XXX;pYY` 命名 → **確認原檔由 Google Slides 匯出**
- 講者一致為 **Laurence Moroney, Google**（M2 主講）+ Harvard TinyMLx 課程素材
- Edge Impulse 部分檔案來自 Edge Impulse 公司教材

## 三、兩套視覺世代並存（重要差異點）

| 屬性 | TinyMLx / Google 系列 | Edge Impulse 系列 |
|---|---|---|
| 投影片尺寸 | 10×5.625 in | 20×11.25 in（多數） |
| 主色 | Google 四色 | 橘 + 青 |
| 字體 | Google Sans + Roboto | Google Sans + Arial |
| 圖像風格 | Google Material icon + 概念圖 | 平台 UI 截圖 + 硬體實拍 |
| 封面 | 三行純字、無圖 | 多張產品截圖 + 流程圖 |
| 版權頁尾 | 無 | `© 2021 EdgeImpulse, Inc.` |
| 結尾頁 | `Fullscreen. Show presenter.` 提示 | `Summary` 或留白 |

## 四、本土化 / 二次製作建議

### A. 統一規格
1. **統一投影片尺寸**：建議改為 **13.33 × 7.5 in**（PowerPoint 標準 16:9）或全部統一為 10×5.625 in
2. **統一配色**：擇一 — Google Material 風（藍紅黃綠）或 Edge Impulse 風（橘青）
3. **移除 Google Slides 殘留**：
   - 結尾的 `Fullscreen. Show presenter.` placeholder（簡報員操作提示，非教材內容）
   - 段落間殘留的 `|` 符號

### B. 中文化必要設定
- 所有 .pptx 都沒有 `<a:ea>` 東亞字體宣告
- 二次製作時須在 slideMaster + layouts 補上：
  ```xml
  <a:ea typeface="Noto Sans TC"/>  <!-- 或 思源黑體 TC -->
  ```
- 標題對應：Google Sans Bold → **Noto Sans TC Bold** / **思源黑體 TC Bold**
- 內文對應：Google Sans / Roboto → **Noto Sans TC Regular / Medium**
- 程式碼：Roboto Mono → **Fira Code** / **JetBrains Mono**

### C. 版權清理
- 移除 Edge Impulse 系列底部的 `© 2021 EdgeImpulse, Inc.` 標示（避免侵權）
- M3 `3.3.4` 第 7 頁的 968-shape 神經網路圖，建議改用 SVG 或 Mermaid 重畫
- 若教學使用：原作者署名（Laurence Moroney, Google）建議保留作為「參考來源」

### D. 設計取捨建議
| 要保留 | 要捨棄 |
|---|---|
| 16:9 寬螢幕 | 20×11.25 in 大畫布 |
| Google Material 配色（更現代） | Highcharts 預設色（過時感） |
| 標題靠左頂端 | 多版本佈局並存 |
| 純白底、留白多 | 多色花俏背景 |
| Google Sans 風字體 | Calibri、Tahoma、Trebuchet MS 混用 |
| PNG + shape 並用 | 968-shape 大型手繪圖 |

## 五、最終風格指紋（一句話）

> **「16:9 純白底、Google Sans + Google Material 四色、標題靠左頂端、PNG icon + 大量自繪 shape、無動畫、無頁碼、由 Google Slides 匯出。」**

這套風格適合：技術教學、線上課程、概念說明、業界研討會  
不適合：行銷簡報、年度報告、創意提案（過於樸實）

---
*分析時間：2026-05-17*  
*工具：Python python-pptx + 4 個 Claude 並行 subagent*
