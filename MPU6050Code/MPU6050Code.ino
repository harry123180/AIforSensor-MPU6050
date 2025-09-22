#include <Wire.h>

const int MPU = 0x68;
float AccX, AccY, AccZ;
unsigned long lastTime = 0;

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  Serial.println("MPU6050 Accelerometer Test");
  
  // 使用I2C1 (GP14/GP15)
  Wire1.setSDA(14);
  Wire1.setSCL(15);
  Wire1.begin();
  Wire1.setClock(400000); // 400kHz for faster reading
  
  // 初始化MPU6050
  initMPU6050();
  
  Serial.println("MPU6050 initialized successfully!");
  Serial.println("±2g range, 1kHz sampling");
  Serial.println("AccelX,AccelY,AccelZ");
  delay(1000);
}

void initMPU6050() {
  // 喚醒MPU6050 (Power Management 1)
  writeRegister(0x6B, 0x00);
  delay(100);
  
  // 設定加速度範圍為±2g (Accelerometer Configuration)
  writeRegister(0x1C, 0x00); // AFS_SEL = 0 (±2g)
  delay(10);
  
  // 設定陀螺儀範圍為±250°/s (Gyroscope Configuration)
  writeRegister(0x1B, 0x00); // FS_SEL = 0 (±250°/s)
  delay(10);
  
  // 設定取樣率為1kHz (Sample Rate Divider)
  writeRegister(0x19, 0x00); // SMPLRT_DIV = 0
  delay(10);
  
  // 關閉數位低通濾波器以獲得最大頻寬
  writeRegister(0x1A, 0x00); // DLPF_CFG = 0
  delay(10);
}

void writeRegister(uint8_t reg, uint8_t value) {
  Wire1.beginTransmission(MPU);
  Wire1.write(reg);
  Wire1.write(value);
  Wire1.endTransmission();
}

void loop() {
  unsigned long currentTime = millis();
  
  // 1kHz取樣 (每1ms讀取一次)
  if (currentTime - lastTime >= 1) {
    lastTime = currentTime;
    
    readAcceleration();
    
    // 輸出格式適合Arduino IDE序列埠繪圖家
    Serial.print(AccX, 3);
    Serial.print(",");
    Serial.print(AccY, 3);
    Serial.print(",");
    Serial.println(AccZ, 3);
  }
}

void readAcceleration() {
  Wire1.beginTransmission(MPU);
  Wire1.write(0x3B); // ACCEL_XOUT_H register
  Wire1.endTransmission(false);
  Wire1.requestFrom(MPU, 6, true);
  
  if (Wire1.available() >= 6) {
    // 讀取並組合16位元數據
    int16_t rawX = (Wire1.read() << 8) | Wire1.read();
    int16_t rawY = (Wire1.read() << 8) | Wire1.read();
    int16_t rawZ = (Wire1.read() << 8) | Wire1.read();
    
    // 轉換為g值 (±2g範圍: 16384 LSB/g)
    AccX = rawX / 16384.0;
    AccY = rawY / 16384.0;
    AccZ = rawZ / 16384.0;
  }
}