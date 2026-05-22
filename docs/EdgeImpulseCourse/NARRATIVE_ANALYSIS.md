# EdgeImpulseCourse 敘事與教學手法分析

> 由 4 個並行 subagent 通讀 4 個 Module 共 **42 個 .pptx、901 張投影片**（含 speaker notes 全文），彙整出共通敘事邏輯、行文風格、教學手段與價值觀點。

## 一、四個 Module 的敘事骨架對照

| Module | 敘事順序 | 一句話結論 |
|---|---|---|
| **M1 Intro to ML** | What → Why → How → **Ethics** → Tools | TinyML 不只塞模型，而是讓 AI 進入鈕扣電池，但要負責任地進入 |
| **M2 Deep Learning** | What → **How**（先實作）→ Why（最後談動機） | ML 就是「猜、量、修、再猜」四循環 + 塞進 256KB RAM 的工程 |
| **M3 ML Workflow** | Application → Problem → **Pipeline** | TinyML 成敗 90% 在資料工程與流程，不是模型本身 |
| **M4 Deployment** | What → Why → How → **Trade-offs** → Summary | 量化 + TFLite Micro = 把模型塞進 MCU 的標準答案 |

**共通內核**：四個模組都用「What/Why/How」三段為骨架，但每個都各自加上一個「特色強調章節」（M1 倫理、M2 從實作回推、M3 流程、M4 取捨）。

## 二、共通敘事手法（4 個模組高度一致）

### 1. 「漸進堆疊式投影片」 — 最具識別度的特徵

同一張畫面用 4-10 張投影片逐步加要素，每張多加一個 box / 條列 / 箭頭，模擬白板教學的「邊講邊畫」。

| Module | 經典案例 |
|---|---|
| M1 | Latency/Bandwidth/Power 三維對比，連用 7 張累積 |
| M1 | 晶片尺寸 561mm² → 83mm² → 30mm² → 3.2mm² 五張依次亮出 |
| M2 | "Make a Guess! → +Measure → +Optimize → +Repeat" 4 步堆疊出現 7 次以上 |
| M2 | Loss curve 連續 24 張只畫同一條曲線，箭頭一步步移動 |
| M3 | ML Lifecycle 從單一節點到完整圖（slide 3 → 10） |
| M3 | Confusion matrix 從 0 填到完整數字 |
| M4 | Echo Dot 拆機：Microphone → +IMU → +Temperature → +I/O |

### 2. 「單一例子貫穿整章」 — 記憶錨點手法

| Module | 主例 |
|---|---|
| M1 | Google Assistant「Okay, Google!」貫穿 1.1.5 |
| M2 | `Y = 2X-1` 數列從 2.1.1 一路用到 2.2.3（跨 5 個檔案） |
| M2 | 同一段 6 行 Keras code 反覆出現 20+ 次當敘事節拍器 |
| M3 | 三大應用支柱（Acoustic / Image / Motion）在開場與收尾都出現 |
| M4 | tfmot 量化範例重複 5 次加深印象 |

### 3. 「數字震撼對比」 — 製造張力

| 數字 | 用途 |
|---|---|
| 256KB RAM vs 16.9MB MobileNetv1 | 跨 M2/M3/M4 重複出現的反差 |
| 5 Quintillion bytes/day | 數據洪流規模感（M1） |
| <1% of unstructured data | 資料浪費的痛點（M1） |
| 250 Billion MCUs / 140 µW Syntiant | TinyML 的物質基礎（M1） |
| 100,000× / 10,000× / 1,000× / 10× | MCU vs MPU 差距（M2/M4） |
| 375 → 33 → 3 個特徵 | 特徵濃縮震撼（M3） |
| 0.973 accuracy 但 F1 只有 0.247 | Accuracy 騙人經典反例（M3） |

### 4. 「問句驅動」 — 用提問當小標題

四個模組都把問句當成「分節標題」使用：

