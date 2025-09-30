#include <Button2.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// GPIO 定義 / GPIO definitions
#define LED_PIN_R 0   // 紅色 LED 腳位 / Red LED pin
#define LED_PIN_G 1   // 綠色 LED 腳位 / Green LED pin
#define LED_PIN_B 2   // 藍色 LED 腳位 / Blue LED pin
#define BTN1_PIN 4  // 錄製按鈕（短按啟動）/ Record button (short press)
#define BTN2_PIN 5  // 模式按鈕（切換案例）/ Mode button (switch case)

// SD 卡腳位 / SD card pin map
#define CS_PIN 9   // SD 卡晶片選擇 / SD card chip select
#define SCK_PIN 10  // SD 卡 SPI 時脈 / SD card SPI clock
#define MOSI_PIN 11 // SD 卡 MOSI / SD card MOSI
#define MISO_PIN 12 // SD 卡 MISO / SD card MISO
// MPU6050 I2C 位址 / MPU6050 I2C address
const int MPU = 0x68;

// 按鈕物件 / Button instances
Button2 btn1;
Button2 btn2;

// 系統狀態宣告 / System state enum
// 系統狀態機制 / System state machine
enum SystemState {
  STATE_SETUP,     // 設定狀態 / Setup state
  STATE_RUNNING    // 運行狀態 / Running state
};

// 取樣模式列舉 / Sampling modes
enum SamplingMode {
  MODE_IDLE,           // 閒置 / Idle
  MODE_SINGLE,         // 單次採樣 / Single capture
  MODE_CONTINUOUS,     // 連續採樣 / Continuous capture
  MODE_DELETE_PENDING  // 準備刪除 / Delete countdown
};

// 全域變數 / Global variables
volatile SystemState currentState = STATE_SETUP;
volatile SamplingMode samplingMode = MODE_IDLE;
volatile int caseNumber = 0;
volatile bool dataReady = false;
volatile bool stopContinuous = false;
volatile bool cancelDelete = false;
volatile int sampleIndex = 0;

// 資料陣列 3軸 * 1000點
float imuSamples[3][1000];  // IMU 取樣緩衝（3 軸 × 1000 筆）/ IMU sample buffer (3 axes × 1000 samples)

// 時間相關
unsigned long sampleStartTime = 0;
unsigned long deleteStartTime = 0;
unsigned long lastSampleTime = 0;

// 檔案計數 - 追蹤每個case的檔案數量
int caseFileCounts[4] = {0, 0, 0, 0};  // 案例檔案計數（case0-case3）/ File count per case (case0-case3)

// Core1變數
volatile bool core1Ready = false;

// 初始化主核心與外設 / Initialize main core and peripherals
void setup() {
  Serial.begin(115200);
  delay(2000);
  
  // 初始化GPIO
  pinMode(LED_PIN_R, OUTPUT);
  pinMode(LED_PIN_G, OUTPUT);
  pinMode(LED_PIN_B, OUTPUT);
  
  // 關閉所有LED
  digitalWrite(LED_PIN_R, LOW);
  digitalWrite(LED_PIN_G, LOW);
  digitalWrite(LED_PIN_B, LOW);
  
  Serial.println("System Initializing...");
  
  // 開機RGB閃爍兩次 (1秒亮/1秒滅)
  for(int i = 0; i < 2; i++) {
    digitalWrite(LED_PIN_R, HIGH);
    digitalWrite(LED_PIN_G, HIGH);
    digitalWrite(LED_PIN_B, HIGH);
    delay(1000);
    digitalWrite(LED_PIN_R, LOW);
    digitalWrite(LED_PIN_G, LOW);
    digitalWrite(LED_PIN_B, LOW);
    delay(1000);
  }
  
  // 初始化SD卡
  Serial.println("Initializing SD card...");
  SPI1.setRX(MISO_PIN);
  SPI1.setTX(MOSI_PIN);
  SPI1.setSCK(SCK_PIN);
  
  if (SD.begin(CS_PIN, SPI1)) {
    Serial.println("SD card initialized successfully");
    // 成功閃爍R燈 (0.5秒亮/0.5秒滅)
    digitalWrite(LED_PIN_R, HIGH);
    delay(500);
    digitalWrite(LED_PIN_R, LOW);
    delay(500);
  } else {
    Serial.println("SD card initialization failed");
    while(1) {
      digitalWrite(LED_PIN_R, HIGH);
      delay(200);
      digitalWrite(LED_PIN_R, LOW);
      delay(200);
    }
  }
  
  // 初始化MPU6050
  Serial.println("Initializing MPU6050...");
  Wire1.setSDA(14);
  Wire1.setSCL(15);
  Wire1.begin();
  Wire1.setClock(400000);
  
  initMPU6050();
  
  // 成功閃爍G燈 (0.5秒亮/0.5秒滅)
  digitalWrite(LED_PIN_G, HIGH);
  delay(500);
  digitalWrite(LED_PIN_G, LOW);
  delay(500);
  
  // 初始化所有case的檔案計數
  initializeCaseFileCounts();
  
  // 顯示開機狀態
  bool hasOldFiles = hasAnyOldFiles();
  if (!hasOldFiles) {
    // 沒有舊檔案，閃爍B燈一次 (0.5秒亮/0.5秒滅)
    digitalWrite(LED_PIN_B, HIGH);
    delay(500);
    digitalWrite(LED_PIN_B, LOW);
    delay(500);
  } else {
    // 有舊檔案，閃爍B燈三次 (0.8秒亮/0.2秒滅)
    for(int i = 0; i < 3; i++) {
      digitalWrite(LED_PIN_B, HIGH);
      delay(800);
      digitalWrite(LED_PIN_B, LOW);
      delay(200);
    }
  }
  
  // 初始化按鈕
  btn1.begin(BTN1_PIN);
  btn1.setDebounceTime(50);
  btn1.setLongClickTime(1000);
  btn1.setDoubleClickTime(400);
  
  btn2.begin(BTN2_PIN);
  btn2.setDebounceTime(50);
  
  // BTN1事件處理
  btn1.setClickHandler(handleBtn1SingleClick);
  btn1.setDoubleClickHandler(handleBtn1DoubleClick);
  btn1.setLongClickHandler(handleBtn1LongClick);
  
  // BTN2事件處理
  btn2.setClickHandler(handleBtn2SingleClick);
  
  // 更新case LED顯示
  updateCaseLED();
  
  Serial.println("System ready. Current state: SETUP");
  Serial.println("Current case: 0");
}

