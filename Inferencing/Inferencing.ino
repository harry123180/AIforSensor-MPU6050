  /* Edge Impulse Arduino inferencing library
  * MPU6050 三軸加速度計推論程式
  */

  #include <EdgeAI_inferencing.h>
  #include <Wire.h>

  // 記憶體優化
  #define EIDSP_QUANTIZE_FILTERBANK   0

  // MPU6050設定
  const int MPU = 0x68;

  // 根據你的模型，1秒窗口，1000Hz，3軸
  // 總共需要 1000 * 3 = 3000 個資料點
  static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

  // 採樣控制
  unsigned long last_sample_time = 0;
  const int SAMPLE_INTERVAL_MS = 1;  // 取樣間隔 1ms（1000Hz）/ Sampling interval 1 ms (1000 Hz)
  int feature_ix = 0;
  bool sampling_done = false;

  /**
  * @brief      感測器資料供 Edge Impulse 讀取 / Provide sensor data for Edge Impulse
  *             取得原始資料的回調函式
  */
  int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
      memcpy(out_ptr, features + offset, length * sizeof(float));
      return 0;
  }

  // 初始化序列埠與 MPU6050 / Initialize serial and MPU6050
  void setup() {
      Serial.begin(115200);
      while (!Serial);
      
      Serial.println("Edge Impulse MPU6050 Inference");
      Serial.println("================================");
      
      // 初始化MPU6050
      Serial.print("Initializing MPU6050...");
      Wire1.setSDA(14);
      Wire1.setSCL(15);
      Wire1.begin();
      Wire1.setClock(400000);
      
      // 檢查MPU6050
      Wire1.beginTransmission(MPU);
      if (Wire1.endTransmission() != 0) {
          Serial.println(" FAILED!");
          Serial.println("Please check MPU6050 connection.");
          while(1);
      }
      
      initMPU6050();
      Serial.println(" OK");
      
      // 顯示模型資訊
      Serial.println("\nModel information:");
      Serial.print("  Number of classes: ");
      Serial.println(sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));
      Serial.print("  Classes: ");
      for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
          Serial.print(ei_classifier_inferencing_categories[ix]);
          if (ix != EI_CLASSIFIER_LABEL_COUNT - 1) {
              Serial.print(", ");
          }
      }
      Serial.println();
      
      Serial.print("  Sample length: ");
      Serial.print(EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE / 3);  // 除以3得到採樣點數
      Serial.println(" ms");
      
      Serial.print("  DSP input size: ");
      Serial.println(EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
      
      Serial.println("\nStarting continuous inference in 2 seconds...");
      delay(2000);
      Serial.println("\n>>> Inference started <<<\n");
  }

  // 主迴圈：取樣並執行推論 / Main loop: sample and run inference
  void loop() {
      // 階段1: 收集資料
      Serial.println("Collecting data...");
      collectSamples();
      
      // 階段2: 執行推論
      Serial.println("Running inference...");
      
      // 建立signal
      signal_t signal;
      signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
      signal.get_data = &raw_feature_get_data;
      
      // 執行分類器
      ei_impulse_result_t result = { 0 };
      
      EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
      
      if (res != EI_IMPULSE_OK) {
          Serial.print("ERR: Failed to run classifier (");
          Serial.print(res);
          Serial.println(")");
          delay(1000);
          return;
      }
      
      // 階段3: 輸出結果
      printResults(result);
      
      // 短暫延遲後繼續
      delay(100);
  }

  /**
  * @brief 收集1秒的資料 (1000個採樣點 x 3軸)
  */
  void collectSamples() {
      feature_ix = 0;
      last_sample_time = millis();
      unsigned long start_time = millis();
      
      // 需要收集的總資料點數
      int total_samples = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE / 3;
      
      while (feature_ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
          unsigned long current_time = millis();
          
          // 每1ms採樣一次
          if (current_time - last_sample_time >= SAMPLE_INTERVAL_MS) {
              last_sample_time = current_time;
              
              // 讀取加速度
              float accX, accY, accZ;
              readAcceleration(accX, accY, accZ);
              
              // 儲存到features陣列
              if (feature_ix + 2 < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
                  features[feature_ix++] = accX;
                  features[feature_ix++] = accY;
                  features[feature_ix++] = accZ;
                  
                  // 顯示進度 (每100ms)
                  if ((feature_ix / 3) % 100 == 0) {
                      Serial.print(".");
                  }
              }
          }
          
          // 防止無限迴圈
          if (millis() - start_time > 2000) {
              Serial.println("\nWarning: Data collection timeout!");
              break;
          }
      }
      
      Serial.println(" Done!");
      Serial.print("Collected ");
      Serial.print(feature_ix / 3);
      Serial.println(" samples");
  }

  /**
  * @brief 列印推論結果
  */
  void printResults(ei_impulse_result_t result) {
      Serial.println("\n========== Results ==========");
      
      // 時間資訊
      Serial.print("DSP: ");
      Serial.print(result.timing.dsp);
      Serial.print(" ms, Classification: ");
      Serial.print(result.timing.classification);
      Serial.print(" ms, Anomaly: ");
      Serial.print(result.timing.anomaly);
      Serial.println(" ms");
      
      // 分類結果
      Serial.println("\nPredictions:");
      for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
          Serial.print("  ");
          Serial.print(result.classification[ix].label);
          Serial.print(": ");
          
          // 顯示百分比
          float value = result.classification[ix].value;
          Serial.print(value * 100.0, 1);
          Serial.print("% (");
          
          // 顯示信心度條
          int bar_length = (int)(value * 20);
          for (int i = 0; i < 20; i++) {
              if (i < bar_length) Serial.print("█");
              else Serial.print("░");
          }
          Serial.println(")");
      }
      
      // 找出最高信心度的類別
      float max_value = 0;
      const char* max_label = "unknown";
      for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
          if (result.classification[ix].value > max_value) {
              max_value = result.classification[ix].value;
              max_label = result.classification[ix].label;
          }
      }
      
      Serial.print("\n>>> DETECTED: ");
      Serial.print(max_label);
      Serial.print(" (");
      Serial.print(max_value * 100.0, 1);
      Serial.println("%) <<<");
      
  #if EI_CLASSIFIER_HAS_ANOMALY == 1
      Serial.print("Anomaly score: ");
      Serial.println(result.anomaly, 3);
  #endif
      
      Serial.println("=============================\n");
  }

  /**
  * @brief 初始化MPU6050
  */
  void initMPU6050() {
      // 喚醒MPU6050
      writeRegister(0x6B, 0x00);
      delay(100);
      
      // 設定加速度計範圍 ±2g
      writeRegister(0x1C, 0x00);
      delay(10);
      
      // 設定陀螺儀範圍 ±250°/s
      writeRegister(0x1B, 0x00);
      delay(10);
      
      // 設定採樣率 1kHz
      writeRegister(0x19, 0x00);
      delay(10);
      
      // 關閉DLPF
      writeRegister(0x1A, 0x00);
      delay(10);
  }

  void writeRegister(uint8_t reg, uint8_t value) {
      Wire1.beginTransmission(MPU);
      Wire1.write(reg);
      Wire1.write(value);
      Wire1.endTransmission();
  }

  void readAcceleration(float &accX, float &accY, float &accZ) {
      Wire1.beginTransmission(MPU);
      Wire1.write(0x3B); // ACCEL_XOUT_H
      Wire1.endTransmission(false);
      Wire1.requestFrom(MPU, 6, true);
      
      if (Wire1.available() >= 6) {
          int16_t rawX = (Wire1.read() << 8) | Wire1.read();
          int16_t rawY = (Wire1.read() << 8) | Wire1.read();
          int16_t rawZ = (Wire1.read() << 8) | Wire1.read();
          
          // 轉換為g值 (±2g範圍)
          accX = rawX / 16384.0;
          accY = rawY / 16384.0;
          accZ = rawZ / 16384.0;
      } else {
          // 如果讀取失敗，返回預設值
          accX = 0.0;
          accY = 0.0;
          accZ = 1.0;
      }
  }