- M1: `What am I building? / Who am I building this for? / What are the consequences?`
- M2: `How good is the guess? / What if we square them?`
- M3: `How do we engineer a TinyML vision network?`
- M4: `Why do we Quantize? / How do we Quantize? / What are the trade-offs?`

這種寫法把學員從被動聽課變成「跟著想」的主動角色。

### 5. 「鋪陳痛點 → 給解藥」的小型敘事弧

- M2: `"Profit!"` 鞋子分類器成功 → `"Profit?"` 高跟鞋翻車 → 引出 train/val/test split
- M3: 99% accuracy 的 naive classifier 永遠猜 field → 引出 confusion matrix
- M4: `:(` 空白投影片 表達 PTQ 掉準度 → 引出 QAT 解藥

### 6. 「Code-First」教學手法

特別在 M2、M4 中強烈：先丟一段可執行的 Python，再回頭拆解每一行的意義。**不是先講理論再寫程式，而是反過來**。

### 7. 收尾統一模板

| 結尾類型 | 出現位置 |
|---|---|
| `Your turn!` / `Quiz!` | M2 每個檔結尾固定 |
| `Fullscreen. Show presenter.` | M1/M2/M3 多檔結尾（Google Slides 簡報員提示） |
| `In Summary…` | M4 量化單元結尾 |

## 三、共通行文風格

### 1. 語氣定位

**口語化的學術風 + 白板教學感**
- 不是論文（沒有 abstract / methodology / conclusion）
- 不是新聞報導（沒有 5W1H 開場）
- 像「資深工程師在白板前邊講邊畫，偶爾停下來問你『你覺得呢？』」

### 2. 人稱與句型

- **第二人稱 `you / we / your` 極高頻**
- **短句、感嘆號、省略號**
- **祈使句**: `Don't collect from scratch.` / `Recall what I said...`
- **驚嘆語氣**: `A long... long... Long time!` / `Houston, we have a problem!`
- **口語拉長字**: `loong` / `Profit!?`

### 3. 經典金句節錄（跨四個模組）

> **概念定錨類**  
> "AI is one of the most important things humanity is working on. It is more profound than electricity or fire." — Sundar Pichai (M1)  
> "Bigger Is Not Always Better." (M1)  
> "From coding to learning..." (M2)  

> **教學節奏類**  
> "Make a Guess! / Measure your accuracy / Optimize your Guess / Repeat" (M2)  
> "Houston, we have a problem!" (M2)  
> "Profit?" (高跟鞋翻車轉折，M2)  

> **價值主張類**  
> "You can't train a model to win the lottery, no matter how much data you have." (M3)  
> "Good Data is Necessary for Accuracy." (M3)  
> "tinyML is all about on-device intelligence on battery power devices like a coin-cell!" (M3)  

> **工程現實類**  
> "Our board only has 256KB of RAM yet MobileNetv1 needs 16.9MB!" (M2/M4)  
> "Not all embedded systems are created equal. Sacrifice portability across systems for efficiency." (M4)  

> **倫理提醒類**  
> "AI is not always the best solution!" (M1)  
> "There are always biases in the data and where it comes from." (M3)  

## 四、共通價值觀點

### 1. 對 AI 的態度：**樂觀但帶責任感**

四個模組對 AI 都不是純粹推崇，而是「展示能力 + 提醒邊界」並陳：
- 樂觀面：AI 民主化、邊緣優先、coin-cell 上的智慧
- 責任面：Human-Centered Design、Bias 意識、AI is not always the best solution

### 2. 對學員的預設立場

| Module | 預設背景 | 證據 |
|---|---|---|
| M1 | 跨領域初學者（不一定有 ML 背景） | 從 AI 是什麼開始講 |
| M2 | 會 Python，但 ML 是新手 | 詳解 W、b、`tf.GradientTape`，不解釋 `import numpy` |
| M3 | 有程式基礎、想做專案的工程師或研究生 | 直接用 SOTA、quantization、GDPR 不解釋 |
| M4 | 想把模型塞硬體的工程學生 | 直接給 Python 程式碼開場 |