// 初始化第二核心（Core1） / Initialize secondary core (Core1)
void setup1() {
  // Core1初始化
  delay(3000);
  core1Ready = true;
}

// Core0 主迴圈：按鍵與狀態管理 / Core0 loop: button & state management
void loop() {
  // Core0 - 主控制邏輯
  btn1.loop();
  btn2.loop();
  
  // 處理刪除倒數計時
  if (samplingMode == MODE_DELETE_PENDING && currentState == STATE_RUNNING) {
    if (millis() - deleteStartTime >= 10000 && !cancelDelete) {
      // 執行刪除
      deleteCurrentCaseFiles();
      digitalWrite(LED_PIN_R, LOW);
      digitalWrite(LED_PIN_G, LOW);
      digitalWrite(LED_PIN_B, LOW);
      currentState = STATE_SETUP;
      samplingMode = MODE_IDLE;
      updateCaseLED();
      Serial.println("Deletion completed. Back to SETUP state");
    } else if (cancelDelete) {
      // 取消刪除
      digitalWrite(LED_PIN_R, LOW);
      digitalWrite(LED_PIN_G, LOW);
      digitalWrite(LED_PIN_B, LOW);
      currentState = STATE_SETUP;
      samplingMode = MODE_IDLE;
      cancelDelete = false;
      updateCaseLED();
      Serial.println("Deletion cancelled. Back to SETUP state");
    }
  }
  
  // 處理資料寫入
  if (dataReady && sampleIndex >= 1000) {
    writeDataToSD();
    dataReady = false;
    sampleIndex = 0;
    
    if (samplingMode == MODE_SINGLE) {
      // 單次採樣 / Single capture完成
      digitalWrite(LED_PIN_R, LOW);
      currentState = STATE_SETUP;
      samplingMode = MODE_IDLE;
      updateCaseLED();
      Serial.println("Single sampling completed. Back to SETUP state");
    } else if (samplingMode == MODE_CONTINUOUS) {
      if (stopContinuous) {
        // 連續採樣 / Continuous capture停止
        digitalWrite(LED_PIN_R, LOW);
        currentState = STATE_SETUP;
        samplingMode = MODE_IDLE;
        stopContinuous = false;
        updateCaseLED();
        Serial.println("Continuous sampling stopped. Back to SETUP state");
      }
      // 如果沒有停止訊號，繼續採樣下一批資料
    }
  }
}

// Core1 監控旗標並寫入資料 / Core1 monitors flags and writes data
void loop1() {
  // Core1 - 資料採樣
  if (!core1Ready) return;
  
  if ((samplingMode == MODE_SINGLE || samplingMode == MODE_CONTINUOUS) && 
      currentState == STATE_RUNNING) {
    
    unsigned long currentTime = millis();
    
    // 1kHz採樣率
    if (currentTime - lastSampleTime >= 1) {
      lastSampleTime = currentTime;
      
      if (sampleIndex < 1000) {
        // 讀取MPU6050資料
        float accX, accY, accZ;
        readAcceleration(accX, accY, accZ);
        
        // 儲存到陣列
        imuSamples[0][sampleIndex] = accX;
        imuSamples[1][sampleIndex] = accY;
        imuSamples[2][sampleIndex] = accZ;
        
        sampleIndex++;
        
        // 採樣滿1000點
        if (sampleIndex >= 1000) {
          dataReady = true;
        }
      }
    }
  }
}

