#include <Button2.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// GPIO定義
#define R 0
#define G 1
#define B 2
#define BTN1_PIN 4  // 開始按鈕
#define BTN2_PIN 5  // 切換按鈕

// SD卡腳位
#define CS_PIN 9
#define SCK_PIN 10
#define MOSI_PIN 11
#define MISO_PIN 12

// MPU6050
const int MPU = 0x68;

// 按鈕物件
Button2 btn1;
Button2 btn2;

// 系統狀態
enum SystemState {
  STATE_SETUP,     // 設定狀態
  STATE_RUNNING    // 運行狀態
};

enum SamplingMode {
  MODE_IDLE,           // 閒置
  MODE_SINGLE,         // 單次採樣
  MODE_CONTINUOUS,     // 連續採樣
  MODE_DELETE_PENDING  // 準備刪除
};

// 全域變數
volatile SystemState currentState = STATE_SETUP;
volatile SamplingMode samplingMode = MODE_IDLE;
volatile int caseNumber = 0;
volatile bool dataReady = false;
volatile bool stopContinuous = false;
volatile bool cancelDelete = false;
volatile int sampleIndex = 0;

// 資料陣列 3軸 * 1000點
float SIG_DATA[3][1000];

// 時間相關
unsigned long sampleStartTime = 0;
unsigned long deleteStartTime = 0;
unsigned long lastSampleTime = 0;

// 檔案計數 - 追蹤每個case的檔案數量
int caseFileCounts[4] = {0, 0, 0, 0};  // 支援case0-case3

// Core1變數
volatile bool core1Ready = false;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  // 初始化GPIO
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);
  
  // 關閉所有LED
  digitalWrite(R, LOW);
  digitalWrite(G, LOW);
  digitalWrite(B, LOW);
  
  Serial.println("System Initializing...");
  
  // 開機RGB閃爍兩次 (1秒亮/1秒滅)
  for(int i = 0; i < 2; i++) {
    digitalWrite(R, HIGH);
    digitalWrite(G, HIGH);
    digitalWrite(B, HIGH);
    delay(1000);
    digitalWrite(R, LOW);
    digitalWrite(G, LOW);
    digitalWrite(B, LOW);
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
    digitalWrite(R, HIGH);
    delay(500);
    digitalWrite(R, LOW);
    delay(500);
  } else {
    Serial.println("SD card initialization failed");
    while(1) {
      digitalWrite(R, HIGH);
      delay(200);
      digitalWrite(R, LOW);
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
  digitalWrite(G, HIGH);
  delay(500);
  digitalWrite(G, LOW);
  delay(500);
  
  // 初始化所有case的檔案計數
  initializeCaseFileCounts();
  
  // 顯示開機狀態
  bool hasOldFiles = hasAnyOldFiles();
  if (!hasOldFiles) {
    // 沒有舊檔案，閃爍B燈一次 (0.5秒亮/0.5秒滅)
    digitalWrite(B, HIGH);
    delay(500);
    digitalWrite(B, LOW);
    delay(500);
  } else {
    // 有舊檔案，閃爍B燈三次 (0.8秒亮/0.2秒滅)
    for(int i = 0; i < 3; i++) {
      digitalWrite(B, HIGH);
      delay(800);
      digitalWrite(B, LOW);
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

void setup1() {
  // Core1初始化
  delay(3000);
  core1Ready = true;
}

void loop() {
  // Core0 - 主控制邏輯
  btn1.loop();
  btn2.loop();
  
  // 處理刪除倒數計時
  if (samplingMode == MODE_DELETE_PENDING && currentState == STATE_RUNNING) {
    if (millis() - deleteStartTime >= 10000 && !cancelDelete) {
      // 執行刪除
      deleteCurrentCaseFiles();
      digitalWrite(R, LOW);
      digitalWrite(G, LOW);
      digitalWrite(B, LOW);
      currentState = STATE_SETUP;
      samplingMode = MODE_IDLE;
      updateCaseLED();
      Serial.println("Deletion completed. Back to SETUP state");
    } else if (cancelDelete) {
      // 取消刪除
      digitalWrite(R, LOW);
      digitalWrite(G, LOW);
      digitalWrite(B, LOW);
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
      // 單次採樣完成
      digitalWrite(R, LOW);
      currentState = STATE_SETUP;
      samplingMode = MODE_IDLE;
      updateCaseLED();
      Serial.println("Single sampling completed. Back to SETUP state");
    } else if (samplingMode == MODE_CONTINUOUS) {
      if (stopContinuous) {
        // 連續採樣停止
        digitalWrite(R, LOW);
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
        SIG_DATA[0][sampleIndex] = accX;
        SIG_DATA[1][sampleIndex] = accY;
        SIG_DATA[2][sampleIndex] = accZ;
        
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
    digitalWrite(R, HIGH);
    digitalWrite(G, (caseNumber & 0x02) ? HIGH : LOW);
    digitalWrite(B, (caseNumber & 0x01) ? HIGH : LOW);
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
    digitalWrite(R, HIGH);
    digitalWrite(G, (caseNumber & 0x02) ? HIGH : LOW);
    digitalWrite(B, (caseNumber & 0x01) ? HIGH : LOW);
  }
}

void handleBtn1LongClick(Button2 &b) {
  if (currentState == STATE_SETUP) {
    // 準備刪除
    Serial.println("Delete mode activated. Wait 10 seconds or press BTN1 to cancel");
    currentState = STATE_RUNNING;
    samplingMode = MODE_DELETE_PENDING;
    deleteStartTime = millis();
    cancelDelete = false;
    
    // RGB全亮
    digitalWrite(R, HIGH);
    digitalWrite(G, HIGH);
    digitalWrite(B, HIGH);
  }
}

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
void updateCaseLED() {
  if (currentState != STATE_SETUP) return;
  
  digitalWrite(R, LOW);
  // G = bit1, B = bit0
  digitalWrite(G, (caseNumber & 0x02) ? HIGH : LOW);
  digitalWrite(B, (caseNumber & 0x01) ? HIGH : LOW);
}

// 初始化MPU6050
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

void writeRegister(uint8_t reg, uint8_t value) {
  Wire1.beginTransmission(MPU);
  Wire1.write(reg);
  Wire1.write(value);
  Wire1.endTransmission();
}

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
      dataFile.print(SIG_DATA[0][i], 3);
      dataFile.print(",");
      dataFile.print(SIG_DATA[1][i], 3);
      dataFile.print(",");
      dataFile.println(SIG_DATA[2][i], 3);
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
      digitalWrite(R, HIGH);
      delay(100);
      digitalWrite(R, LOW);
      delay(100);
    }
  }
}

// 刪除當前case的所有檔案
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