**整體梯度**：從 M1 的科普 → M4 的工程實作，難度線性上升。

### 3. 反覆強調的三大價值

1. **AI 民主化**（democratization）：模型走出資料中心、走進每顆 MCU
2. **資料中心思維**（data-centric）：90% 努力放在資料工程而非模型
3. **資源受限工程**（constrained engineering）：256KB RAM、coin-cell battery 是設計起點

### 4. credibility 三角

引用的權威來源高度集中：

| 來源類型 | 代表 |
|---|---|
| **企業** | Google（TPU/Colab/TensorFlow/PAIR/Responsible AI）、Apple、Amazon Echo、Arduino、Espressif |
| **學術** | Stanford HAI、Stuart Russell、Nick Bostrom、Pete Warden、Song Han、IEEE Access |
| **國際組織** | UN SDGs、EU 7 Key Requirements for Trustworthy AI |

**主軸講者**：Laurence Moroney (Google) — 整套課程的內容指紋。

## 五、抽象出來的「敘事公式」

把這 4 個 Module 共通的敘事手法抽象化，可歸納為以下 **「Google Tech Edu」敘事公式**：

```
[名人引言開場]
    ↓
[單一具體例子鎖定]
    ↓
[同一畫面漸進堆疊 N 張投影片]
    ↓
[數字震撼對比製造張力]
    ↓
[小型敘事弧：成功 → 翻車 → 解藥]
    ↓
[Code-first 給程式碼，反推概念]
    ↓
[問句當小標題串接子主題]
    ↓
[反覆出現的「框架圖」當記憶錨點]
    ↓
[Your turn! 把球丟回給學員]
```

## 六、整體一句話評價

> **「這是 Google 派的工程師白板教學風 — 不是論文、不是科普、不是行銷簡報，而是『資深前輩邊講邊畫，每隔幾分鐘把球丟回給你想一下』的節奏。內容上樂觀但有責任感，技術上聚焦資源受限的工程現實，引用上以 Google 生態為主軸。」**

## 七、本土化二創建議

### 適合直接套用的手法
- ✅ **漸進堆疊式投影片**（同畫面多版本，PowerPoint 用 v-click 或多張即可實現）
- ✅ **單一例子貫穿章節**（用工安事故報表自動化貫穿整個輔英課程）
- ✅ **問句當小標題**（「為什麼工安人也要會 AI？」「我們用什麼工具？」「你會帶走什麼？」）
- ✅ **數字震撼對比**（80% 重複工作、365 天巡檢、1 個 AI 助理）
- ✅ **Code-first 範例**（先給 AI 寫的程式，再說「這是怎麼產生的」）
- ✅ **Your turn! 結尾**（每章末給學員一個小挑戰）

### 需在地化的元素
- 🔄 **引用權威**：把 Sundar Pichai / Pete Warden 替換為**台灣本土案例**（張善政談 AI、台積電智慧工廠、台大資工教授等）
- 🔄 **生活案例**：把 Google Assistant / Apple Watch 換成**台灣熟悉的**（LINE Bot、發票對獎、Garmin 手錶）
- 🔄 **工業案例**：把 Diabetic Retinopathy 換成**工安現場**（PPE 偵測、設備異常、巡檢自動化）
- 🔄 **價值定位**：原本「AI 民主化」+ Google 中心 → 換成「**擺脫大廠雲端依賴**」+ 開源平台中心

### 不要照抄的部分
- ❌ Speaker notes 文化（台灣講者較少用，且學生看不到）
- ❌ `Fullscreen. Show presenter.` 結尾投影片（Google Slides 殘留）
- ❌ Mechanical Turk / GDPR 等對台灣學生陌生的概念
- ❌ "Profit?" 這類英文雙關（換成中文也丟失幽默感）

---
*分析時間：2026-05-17*  
*工具：Python python-pptx + 4 個 Claude 並行 subagent*  
*姊妹報告：`STYLE_ANALYSIS.md`（視覺設計面）*