// 按鈕事件處理函式
// Btn1 單擊：啟動單次取樣 / Btn1 single click: start single capture
void handleBtn1SingleClick(Button2 &b) {
  if (currentState == STATE_SETUP) {
    // 開始單次採樣
    Serial.println("Starting single sampling...");
    currentState = STATE_RUNNING;
    samplingMode = MODE_SINGLE;
    sampleIndex = 0;
    dataReady = false;
    sampleStartTime = millis();
    lastSampleTime = millis();
    
    // 亮R燈，保持case LED
    digitalWrite(LED_PIN_R, HIGH);
    digitalWrite(LED_PIN_G, (caseNumber & 0x02) ? HIGH : LOW);
    digitalWrite(LED_PIN_B, (caseNumber & 0x01) ? HIGH : LOW);
  } else if (currentState == STATE_RUNNING && samplingMode == MODE_CONTINUOUS) {
    // 停止連續採樣
    stopContinuous = true;
    Serial.println("Stopping continuous sampling...");
  } else if (samplingMode == MODE_DELETE_PENDING) {
    // 取消刪除
    cancelDelete = true;
    Serial.println("Delete cancelled");
  }
}

// Btn1 雙擊：切換案例編號 / Btn1 double click: switch case index
void handleBtn1DoubleClick(Button2 &b) {
  if (currentState == STATE_SETUP) {
    // 開始連續採樣
    Serial.println("Starting continuous sampling...");
    currentState = STATE_RUNNING;
    samplingMode = MODE_CONTINUOUS;
    sampleIndex = 0;
    dataReady = false;
    stopContinuous = false;
    sampleStartTime = millis();
    lastSampleTime = millis();
    
    // 亮R燈，保持case LED
    digitalWrite(LED_PIN_R, HIGH);
    digitalWrite(LED_PIN_G, (caseNumber & 0x02) ? HIGH : LOW);
    digitalWrite(LED_PIN_B, (caseNumber & 0x01) ? HIGH : LOW);
  }
}

// Btn1 長按：排程刪除資料 / Btn1 long press: schedule deletion
void handleBtn1LongClick(Button2 &b) {
  if (currentState == STATE_SETUP) {
    // 準備刪除 / Delete countdown
    Serial.println("Delete mode activated. Wait 10 seconds or press BTN1 to cancel");
    currentState = STATE_RUNNING;
    samplingMode = MODE_DELETE_PENDING;
    deleteStartTime = millis();
    cancelDelete = false;
    
    // RGB全亮
    digitalWrite(LED_PIN_R, HIGH);
    digitalWrite(LED_PIN_G, HIGH);
    digitalWrite(LED_PIN_B, HIGH);
  }
}

// Btn2 單擊：切換連續取樣 / Btn2 single click: toggle continuous mode
void handleBtn2SingleClick(Button2 &b) {
  if (currentState == STATE_SETUP) {
    // 切換case編號
    caseNumber = (caseNumber + 1) % 4;
    updateCaseLED();
    Serial.print("Case switched to: ");
    Serial.println(caseNumber);
    Serial.print("Files in this case: ");
    Serial.println(caseFileCounts[caseNumber]);
  }
}

// 更新case LED顯示
// 依案例顯示 RGB LED / Update RGB LED based on case index
void updateCaseLED() {
  if (currentState != STATE_SETUP) return;
  
  digitalWrite(LED_PIN_R, LOW);
  // LED_PIN_G = bit1, LED_PIN_B = bit0
  digitalWrite(LED_PIN_G, (caseNumber & 0x02) ? HIGH : LOW);
  digitalWrite(LED_PIN_B, (caseNumber & 0x01) ? HIGH : LOW);
}

// 初始化MPU6050
// 初始化 MPU6050 與量測範圍 / Initialize MPU6050 and measurement range
void initMPU6050() {
  writeRegister(0x6B, 0x00);  // 喚醒
  delay(100);
  writeRegister(0x1C, 0x00);  // ±2g
  delay(10);
  writeRegister(0x1B, 0x00);  // ±250°/s
  delay(10);
  writeRegister(0x19, 0x00);  // 1kHz
  delay(10);
  writeRegister(0x1A, 0x00);  // 關閉DLPF
  delay(10);
  Serial.println("MPU6050 initialized successfully");
}

// 寫入 MPU6050 暫存器 / Write MPU6050 register
void writeRegister(uint8_t reg, uint8_t value) {
  Wire1.beginTransmission(MPU);
  Wire1.write(reg);
  Wire1.write(value);
  Wire1.endTransmission();
}

// 讀取三軸加速度資料 / Read tri-axis acceleration
void readAcceleration(float &accX, float &accY, float &accZ) {
  Wire1.beginTransmission(MPU);
  Wire1.write(0x3B);
  Wire1.endTransmission(false);
  Wire1.requestFrom(MPU, 6, true);
  
  if (Wire1.available() >= 6) {
    int16_t rawX = (Wire1.read() << 8) | Wire1.read();
    int16_t rawY = (Wire1.read() << 8) | Wire1.read();
    int16_t rawZ = (Wire1.read() << 8) | Wire1.read();
    
    accX = rawX / 16384.0;
    accY = rawY / 16384.0;
    accZ = rawZ / 16384.0;
  }
}

// 初始化所有case的檔案計數
// 掃描 SD 卡統計案例檔案 / Scan SD card for case counts
void initializeCaseFileCounts() {
  File root = SD.open("/");
  
  // 重置計數
  for(int i = 0; i < 4; i++) {
    caseFileCounts[i] = 0;
  }
  
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    
    String filename = entry.name();
    entry.close();
    
    // 解析檔名格式：caseX.sampleY.csv
    if (filename.startsWith("case") && filename.endsWith(".csv")) {
      // 提取case編號
      int caseNumEnd = filename.indexOf('.', 4);
      if (caseNumEnd > 4) {
        String caseNumStr = filename.substring(4, caseNumEnd);
        int caseNum = caseNumStr.toInt();
        
        if (caseNum >= 0 && caseNum < 4) {
          // 提取sample編號
          int sampleStart = filename.indexOf("sample");
          if (sampleStart > 0) {
            int sampleNumStart = sampleStart + 6;
            int sampleNumEnd = filename.indexOf('.', sampleNumStart);
            if (sampleNumEnd > sampleNumStart) {
              String sampleNumStr = filename.substring(sampleNumStart, sampleNumEnd);
              int sampleNum = sampleNumStr.toInt();
              
              // 更新該case的最大sample編號
              if (sampleNum > caseFileCounts[caseNum]) {
                caseFileCounts[caseNum] = sampleNum;
              }
            }
          }
        }
      }
    }
  }
  root.close();
  
  // 輸出掃描結果
  Serial.println("File count scan results:");
  for(int i = 0; i < 4; i++) {
    Serial.print("Case ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(caseFileCounts[i]);
    Serial.println(" files");
  }
}

// 檢查是否有任何舊檔案
bool hasAnyOldFiles() {
  for(int i = 0; i < 4; i++) {
    if (caseFileCounts[i] > 0) {
      return true;
    }
  }
  return false;
}

// 寫入資料到SD卡
// 將緩衝資料寫入 SD 卡 / Flush buffered samples to SD card
void writeDataToSD() {
  // 直接使用記憶體中的計數，並遞增
  caseFileCounts[caseNumber]++;
  String filename = "case" + String(caseNumber) + ".sample" + String(caseFileCounts[caseNumber]) + ".csv";
  
  File dataFile = SD.open(filename, FILE_WRITE);
  if (dataFile) {
    // 寫入標題
    dataFile.println("timestamp,signal_x,signal_y,signal_z");
    
    // 寫入資料
    for (int i = 0; i < 1000; i++) {
      dataFile.print(i);
      dataFile.print(".0,");
      dataFile.print(imuSamples[0][i], 3);
      dataFile.print(",");
      dataFile.print(imuSamples[1][i], 3);
      dataFile.print(",");
      dataFile.println(imuSamples[2][i], 3);
    }
    
    dataFile.close();
    Serial.print("Data saved to: ");
    Serial.println(filename);
  } else {
    // 存檔失敗，恢復計數
    caseFileCounts[caseNumber]--;
    Serial.println("Error writing to SD card");
    // 錯誤時R燈閃爍
    for(int i = 0; i < 5; i++) {
      digitalWrite(LED_PIN_R, HIGH);
      delay(100);
      digitalWrite(LED_PIN_R, LOW);
      delay(100);
    }
  }
}

// 刪除當前case的所有檔案
// 刪除目前案例所有檔案 / Delete all files for current case
void deleteCurrentCaseFiles() {
  File root = SD.open("/");
  String prefix = "case" + String(caseNumber) + ".sample";
  int deletedCount = 0;
  
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    
    String filename = entry.name();
    entry.close();
    
    if (filename.startsWith(prefix) && filename.endsWith(".csv")) {
      SD.remove(filename);
      deletedCount++;
      Serial.print("Deleted: ");
      Serial.println(filename);
    }
  }
  root.close();
  
  // 重置該case的檔案計數
  caseFileCounts[caseNumber] = 0;
  
  Serial.print("Total files deleted: ");
  Serial.println(deletedCount);
}